#include "comfyclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

namespace {
const int kPollIntervalMs = 500;
const int kTimeoutMs      = 300000;  // 5 min
}

//-----------------------------------------------------------------------------

ComfyClient::ComfyClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_pollTimer(new QTimer(this))
    , m_baseUrl(QStringLiteral("http://127.0.0.1:8188")) {
  m_pollTimer->setInterval(kPollIntervalMs);
  connect(m_pollTimer, &QTimer::timeout, this, &ComfyClient::pollHistory);
}

ComfyClient::~ComfyClient() {}

//-----------------------------------------------------------------------------

void ComfyClient::fail(const QString &message) {
  finish();
  emit failed(message);
}

void ComfyClient::finish() {
  ++m_generation;  // invalida respostas ainda em voo deste ciclo
  m_pollTimer->stop();
  m_busy      = false;
  m_promptId.clear();
  m_elapsedMs = 0;
}

//-----------------------------------------------------------------------------

void ComfyClient::generate(const QJsonObject &graph,
                           const QString &outputNodeId) {
  if (m_busy) {
    emit status(tr("Busy; ignoring request."));
    return;
  }
  ++m_generation;
  m_busy         = true;
  m_outputNodeId = outputNodeId;
  m_elapsedMs    = 0;

  QJsonObject body;
  body.insert(QStringLiteral("prompt"), graph);
  body.insert(QStringLiteral("client_id"),
              QUuid::createUuid().toString(QUuid::WithoutBraces));

  QNetworkRequest req(QUrl(m_baseUrl + QStringLiteral("/prompt")));
  req.setHeader(QNetworkRequest::ContentTypeHeader,
                QStringLiteral("application/json"));

  emit status(tr("Submitting..."));
  QNetworkReply *reply =
      m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
  reply->setProperty("aiGeneration", m_generation);
  connect(reply, &QNetworkReply::finished, this,
          &ComfyClient::onSubmitFinished);
}

//-----------------------------------------------------------------------------

void ComfyClient::onSubmitFinished() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) return;
  reply->deleteLater();
  // Resposta de um ciclo que ja nao vale (interrompido ou substituido).
  if (reply->property("aiGeneration").toULongLong() != m_generation) return;

  if (reply->error() != QNetworkReply::NoError) {
    const QVariant code =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (code.isValid())
      // O servidor respondeu e recusou o grafo: o corpo diz qual no falhou.
      fail(tr("ComfyUI rejected the graph (HTTP %1): %2")
               .arg(code.toInt())
               .arg(QString::fromUtf8(reply->readAll().left(600))));
    else
      // Nao alcancou o servidor. Mostrar a URL, nao um spinner eterno.
      fail(tr("ComfyUI not found at %1 (%2)")
               .arg(m_baseUrl, reply->errorString()));
    return;
  }

  const QJsonObject obj =
      QJsonDocument::fromJson(reply->readAll()).object();
  m_promptId = obj.value(QStringLiteral("prompt_id")).toString();
  if (m_promptId.isEmpty()) {
    fail(tr("ComfyUI did not return a prompt_id."));
    return;
  }
  emit status(tr("Generating..."));
  m_pollTimer->start();
}

//-----------------------------------------------------------------------------

void ComfyClient::pollHistory() {
  m_elapsedMs += kPollIntervalMs;
  if (m_elapsedMs > kTimeoutMs) {
    fail(tr("No result for %1 after %2 s.")
             .arg(m_promptId)
             .arg(kTimeoutMs / 1000));
    return;
  }
  QNetworkReply *reply = m_nam->get(QNetworkRequest(
      QUrl(m_baseUrl + QStringLiteral("/history/") + m_promptId)));
  reply->setProperty("aiGeneration", m_generation);
  connect(reply, &QNetworkReply::finished, this,
          &ComfyClient::onHistoryFinished);
}

//-----------------------------------------------------------------------------

void ComfyClient::onHistoryFinished() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) return;
  reply->deleteLater();
  // Resposta de um ciclo que ja nao vale (interrompido ou substituido).
  if (reply->property("aiGeneration").toULongLong() != m_generation) return;
  if (reply->error() != QNetworkReply::NoError) return;  // tenta de novo

  const QJsonObject hist = QJsonDocument::fromJson(reply->readAll()).object();
  const QJsonObject entry = hist.value(m_promptId).toObject();
  if (entry.isEmpty()) return;

  const QJsonObject st = entry.value(QStringLiteral("status")).toObject();
  if (st.value(QStringLiteral("status_str")).toString() ==
      QStringLiteral("error")) {
    fail(tr("Execution failed: %1")
             .arg(QString::fromUtf8(
                 QJsonDocument(st).toJson(QJsonDocument::Compact).left(600))));
    return;
  }
  if (!st.value(QStringLiteral("completed")).toBool()) return;

  m_pollTimer->stop();

  const QJsonArray images = entry.value(QStringLiteral("outputs"))
                                .toObject()
                                .value(m_outputNodeId)
                                .toObject()
                                .value(QStringLiteral("images"))
                                .toArray();
  if (images.isEmpty()) {
    fail(tr("Node %1 produced no image. Check the template's AI_OUTPUT.")
             .arg(m_outputNodeId));
    return;
  }

  const QJsonObject ref = images.at(0).toObject();
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("filename"),
                 ref.value(QStringLiteral("filename")).toString());
  q.addQueryItem(QStringLiteral("subfolder"),
                 ref.value(QStringLiteral("subfolder")).toString());
  q.addQueryItem(QStringLiteral("type"),
                 ref.value(QStringLiteral("type")).toString(
                     QStringLiteral("temp")));
  QUrl url(m_baseUrl + QStringLiteral("/view"));
  url.setQuery(q);

  emit status(tr("Downloading image..."));
  QNetworkReply *img = m_nam->get(QNetworkRequest(url));
  img->setProperty("aiGeneration", m_generation);
  connect(img, &QNetworkReply::finished, this, &ComfyClient::onImageFinished);
}

//-----------------------------------------------------------------------------

void ComfyClient::onImageFinished() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) return;
  reply->deleteLater();
  // Resposta de um ciclo que ja nao vale (interrompido ou substituido).
  if (reply->property("aiGeneration").toULongLong() != m_generation) return;

  if (reply->error() != QNetworkReply::NoError) {
    fail(tr("Failed to download the image: %1").arg(reply->errorString()));
    return;
  }
  QImage image;
  if (!image.loadFromData(reply->readAll())) {
    fail(tr("ComfyUI returned data that is not a readable image."));
    return;
  }
  finish();
  emit status(tr("Done."));
  emit imageReady(image);
}

//-----------------------------------------------------------------------------

void ComfyClient::interrupt() {
  if (!m_busy) return;
  QNetworkRequest req(QUrl(m_baseUrl + QStringLiteral("/interrupt")));
  req.setHeader(QNetworkRequest::ContentTypeHeader,
                QStringLiteral("application/json"));
  QNetworkReply *reply = m_nam->post(req, QByteArray("{}"));
  connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
  finish();
  emit status(tr("Interrupted."));
}
