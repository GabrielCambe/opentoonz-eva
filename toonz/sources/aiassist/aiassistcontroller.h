#pragma once
#ifndef AIASSIST_AIASSISTCONTROLLER_H
#define AIASSIST_AIASSISTCONTROLLER_H

#include "graphtemplate.h"

#include "tfilepath.h"

#include <QElapsedTimer>
#include <QImage>
#include <QVector>
#include <QWidget>

class ComfyClient;
class TXshSimpleLevel;
class QComboBox;
class QLabel;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTimer;

//! Painel de geracao por IA.
//!
//! Usa o frame corrente como referencia visual, envia para um ComfyUI
//! configuravel, mostra o resultado e deixa a insercao como level a cargo
//! explicito do usuario. Nada aqui assume onde o ComfyUI esta instalado.
class AIAssistController final : public QWidget {
  Q_OBJECT

public:
  explicit AIAssistController(QWidget *parent = nullptr);

private slots:
  void onGenerate();
  void onInsertAsLevel();
  void onTemplateChanged(int index);
  void onImageReady(const QImage &image);
  void onFailed(const QString &message);
  void onStatus(const QString &message);

private:
  void reloadTemplates();
  void setBusy(bool busy);

  //! Dispara uma geracao ao vivo se for a hora. Se o cliente estiver ocupado
  //! nao enfileira nada: a coalescencia acontece no fim do ciclo atual.
  void maybeGenerateLive();
  //! Chamado quando um ciclo termina. Se o texto mudou desde o que foi
  //! submetido, dispara uma unica vez com o texto mais novo.
  void coalesceLive();
  bool liveIsActive() const;

  //! Qual dos dois gatilhos armou o debounce. Nao e cosmetico: decide o
  //! intervalo (300 ms e a pausa entre palavras digitadas, 600 ms a pausa entre
  //! tracos) e, em maybeGenerateLive(), se um prompt vazio ainda vale um
  //! pedido. Ver D1 e D5 em tasks/knowledge/aiassist_live_draw__design.md.
  enum class LiveTrigger { Text, Drawing };

  //! Ponto unico de armacao do debounce. Existe porque QTimer::start(int) grava
  //! o intervalo no proprio timer: armar em um lugar com 600 ms e em outro com
  //! start() sem argumento faria o segundo herdar os 600 ms em silencio.
  void armLive(LiveTrigger trigger);

  //! Um caminho unico para "o desenho corrente mudou", vindo de qualquer fonte.
  //! Hoje sao duas: o fim de um traco (TXshLevelHandle::xshLevelChanged) e o
  //! undo/redo (TUndoManager::historyChanged). Desfazer nao emite o primeiro -
  //! ToolUtils::TToolUndo::notifyImageChanged() so invalida icone e atualiza a
  //! ferramenta, sem notificar o level handle -, e sem o segundo um Ctrl+Z ficava
  //! sem regenerar. Ver SC7 no Intent Brief.
  void onDrawingChanged();

  //! Redesenha o statusLabel com o quadro atual do indicador. Chamado pelo
  //! tique do spinner e sempre que a mensagem base muda.
  void refreshBusyStatus();

  //! Resolve o desenho corrente: celula do xsheet quando editando a cena,
  //! frame do level quando editando o level. Ver comentario no .cpp.
  bool resolveCurrentCell(TXshSimpleLevel *&slOut, TFrameId &fidOut) const;

  //! Tamanho em pixels que a referencia tera antes de qualquer reescala do
  //! grafo. Falso quando nao ha desenho corrente.
  bool referenceSourceSize(int &widthOut, int &heightOut) const;

  //! Uma frase dizendo o tamanho da referencia e como o grafo vai reescala-la.
  QString describeReferenceScaling(const GraphTemplate &tpl) const;

  //! Verdadeiro quando o desenho corrente e um resultado que este painel
  //! inseriu (aiassist_NNNN.png). Serve para nao realimentar o gerador com a
  //! propria saida: depois de Insert as Level a coluna corrente passa a ser a do
  //! PNG gerado, e a referencia sai justamente da coluna corrente.
  bool currentLevelIsGenerated() const;

  //! Frame corrente como PNG em base64. Vazio se nao houver imagem raster.
  QString currentFrameAsPngBase64(QString &errorOut) const;
  //! Folha branca 512x512 em base64, para satisfazer o no de referencia
  //! quando o usuario desligou a influencia dela.
  static QString blankReferencePngBase64();

  ComfyClient *m_client;
  QVector<GraphTemplate> m_templates;
  QImage m_result;

  QComboBox *m_templateCombo;
  QLineEdit *m_promptField;
  QLineEdit *m_negativeField;
  QSpinBox *m_seedField;
  QCheckBox *m_useReferenceCheck;
  QCheckBox *m_liveCheck;
  QTimer *m_debounce;
  LiveTrigger m_lastLiveTrigger = LiveTrigger::Text;
  //! Ligado enquanto onInsertAsLevel() roda. Hoje inserir nao emite o sinal do
  //! gatilho - foi verificado -, mas isso e uma propriedade do IoCmd, nao uma
  //! garantia: se um dia ele passar a notificar, esta flag e o que impede o
  //! painel de se disparar sozinho. Defesa em profundidade, nao correcao.
  bool m_suppressLiveTrigger = false;
  //! Texto do prompt no instante da ultima submissao. Comparar com o campo
  //! atual e o que decide se ainda ha trabalho pendente - nao precisa de um
  //! flag separado, que poderia dessincronizar do texto.
  QString m_submittedPrompt;
  //! Quantas revisoes o desenho corrente acumulou. Avanca uma vez por traco
  //! concluido, no mesmo ponto em que o gatilho por desenho e armado, e so com
  //! o Live ligado - com ele desligado a lambda ja retornou antes.
  //!
  //! Existe porque a coalescencia era chaveada so no texto do prompt, e
  //! desenhando o prompt nao muda entre geracoes: sem isto, desenhar durante uma
  //! geracao em voo seria perdido em silencio. Um serial e nao um hash da
  //! referencia porque comparar hashes exigiria rasterizar e recodificar o
  //! desenho a cada fim de ciclo, no caminho comum aos dois gatilhos, para
  //! responder o que o proprio sinal ja responde. Ver D6 no design.
  quint64 m_drawSerial          = 0;
  //! O valor de m_drawSerial no instante da ultima submissao, do mesmo jeito
  //! que m_submittedPrompt guarda o texto submetido.
  quint64 m_submittedDrawSerial = 0;

  QTimer *m_spinTimer;
  QElapsedTimer m_elapsed;
  int m_spinFrame = 0;
  //! Ultima mensagem vinda do ComfyClient, sem o indicador. Guardada a parte
  //! porque o spinner reescreve o label a cada tique e precisa recompor o
  //! texto do zero, em vez de anexar em cima do que ja esta la.
  QString m_busyMessage;
  QPushButton *m_generateButton;
  QPushButton *m_insertButton;
  QLabel *m_previewLabel;
  QLabel *m_statusLabel;
};

#endif  // AIASSIST_AIASSISTCONTROLLER_H
