#include "graphtemplate.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

namespace {
const char *kRequired[] = {"AI_PROMPT", "AI_SAMPLER", "AI_OUTPUT"};
}

//-----------------------------------------------------------------------------

bool GraphTemplate::index(QString &errorOut) {
  m_nodeByTitle.clear();
  for (auto it = m_graph.begin(); it != m_graph.end(); ++it) {
    const QJsonObject node = it.value().toObject();
    const QString title    = node.value(QStringLiteral("_meta"))
                              .toObject()
                              .value(QStringLiteral("title"))
                              .toString();
    if (!title.startsWith(QStringLiteral("AI_"))) continue;
    if (m_nodeByTitle.contains(title)) {
      errorOut = QObject::tr("duplicated title %1").arg(title);
      return false;
    }
    m_nodeByTitle.insert(title, it.key());
  }
  QStringList missing;
  for (const char *req : kRequired)
    if (!m_nodeByTitle.contains(QString::fromLatin1(req)))
      missing << QString::fromLatin1(req);
  if (!missing.isEmpty()) {
    errorOut =
        QObject::tr("missing %1").arg(missing.join(QStringLiteral(", ")));
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------

bool GraphTemplate::loadFromFile(const QString &path, GraphTemplate &out,
                                 QString &errorOut) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    errorOut = QObject::tr("cannot open file");
    return false;
  }
  QJsonParseError perr;
  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
  if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
    errorOut = QObject::tr("invalid JSON: %1").arg(perr.errorString());
    return false;
  }
  out.m_graph = doc.object();
  out.m_path  = path;
  out.m_name  = QFileInfo(path).completeBaseName();
  return out.index(errorOut);
}

//-----------------------------------------------------------------------------

QVector<GraphTemplate> GraphTemplate::scanFolder(const QString &folder,
                                                 QStringList &rejectedOut) {
  QVector<GraphTemplate> result;
  QDir dir(folder);
  const QStringList files = dir.entryList(
      QStringList() << QStringLiteral("*.json"), QDir::Files, QDir::Name);
  for (const QString &file : files) {
    GraphTemplate tpl;
    QString err;
    const QString full = dir.absoluteFilePath(file);
    if (GraphTemplate::loadFromFile(full, tpl, err))
      result.push_back(tpl);
    else
      rejectedOut << QStringLiteral("%1: %2").arg(file, err);
  }
  return result;
}

//-----------------------------------------------------------------------------

bool GraphTemplate::referenceScaleTarget(int &widthOut, int &heightOut) const {
  QString scaleId = m_nodeByTitle.value(QStringLiteral("AI_SCALE"));

  if (scaleId.isEmpty()) {
    // Sem marcador explicito, aceita um unico ImageScale. Com dois ou mais nao
    // da para saber qual esta na cadeia da referencia sem percorrer os links,
    // e uma linha de status errada e pior que nenhuma.
    for (auto it = m_graph.constBegin(); it != m_graph.constEnd(); ++it) {
      const QString cls =
          it.value().toObject().value(QStringLiteral("class_type")).toString();
      if (cls != QLatin1String("ImageScale")) continue;
      if (!scaleId.isEmpty()) return false;  // ambiguo
      scaleId = it.key();
    }
  }
  if (scaleId.isEmpty()) return false;

  const QJsonObject inputs = m_graph.value(scaleId)
                                 .toObject()
                                 .value(QStringLiteral("inputs"))
                                 .toObject();
  const QJsonValue w = inputs.value(QStringLiteral("width"));
  const QJsonValue h = inputs.value(QStringLiteral("height"));
  if (!w.isDouble() || !h.isDouble()) return false;  // veio por link

  widthOut  = w.toInt();
  heightOut = h.toInt();
  return widthOut > 0 && heightOut > 0;
}

//-----------------------------------------------------------------------------

int GraphTemplate::samplerSteps() const {
  const QString samplerId = m_nodeByTitle.value(QStringLiteral("AI_SAMPLER"));
  if (samplerId.isEmpty()) return 0;
  const QJsonValue steps = m_graph.value(samplerId)
                               .toObject()
                               .value(QStringLiteral("inputs"))
                               .toObject()
                               .value(QStringLiteral("steps"));
  return steps.isDouble() ? steps.toInt() : 0;
}

//-----------------------------------------------------------------------------

double GraphTemplate::samplerCfg() const {
  const QString samplerId = m_nodeByTitle.value(QStringLiteral("AI_SAMPLER"));
  if (samplerId.isEmpty()) return 1.0;
  const QJsonValue cfg = m_graph.value(samplerId)
                             .toObject()
                             .value(QStringLiteral("inputs"))
                             .toObject()
                             .value(QStringLiteral("cfg"));
  // Um cfg vindo por link ([node, slot]) e um array, nao um numero: nesse caso
  // o valor so existe em tempo de execucao no servidor e nao da para decidir
  // aqui. toDouble(1.0) devolve o default e o campo fica desabilitado.
  return cfg.toDouble(1.0);
}

//-----------------------------------------------------------------------------

QJsonObject GraphTemplate::build(const QString &prompt, const QString &negative,
                                 const QString &referenceBase64, qint64 seed,
                                 bool useReference) const {
  QJsonObject graph = m_graph;  // copia; o template original fica intacto

  auto setInput = [&graph](const QString &nodeId, const QString &key,
                           const QJsonValue &value) {
    QJsonObject node   = graph.value(nodeId).toObject();
    QJsonObject inputs = node.value(QStringLiteral("inputs")).toObject();
    inputs.insert(key, value);
    node.insert(QStringLiteral("inputs"), inputs);
    graph.insert(nodeId, node);
  };

  setInput(m_nodeByTitle.value(QStringLiteral("AI_PROMPT")),
           QStringLiteral("text"), prompt);
  setInput(m_nodeByTitle.value(QStringLiteral("AI_SAMPLER")),
           QStringLiteral("seed"), QJsonValue(seed));
  if (hasNegativePrompt())
    setInput(m_nodeByTitle.value(QStringLiteral("AI_NEGATIVE")),
             QStringLiteral("text"), negative);
  if (consumesReference()) {
    // O no continua recebendo imagem mesmo com a referencia desligada: e um
    // LoadImageFromBase64 e o servidor recusa data vazio. Quem desliga a
    // influencia sao as duas linhas abaixo, nao a ausencia da imagem.
    setInput(m_nodeByTitle.value(QStringLiteral("AI_REFERENCE")),
             QStringLiteral("data"), referenceBase64);

    if (!useReference) {
      if (hasControlNode())
        setInput(m_nodeByTitle.value(QStringLiteral("AI_CONTROL")),
                 QStringLiteral("strength"), QJsonValue(0.0));
      setInput(m_nodeByTitle.value(QStringLiteral("AI_SAMPLER")),
               QStringLiteral("denoise"), QJsonValue(1.0));
    }
  }

  return graph;
}
