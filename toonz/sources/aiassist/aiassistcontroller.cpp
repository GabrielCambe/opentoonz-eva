#include "aiassistcontroller.h"
#include "comfyclient.h"

// Toonz includes
#include "iocommand.h"
#include "tapp.h"

// TnzLib includes
#include "toonz/toonzscene.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/txshsimplelevel.h"
#include "toonz/tframehandle.h"
#include "toonz/tscenehandle.h"

// TnzCore includes
#include "trasterimage.h"
#include "ttoonzimage.h"
#include "tvectorimage.h"
#include "tvectorrenderdata.h"
#include "tofflinegl.h"
#include "trop.h"
#include "tropcm.h"
#include "toonz/txsheet.h"
#include "toonz/txshcell.h"
#include "toonz/txsheethandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/imagemanager.h"
#include "toonz/tcamera.h"
#include "toonz/txshleveltypes.h"
#include "tsystem.h"

// TnzQt includes
#include "toonzqt/gutil.h"

// Qt includes
#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include "tundo.h"

#include <QDir>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

namespace {

//! Onde o painel guarda a URL do ComfyUI enquanto ela nao vira preferencia
//! do OpenToonz. QSettings mantem a divergencia contra o upstream em zero
//! arquivos extras; migrar para Preferences e trabalho da fase 4.
const char *kUrlKey     = "aiassist/comfyui_url";
const char *kDefaultUrl = "http://127.0.0.1:8188";

//! Duas pausas porque sao dois gestos diferentes. 300 ms e a pausa entre
//! palavras digitadas. 600 ms e a pausa entre tracos: o sinal de desenho chega
//! uma vez por traco concluido, entao o que se mede aqui e o intervalo de um
//! traco ao proximo, e ao hachurar eles se sucedem bem mais rapido que isso.
//! Calibrado com o usuario; ver D1 em tasks/knowledge/aiassist_live_draw__design.md.
const int kTextDebounceMs = 300;
const int kDrawDebounceMs = 600;

//! Prefixo dos arquivos que este painel gera. Ver currentLevelIsGenerated().
const char *kGeneratedPrefix = "aiassist_";

//! Mantem um bool ligado enquanto vive. onInsertAsLevel() tem varias saidas
//! antecipadas, e esquecer de desligar a flag em uma delas deixaria o gatilho
//! por desenho mudo para sempre - defeito silencioso, que so apareceria muito
//! depois e sem nada apontando para a causa.
class ScopedFlag {
public:
  explicit ScopedFlag(bool &flag) : m_flag(flag) { m_flag = true; }
  ~ScopedFlag() { m_flag = false; }
  ScopedFlag(const ScopedFlag &)            = delete;
  ScopedFlag &operator=(const ScopedFlag &) = delete;

private:
  bool &m_flag;
};

QString templatesFolder() {
  // AIASSIST_GRAPHS permite apontar para a pasta do repositorio durante o
  // desenvolvimento, sem depender de uma etapa de deploy.
  const QByteArray env = qgetenv("AIASSIST_GRAPHS");
  if (!env.isEmpty()) return QString::fromLocal8Bit(env);
  return QDir(QCoreApplication::applicationDirPath())
      .absoluteFilePath(QStringLiteral("aiassist/graphs"));
}

}  // namespace

//-----------------------------------------------------------------------------

AIAssistController::AIAssistController(QWidget *parent) : QWidget(parent) {
  // Sem isto o painel fica preto. O QSS do OpenToonz pinta QWidget com #484848 e
  // TPanel com #101010, mas o Qt so aplica background de folha de estilo em
  // subclasses proprias de QWidget quando WA_StyledBackground esta ligado. Sem a
  // flag esta widget nao pinta nada e aparece o preto do TPanel por baixo.
  setAttribute(Qt::WA_StyledBackground, true);
  setObjectName(QStringLiteral("AIAssistController"));

  m_client = new ComfyClient(this);
  m_client->setBaseUrl(
      QSettings().value(QLatin1String(kUrlKey), QLatin1String(kDefaultUrl))
          .toString());

  m_templateCombo  = new QComboBox(this);
  m_promptField    = new QLineEdit(this);
  m_negativeField  = new QLineEdit(this);
  m_seedField      = new QSpinBox(this);
  m_useReferenceCheck =
      new QCheckBox(tr("Use the drawing I am editing as reference"), this);
  m_useReferenceCheck->setChecked(true);
  // Um checkbox so para os dois gatilhos, e nao um por gatilho: assim
  // liveIsActive() continua sendo o unico predicado que autoriza trabalho ao
  // vivo, e o invariante "com o Live desligado o painel nao faz nada" fica
  // garantido num lugar so. Ver D2 no design.
  m_liveCheck =
      new QCheckBox(tr("Live: regenerate as I draw or type"), this);

  // Disparar a cada tecla, como o Acer Drawing Assistant faz, produz trabalho
  // que ja nasce obsoleto: no /history deles ha 143 prompts progressivamente
  // digitados em 200 geracoes. O intervalo efetivo nao vem daqui - quem arma o
  // timer e armLive(), que escolhe a pausa conforme o gatilho. O valor abaixo e
  // so o estado inicial.
  m_debounce = new QTimer(this);
  m_debounce->setSingleShot(true);
  m_debounce->setInterval(kTextDebounceMs);

  // Indicador de progresso. Quadros em ASCII de proposito: a fonte da UI vem do
  // tema e o MSVC ja emite C4566 para literais nao-ASCII neste projeto. Um
  // spinner que vira caixinha em alguma maquina e pior que um spinner feio.
  m_spinTimer = new QTimer(this);
  m_spinTimer->setInterval(120);
  m_generateButton = new QPushButton(tr("Generate"), this);
  m_insertButton   = new QPushButton(tr("Insert as Level"), this);
  m_previewLabel   = new QLabel(this);
  m_statusLabel    = new QLabel(this);

  m_promptField->setPlaceholderText(tr("What should it become?"));
  m_negativeField->setPlaceholderText(tr("What to avoid (optional)"));
  m_seedField->setRange(0, 2147483647);
  m_seedField->setValue(0);
  m_previewLabel->setMinimumSize(256, 256);
  m_previewLabel->setAlignment(Qt::AlignCenter);
  m_previewLabel->setFrameShape(QFrame::StyledPanel);
  m_previewLabel->setText(tr("No image yet"));
  m_statusLabel->setWordWrap(true);
  m_insertButton->setEnabled(false);

  QFormLayout *form = new QFormLayout();
  form->addRow(tr("Graph"), m_templateCombo);
  form->addRow(tr("Prompt"), m_promptField);
  form->addRow(tr("Negative"), m_negativeField);
  form->addRow(tr("Seed"), m_seedField);
  form->addRow(QString(), m_useReferenceCheck);
  form->addRow(QString(), m_liveCheck);

  QHBoxLayout *buttons = new QHBoxLayout();
  buttons->addWidget(m_generateButton);
  buttons->addWidget(m_insertButton);

  QVBoxLayout *root = new QVBoxLayout(this);
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(8);
  root->addLayout(form);
  root->addLayout(buttons);
  root->addWidget(m_previewLabel, 1);
  root->addWidget(m_statusLabel);
  setLayout(root);

  connect(m_generateButton, &QPushButton::clicked, this,
          &AIAssistController::onGenerate);
  connect(m_insertButton, &QPushButton::clicked, this,
          &AIAssistController::onInsertAsLevel);
  connect(m_templateCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onTemplateChanged(int)));
  // O texto de status muda conforme o check, entao reaproveita o mesmo slot.
  connect(m_useReferenceCheck, &QCheckBox::toggled, this, [this](bool) {
    onTemplateChanged(m_templateCombo->currentIndex());
  });

  connect(m_promptField, &QLineEdit::textEdited, this,
          [this](const QString &) {
            if (liveIsActive()) armLive(LiveTrigger::Text);
          });
  connect(m_debounce, &QTimer::timeout, this,
          &AIAssistController::maybeGenerateLive);
  connect(m_spinTimer, &QTimer::timeout, this, [this]() {
    ++m_spinFrame;
    refreshBusyStatus();
  });
  connect(m_liveCheck, &QCheckBox::toggled, this, [this](bool on) {
    if (on)
      maybeGenerateLive();
    else
      m_debounce->stop();
    onTemplateChanged(m_templateCombo->currentIndex());
  });

  // A linha de escala fala da celula corrente, entao precisa acompanhar quando
  // ela muda. Sem isto ela mentiria assim que o usuario trocasse de frame.
  TApp *app = TApp::instance();
  auto refresh = [this]() { onTemplateChanged(m_templateCombo->currentIndex()); };
  connect(app->getCurrentFrame(), &TFrameHandle::frameSwitched, this, refresh);
  connect(app->getCurrentColumn(), &TColumnHandle::columnIndexSwitched, this,
          refresh);
  connect(app->getCurrentLevel(), &TXshLevelHandle::xshLevelSwitched, this,
          [refresh](TXshLevel *) { refresh(); });

  // O segundo gatilho: o desenho. xshLevelChanged chega uma vez por traco
  // concluido, na soltura do ponteiro - o caminho por evento de movimento nao
  // notifica nada. A cadeia foi verificada de inputmanager.cpp:276 ate
  // txshlevelhandle.h:40 e esta escrita em
  // tasks/knowledge/aiassist_live_draw__loop_paths.md.
  //
  // A guarda vem antes de tudo de proposito: com o Live desligado a lambda
  // retorna sem rasterizar, sem converter e sem tocar no timer. E o que I3
  // exige, e e barato porque liveIsActive() so olha um checkbox e o template.
  connect(app->getCurrentLevel(), &TXshLevelHandle::xshLevelChanged, this,
          &AIAssistController::onDrawingChanged);

  // Segunda fonte para o mesmo fato. Desfazer muda o desenho tanto quanto
  // desenhar, mas nao emite xshLevelChanged: o undo das ferramentas passa por
  // ToolUtils::TToolUndo::notifyImageChanged(), que invalida icone e atualiza a
  // ferramenta sem notificar o level handle (toolutils.cpp:620). O level handle
  // so e avisado quando o undo criou ou removeu frame/level (:617). Resultado:
  // Ctrl+Z depois de um traco ficava sem regenerar.
  //
  // historyChanged cobre undo, redo e o registro de novos undos (tundo.cpp:259,
  // :288 e :331). Ele e mais largo que o desenho - qualquer operacao que
  // registre undo passa por aqui -, e isso e deliberado: o pedido foi que
  // qualquer alteracao regenere. As guardas de onDrawingChanged() filtram o que
  // nao deve disparar, e o debounce funde a emissao dupla de um mesmo traco.
  connect(TUndoManager::manager(), &TUndoManager::historyChanged, this,
          &AIAssistController::onDrawingChanged);
  connect(m_client, &ComfyClient::imageReady, this,
          &AIAssistController::onImageReady);
  connect(m_client, &ComfyClient::failed, this, &AIAssistController::onFailed);
  connect(m_client, &ComfyClient::status, this, &AIAssistController::onStatus);

  reloadTemplates();
}

//-----------------------------------------------------------------------------

void AIAssistController::reloadTemplates() {
  QStringList rejected;
  m_templates = GraphTemplate::scanFolder(templatesFolder(), rejected);

  m_templateCombo->clear();
  for (const GraphTemplate &tpl : m_templates)
    m_templateCombo->addItem(tpl.name());

  if (m_templates.isEmpty())
    m_statusLabel->setText(
        tr("No valid graph found in %1").arg(templatesFolder()));
  else if (!rejected.isEmpty())
    m_statusLabel->setText(tr("%1 graph(s) rejected: %2")
                               .arg(rejected.size())
                               .arg(rejected.join(QStringLiteral("; "))));
  m_generateButton->setEnabled(!m_templates.isEmpty());
  onTemplateChanged(m_templateCombo->currentIndex());
}

//-----------------------------------------------------------------------------

bool AIAssistController::liveIsActive() const {
  if (!m_liveCheck->isChecked()) return false;
  const int index = m_templateCombo->currentIndex();
  if (index < 0 || index >= m_templates.size()) return false;
  return m_templates.at(index).isLiveCapable();
}

//-----------------------------------------------------------------------------

void AIAssistController::onDrawingChanged() {
  if (!liveIsActive()) return;

  // Duas guardas contra realimentacao, e elas cobrem coisas diferentes. A flag
  // cobre a janela em que a insercao roda - e ela vale mais desde que
  // historyChanged entrou, porque Insert as Level registra undo e passaria por
  // aqui. O teste de nivel cobre o estado DEPOIS da insercao: ela move a coluna
  // corrente para a coluna do PNG gerado, e e da coluna corrente que sai a
  // referencia. Ver D3 no design e
  // tasks/knowledge/aiassist_live_draw__loop_paths.md.
  if (m_suppressLiveTrigger) return;
  if (currentLevelIsGenerated()) return;

  // A revisao avanca aqui, e nao no momento da geracao: e este o instante em que
  // se sabe que o desenho mudou. Se a geracao em voo terminar depois disto,
  // coalesceLive() ve serial != submetido e dispara mais uma vez - que e o que
  // impede a mudanca de se perder.
  ++m_drawSerial;
  armLive(LiveTrigger::Drawing);
}

//-----------------------------------------------------------------------------

void AIAssistController::armLive(LiveTrigger trigger) {
  m_lastLiveTrigger = trigger;
  m_debounce->start(trigger == LiveTrigger::Drawing ? kDrawDebounceMs
                                                    : kTextDebounceMs);
}

//-----------------------------------------------------------------------------

void AIAssistController::maybeGenerateLive() {
  if (!liveIsActive()) return;

  // Um prompt vazio nao significa a mesma coisa nos dois gatilhos. Digitando,
  // campo vazio quer dizer que nao ha pedido - e esse e o comportamento de
  // hoje, que nao pode mudar. Desenhando, o pedido esta no traco: um grafo que
  // consome referencia tem o que fazer sem uma palavra sequer. Sem referencia,
  // porem, prompt vazio geraria ruido puro. Ver D5 no design.
  if (m_promptField->text().trimmed().isEmpty()) {
    if (m_lastLiveTrigger != LiveTrigger::Drawing) return;
    const int index = m_templateCombo->currentIndex();
    if (index < 0 || index >= m_templates.size()) return;
    if (!m_templates.at(index).consumesReference()) return;
    if (!m_useReferenceCheck->isChecked()) return;
  }

  // Um unico job em voo, sempre. Enfileirar no servidor e o que faz a imagem
  // ficar para tras do que voce ja digitou. Se estiver ocupado, nao se guarda
  // nada aqui: coalesceLive() vai olhar o campo no fim do ciclo e ele ja tera o
  // texto mais novo por definicao.
  if (m_client->isBusy()) return;
  onGenerate();
}

//-----------------------------------------------------------------------------

void AIAssistController::coalesceLive() {
  if (!liveIsActive()) return;
  // Coalescencia final: durante o ciclo que acabou, o usuario pode ter digitado
  // varias teclas ou desenhado varios tracos. Dispara UMA vez, com o estado mais
  // recente, em vez de uma vez por tecla ou por traco. Assim o numero de
  // geracoes e limitado pela velocidade da GPU, e nao pela velocidade da mao - e
  // nunca se fica atrasado.
  //
  // Comparar so o texto nao bastava. Desenhando, o prompt nao muda entre
  // geracoes: a comparacao retornava cedo e a coalescencia nunca re-disparava,
  // entao desenhar durante uma geracao perdia a mudanca sem aviso nenhum.
  const bool textChanged     = m_promptField->text() != m_submittedPrompt;
  const bool drawingChanged  = m_drawSerial != m_submittedDrawSerial;
  if (!textChanged && !drawingChanged) return;

  // Quem mudou decide a pausa. Se foi o desenho, vale a pausa entre tracos,
  // mesmo que o ultimo gatilho registrado tenha sido o texto.
  armLive(drawingChanged ? LiveTrigger::Drawing : LiveTrigger::Text);
}

//-----------------------------------------------------------------------------

void AIAssistController::onTemplateChanged(int index) {
  if (index < 0 || index >= m_templates.size()) return;
  const GraphTemplate &tpl = m_templates.at(index);

  // Tres estados distintos, nao dois: o no AI_NEGATIVE pode nem existir, ou
  // existir e nao ter efeito porque o grafo roda em cfg 1.0 (LCM). Nos dois
  // casos o campo fica desabilitado, mas o motivo muda.
  //
  // O motivo vai no statusLabel, nao em tooltip nem placeholder do proprio
  // campo: o Qt nao entrega evento de tooltip para widget desabilitada, e
  // placeholder em campo desabilitado sai esmaecido a ponto de sumir em alguns
  // temas. Uma explicacao que so aparece quando ja nao da para ler nao explica
  // nada.
  const bool effective = tpl.negativeHasEffect();
  m_negativeField->setEnabled(effective);

  // Sem AI_REFERENCE nao ha o que ligar ou desligar: o grafo e txt2img por
  // construcao. O check fica desabilitado em vez de sumir, para o usuario ver
  // que a opcao existe e por que nao se aplica aqui.
  m_useReferenceCheck->setEnabled(tpl.consumesReference());
  m_liveCheck->setEnabled(tpl.isLiveCapable());
  if (!tpl.isLiveCapable() && m_liveCheck->isChecked())
    m_liveCheck->setChecked(false);

  QStringList notes;
  if (!tpl.consumesReference())
    notes << tr("This graph ignores the drawing (text only).");
  else if (!m_useReferenceCheck->isChecked())
    notes << tr("Reference off: ControlNet strength goes to 0 and denoise to "
                "1.0, so the drawing has no influence.");
  else
    notes << describeReferenceScaling(tpl);
  if (!tpl.isLiveCapable())
    notes << tr("Live mode off: this graph runs %1 steps. Regenerating while "
                "you draw or type only works on fast graphs (8 steps or "
                "fewer).")
                 .arg(tpl.samplerSteps());
  else if (m_liveCheck->isChecked()) {
    // O texto tem de descrever o gatilho vigente sem mentir: sao duas pausas
    // diferentes porque sao dois gestos diferentes, e dizer so uma delas seria
    // pior que nao dizer nenhuma.
    notes << tr("Live on: regenerates %1 ms after you stop drawing, or %2 ms "
                "after you stop typing, one at a time. Keep the seed fixed so "
                "your changes are the only variable.")
                 .arg(kDrawDebounceMs)
                 .arg(kTextDebounceMs);
    // E quando o desenho corrente e um resultado ja inserido, o gatilho por
    // desenho esta desligado de proposito. Sem dizer isso, o painel pareceria
    // simplesmente quebrado ali.
    if (currentLevelIsGenerated())
      notes << tr("This cell holds a generated image, so drawing here will not "
                  "retrigger - it would feed the result back in as its own "
                  "reference. Draw on your own level to regenerate.");
  }

  if (!effective) {
    if (!tpl.hasNegativePrompt())
      notes << tr("Negative prompt off: this graph has no AI_NEGATIVE node.");
    else
      notes << tr("Negative prompt off: this graph runs at cfg %1. The sampler "
                  "computes noise = uncond + cfg * (cond - uncond), so at cfg 1 "
                  "the negative term cancels out and would be ignored.")
                   .arg(tpl.samplerCfg(), 0, 'g', 3);
  }
  m_statusLabel->setText(notes.join(QLatin1Char('\n')));
}

//-----------------------------------------------------------------------------

namespace {

//! Rasteriza um TVectorImage sem tocar no original. O level continua vetorial no
//! ComboViewer; o que vai para o ComfyUI e uma copia rasterizada e descartavel.
//! Precedente: toonzlib/scriptbinding_rasterizer.cpp renderVectorImage().
TRaster32P rasterizeVector(const TVectorImageP &vi, ToonzScene *scene,
                           QString &errorOut) {
  TPalette *palette = vi->getPalette();
  if (!palette) {
    errorOut = QObject::tr("This vector level has no palette to render with.");
    return TRaster32P();
  }

  TCamera *camera = scene ? scene->getCurrentCamera() : nullptr;
  if (!camera) {
    errorOut = QObject::tr("No current camera to frame the reference.");
    return TRaster32P();
  }
  const TDimension res = camera->getRes();

  TOfflineGL glContext(res);
  glContext.makeCurrent();
  glContext.clear(TPixel32::White);

  TVectorRenderData rd(TVectorRenderData::ProductionSettings(),
                       camera->getStageToCameraRef(), TRect(), palette);
  rd.m_antiAliasing = true;
  glContext.draw(vi, rd);

  // getRaster() ja devolve uma copia do buffer offline, nao um ponteiro para
  // dentro do contexto: pode sobreviver ao fim desta funcao.
  TRaster32P rendered = glContext.getRaster();
  if (!rendered)
    errorOut = QObject::tr("Could not render the vector level.");
  return rendered;
}

//! Converte qualquer tipo de imagem de level para RGBA de 32 bits.
//! Raster puro passa direto; Toonz Raster (CM32) precisa da paleta para virar
//! cor; vetor passa pelo render offline.
TRaster32P imageToRaster32(const TImageP &img, ToonzScene *scene,
                           QString &errorOut) {
  if (TRasterImageP ri = img) {
    TRasterP ras = ri->getRaster();
    if (!ras) {
      errorOut = QObject::tr("The current drawing has no pixels.");
      return TRaster32P();
    }
    if (TRaster32P already = ras) return already;
    TRaster32P out(ras->getSize());
    TRop::convert(out, ras);
    return out;
  }

  if (TToonzImageP ti = img) {
    TRasterCM32P cm = ti->getRaster();
    if (!cm) {
      errorOut = QObject::tr("The current drawing has no pixels.");
      return TRaster32P();
    }
    // Um .tlv guarda indices de paleta, nao cor. Sem este convert o painel
    // recebia um cast nulo e reclamava que nao havia raster nenhum.
    TRaster32P out(cm->getSize());
    TRop::convert(out, cm, ti->getPalette(), TRect());
    return out;
  }

  if (TVectorImageP vi = img) return rasterizeVector(vi, scene, errorOut);

  errorOut = QObject::tr("Unsupported level type for reference.");
  return TRaster32P();
}

}  // namespace

//-----------------------------------------------------------------------------

QString AIAssistController::blankReferencePngBase64() {
  // 512 e o tamanho para onde o ImageScale do template leva qualquer entrada;
  // mandar ja nesse tamanho evita reescala inutil no servidor.
  QImage blank(512, 512, QImage::Format_RGB32);
  blank.fill(Qt::white);

  QByteArray png;
  QBuffer buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  blank.save(&buffer, "PNG");
  return QString::fromLatin1(png.toBase64());
}

//-----------------------------------------------------------------------------

QString AIAssistController::describeReferenceScaling(
    const GraphTemplate &tpl) const {
  int srcW = 0, srcH = 0;
  const bool haveSource = referenceSourceSize(srcW, srcH);

  int dstW = 0, dstH = 0;
  const bool haveTarget = tpl.referenceScaleTarget(dstW, dstH);

  if (!haveSource)
    return tr("Uses the cell you are editing as reference (none selected yet).");

  const QString source = tr("Reference: %1x%2").arg(srcW).arg(srcH);
  if (!haveTarget)
    return source + tr(" - this graph does not say how it resizes it.");

  if (srcW == dstW && srcH == dstH)
    return source + tr(" - sent as is.");

  // ImageScale com crop desabilitado leva a exatamente WxH, sem preservar
  // proporcao. Se as proporcoes nao batem o desenho chega esticado, e isso
  // muda o resultado o suficiente para merecer aviso.
  const bool distorts = srcW * dstH != srcH * dstW;
  const bool shrinks  = dstW < srcW || dstH < srcH;

  QString note = source + tr(" -> %1x%2").arg(dstW).arg(dstH);
  if (distorts)
    note += tr(" (scaled %1 and stretched: aspect ratio not preserved)")
                .arg(shrinks ? tr("down") : tr("up"));
  else
    note += shrinks ? tr(" (scaled down)") : tr(" (scaled up)");
  return note;
}

//-----------------------------------------------------------------------------

bool AIAssistController::resolveCurrentCell(TXshSimpleLevel *&slOut,
                                            TFrameId &fidOut) const {
  TApp *app                 = TApp::instance();
  TFrameHandle *frameHandle = app->getCurrentFrame();
  slOut                     = nullptr;
  fidOut                    = TFrameId();

  // Duas maneiras diferentes de "o desenho atual", e usar a errada foi um bug
  // real: TFrameHandle::getFid() so vale no modo LevelFrame (Level Strip). Ao
  // desenhar no ComboViewer sobre uma coluna o app esta em SceneFrame, onde o
  // handle guarda uma LINHA e o fid armazenado esta velho. Ai a referencia certa
  // e a celula do xsheet.
  if (frameHandle->isEditingLevel()) {
    slOut  = app->getCurrentLevel() ? app->getCurrentLevel()->getSimpleLevel()
                                    : nullptr;
    fidOut = frameHandle->getFid();
  } else {
    TXsheet *xsh = app->getCurrentXsheet() ? app->getCurrentXsheet()->getXsheet()
                                           : nullptr;
    const int col = app->getCurrentColumn()
                        ? app->getCurrentColumn()->getColumnIndex()
                        : -1;
    if (xsh && col >= 0) {
      const TXshCell cell = xsh->getCell(frameHandle->getFrame(), col);
      slOut               = cell.getSimpleLevel();
      fidOut              = cell.getFrameId();
    }
  }
  return slOut != nullptr;
}

//-----------------------------------------------------------------------------

bool AIAssistController::referenceSourceSize(int &widthOut,
                                             int &heightOut) const {
  TXshSimpleLevel *sl = nullptr;
  TFrameId fid;
  if (!resolveCurrentCell(sl, fid)) return false;

  // Vetor nao tem resolucao propria: e rasterizado no enquadramento da camera,
  // entao o tamanho da referencia e o da camera, nao o do level.
  if (sl->getType() == PLI_XSHLEVEL) {
    TApp *app         = TApp::instance();
    ToonzScene *scene = app->getCurrentScene() ? app->getCurrentScene()->getScene()
                                               : nullptr;
    TCamera *camera   = scene ? scene->getCurrentCamera() : nullptr;
    if (!camera) return false;
    widthOut  = camera->getRes().lx;
    heightOut = camera->getRes().ly;
    return true;
  }

  const TDimension res = sl->getResolution();
  if (res.lx <= 0 || res.ly <= 0) return false;
  widthOut  = res.lx;
  heightOut = res.ly;
  return true;
}

//-----------------------------------------------------------------------------

bool AIAssistController::currentLevelIsGenerated() const {
  TXshSimpleLevel *sl = nullptr;
  TFrameId fid;
  if (!resolveCurrentCell(sl, fid)) return false;

  // Guarda por convencao de nome, com o custo assumido: renomear o nivel a
  // desarma. A alternativa seria registrar os caminhos inseridos, o que so
  // sobreviveria a uma sessao. Ver D3 no design.
  const TFilePath path = sl->getPath();
  const std::string name = path.getName();
  if (name.rfind(kGeneratedPrefix, 0) != 0) return false;
  return path.getType() == "png";
}

//-----------------------------------------------------------------------------

QString AIAssistController::currentFrameAsPngBase64(QString &errorOut) const {
  TApp *app = TApp::instance();

  TXshSimpleLevel *sl = nullptr;
  TFrameId fid;
  resolveCurrentCell(sl, fid);

  if (!sl) {
    errorOut = tr("No drawing here. Select a cell with a level, or draw one.");
    return QString();
  }

  const TImageP img = sl->getFullsampledFrame(fid, ImageManager::dontPutInCache);
  if (!img) {
    errorOut = tr("This cell has no image yet.");
    return QString();
  }

  ToonzScene *scene =
      app->getCurrentScene() ? app->getCurrentScene()->getScene() : nullptr;
  const TRaster32P ras = imageToRaster32(img, scene, errorOut);
  if (!ras) {
    if (errorOut.isEmpty())
      errorOut = tr("Could not read the current drawing.");
    return QString();
  }

  const QImage drawing = rasterToQImage(ras, false);
  if (drawing.isNull()) {
    errorOut = tr("Could not convert the current drawing to an image.");
    return QString();
  }

  // Achatar sobre branco antes de mandar. Um .tlv ou um raster com alpha tem
  // fundo transparente, e o ComfyUI descarta o canal alpha ao carregar: os
  // pixels vazios virariam pretos e o ControlNet leria a folha inteira como
  // traco. O artista desenha sobre branco, entao a referencia vai sobre branco.
  QImage image(drawing.size(), QImage::Format_RGB32);
  image.fill(Qt::white);
  {
    QPainter painter(&image);
    painter.drawImage(0, 0, drawing);
  }

  QByteArray png;
  QBuffer buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  if (!image.save(&buffer, "PNG")) {
    errorOut = tr("Could not encode the current drawing as PNG.");
    return QString();
  }
  return QString::fromLatin1(png.toBase64());
}

//-----------------------------------------------------------------------------

void AIAssistController::setBusy(bool busy) {
  m_generateButton->setEnabled(!busy && !m_templates.isEmpty());
  m_templateCombo->setEnabled(!busy);

  if (busy) {
    m_spinFrame = 0;
    m_busyMessage.clear();  // senao o primeiro quadro mostra o "Done." anterior
    m_elapsed.start();
    m_spinTimer->start();
    refreshBusyStatus();
  } else {
    m_spinTimer->stop();
  }
}

//-----------------------------------------------------------------------------

void AIAssistController::refreshBusyStatus() {
  static const char *kFrames[] = {"|", "/", "-", "\\"};
  const QString frame =
      QString::fromLatin1(kFrames[m_spinFrame % 4]);

  // Os segundos nao sao enfeite: o timeout do cliente e de 300 s, e sem eles
  // nao da para distinguir "o servidor esta lento" de "travou". Em modo ao vivo
  // tambem mostram, na pratica, quanto custa cada tecla.
  const qint64 seconds = m_elapsed.elapsed() / 1000;
  m_statusLabel->setText(QStringLiteral("%1 %2 (%3s)")
                             .arg(frame, m_busyMessage.isEmpty()
                                             ? tr("Working...")
                                             : m_busyMessage)
                             .arg(seconds));
}

//-----------------------------------------------------------------------------

void AIAssistController::onGenerate() {
  const int index = m_templateCombo->currentIndex();
  if (index < 0 || index >= m_templates.size()) return;
  const GraphTemplate &tpl = m_templates.at(index);

  const bool useReference =
      tpl.consumesReference() && m_useReferenceCheck->isChecked();

  QString reference;
  if (tpl.consumesReference()) {
    if (useReference) {
      QString error;
      reference = currentFrameAsPngBase64(error);
      if (reference.isEmpty()) {
        onFailed(error);
        return;
      }
    } else {
      // Com a referencia desligada o no ainda precisa de uma imagem valida.
      // Manda-se uma folha branca: barata, e ja neutralizada por strength 0 e
      // denoise 1.0 la no build(). Nao exigir celula aqui e o ponto do toggle -
      // da para gerar sem ter desenho nenhum na cena.
      reference = blankReferencePngBase64();
    }
  }

  setBusy(true);
  m_insertButton->setEnabled(false);
  // Se o negativo nao tem efeito neste grafo, manda vazio em vez do texto que
  // sobrou de outro template. Nao muda a imagem - a matematica ja o descarta -
  // mas evita que o /history do ComfyUI registre um negativo que nunca agiu.
  const QString negative =
      tpl.negativeHasEffect() ? m_negativeField->text() : QString();

  // As duas identidades do que esta sendo submetido, gravadas juntas: o texto e
  // a revisao do desenho. coalesceLive() compara as duas com o estado atual.
  m_submittedPrompt     = m_promptField->text();
  m_submittedDrawSerial = m_drawSerial;
  m_client->generate(
      tpl.build(m_submittedPrompt, negative, reference, m_seedField->value(),
                useReference),
      tpl.outputNodeId());
}

//-----------------------------------------------------------------------------

void AIAssistController::onImageReady(const QImage &image) {
  setBusy(false);
  m_result = image;
  m_previewLabel->setPixmap(QPixmap::fromImage(image).scaled(
      m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  m_insertButton->setEnabled(true);
  coalesceLive();
  m_statusLabel->setText(tr("Generated %1x%2. Nothing was added to the scene yet.")
                             .arg(image.width())
                             .arg(image.height()));
}

//-----------------------------------------------------------------------------

void AIAssistController::onFailed(const QString &message) {
  setBusy(false);
  // Nao rechama coalesceLive() aqui: se a geracao falhou, tentar de novo
  // sozinho a cada 300 ms transformaria um servidor fora do ar num laco de
  // requisicoes - exatamente o defeito que diagnosticamos no cliente da Acer.
  // O usuario digita mais uma tecla e ai sim tenta de novo.
  if (m_liveCheck->isChecked()) m_liveCheck->setChecked(false);
  m_statusLabel->setText(message);
}

void AIAssistController::onStatus(const QString &message) {
  m_busyMessage = message;
  if (m_spinTimer->isActive()) {
    refreshBusyStatus();
    return;
  }
  m_statusLabel->setText(message);
}

//-----------------------------------------------------------------------------

void AIAssistController::onInsertAsLevel() {
  if (m_result.isNull()) return;

  // Ver o comentario da flag no header: hoje o carregador nao emite o sinal do
  // gatilho, mas isso nao e um contrato, e um detalhe do IoCmd.
  ScopedFlag suppress(m_suppressLiveTrigger);

  ToonzScene *scene = TApp::instance()->getCurrentScene()->getScene();
  if (!scene) {
    onFailed(tr("No scene is open."));
    return;
  }

  // Grava na pasta +drawings da cena e deixa o proprio OpenToonz carregar o
  // arquivo. Reaproveitar IoCmd::loadResources evita reimplementar criacao de
  // level, undo e level set a mao.
  const TFilePath folder = scene->decodeFilePath(TFilePath("+drawings"));
  TSystem::mkDir(folder);

  TFilePath fp;
  for (int i = 1; i < 10000; ++i) {
    fp = folder + TFilePath(QString("aiassist_%1.png")
                                .arg(i, 4, 10, QChar('0'))
                                .toStdWString());
    if (!TFileStatus(fp).doesExist()) break;
  }

  if (!m_result.save(QString::fromStdWString(fp.getWideString()), "PNG")) {
    onFailed(tr("Could not write %1").arg(
        QString::fromStdWString(fp.getWideString())));
    return;
  }

  IoCmd::LoadResourceArguments args(fp);
  const int loaded = IoCmd::loadResources(args);
  if (loaded <= 0) {
    onFailed(tr("Saved to %1 but OpenToonz did not load it as a level.")
                 .arg(QString::fromStdWString(fp.getWideString())));
    return;
  }
  m_statusLabel->setText(tr("Inserted as level from %1")
                             .arg(QString::fromStdWString(fp.getWideString())));
}
