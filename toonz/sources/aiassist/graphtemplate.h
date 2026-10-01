#pragma once
#ifndef AIASSIST_GRAPHTEMPLATE_H
#define AIASSIST_GRAPHTEMPLATE_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

//! Um template de grafo do ComfyUI em formato API.
//!
//! O painel nunca constroi grafo: carrega JSON exportado pelo ComfyUI em
//! "Save (API Format)" e escreve em nos identificados por _meta.title.
//! Contrato completo em tasks/knowledge/aiassist__comfyui_protocol.md.
class GraphTemplate {
public:
  GraphTemplate() {}

  //! Carrega e valida. Em caso de falha retorna false e preenche errorOut.
  static bool loadFromFile(const QString &path, GraphTemplate &out,
                           QString &errorOut);

  //! Varre um diretorio; templates invalidos sao descartados e reportados.
  static QVector<GraphTemplate> scanFolder(const QString &folder,
                                           QStringList &rejectedOut);

  bool isValid() const { return !m_nodeByTitle.isEmpty(); }
  QString name() const { return m_name; }
  QString path() const { return m_path; }

  //! Um template sem AI_REFERENCE nao consome o frame: e txt2img.
  bool consumesReference() const {
    return m_nodeByTitle.contains(QStringLiteral("AI_REFERENCE"));
  }

  //! O grafo tem um ControlNetApply marcado, cuja forca da para zerar.
  bool hasControlNode() const {
    return m_nodeByTitle.contains(QStringLiteral("AI_CONTROL"));
  }

  //! Tamanho para o qual o grafo redimensiona a referencia antes de usa-la.
  //! Procura primeiro um no marcado AI_SCALE; se nao houver, aceita um unico
  //! ImageScale no grafo. Retorna false quando nao da para saber com certeza -
  //! nenhum no, mais de um, ou largura/altura vindas por link em vez de
  //! literal. Nesse caso o painel prefere calar a chutar.
  bool referenceScaleTarget(int &widthOut, int &heightOut) const;
  //! O no AI_NEGATIVE existe no grafo.
  bool hasNegativePrompt() const {
    return m_nodeByTitle.contains(QStringLiteral("AI_NEGATIVE"));
  }

  //! O prompt negativo so tem efeito se o classifier-free guidance estiver
  //! ativo. O sampler calcula ruido = uncond + cfg * (cond - uncond); com
  //! cfg = 1 isso vira ruido = cond e o termo negativo se cancela. Grafos LCM
  //! rodam em cfg 1.0 por construcao, entao neles o campo e inutil.
  bool negativeHasEffect() const {
    return hasNegativePrompt() && samplerCfg() > 1.0;
  }

  //! cfg do no AI_SAMPLER. Retorna 1.0 se ausente ou nao numerico, que e a
  //! leitura conservadora: sem cfg conhecido, assume que o negativo nao pega.
  double samplerCfg() const;

  //! steps do no AI_SAMPLER. Zero quando ausente ou vindo por link.
  int samplerSteps() const;

  //! Se o grafo e rapido o bastante para regenerar a cada pausa na
  //! digitacao. O criterio e o numero de passos: um grafo LCM roda 4 e
  //! responde em fracao de segundo; um grafo de guidance normal roda 20 ou
  //! mais e levaria segundos, o que transforma digitar em espera. O limite
  //! de 8 e a fronteira pratica da familia de modelos destilados.
  bool isLiveCapable() const {
    const int steps = samplerSteps();
    return steps > 0 && steps <= 8;
  }
  QString outputNodeId() const {
    return m_nodeByTitle.value(QStringLiteral("AI_OUTPUT"));
  }

  //! Aplica o patch e devolve o grafo pronto para POST /prompt.
  //!
  //! useReference so tem sentido em grafo com AI_REFERENCE. Com false, a
  //! referencia nao e removida do grafo - nao da, os nos sao estruturais e o
  //! servidor recusaria um LoadImageFromBase64 vazio. Em vez disso ela e
  //! neutralizada: AI_CONTROL.strength vai a 0 e AI_SAMPLER.denoise a 1.0, o
  //! que faz o ControlNet nao condicionar nada e o latente da referencia ser
  //! inteiramente substituido por ruido. O resultado e txt2img de verdade,
  //! saindo do mesmo grafo.
  QJsonObject build(const QString &prompt, const QString &negative,
                    const QString &referenceBase64, qint64 seed,
                    bool useReference = true) const;

private:
  bool index(QString &errorOut);

  QJsonObject m_graph;
  QHash<QString, QString> m_nodeByTitle;  // titulo AI_* -> node id
  QString m_name;
  QString m_path;
};

#endif  // AIASSIST_GRAPHTEMPLATE_H
