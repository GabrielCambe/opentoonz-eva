#pragma once
#ifndef AIASSIST_COMFYCLIENT_H
#define AIASSIST_COMFYCLIENT_H

#include <QImage>
#include <QJsonObject>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

//! Cliente HTTP minimo do ComfyUI.
//!
//! Ciclo: POST /prompt -> polling GET /history/<id> -> GET /view.
//! Nao gerencia o servidor e nao assume onde ele esta: a URL vem de fora.
//! Falha de conexao emite failed() com a URL na mensagem — nunca retry silencioso.
class ComfyClient final : public QObject {
  Q_OBJECT

public:
  explicit ComfyClient(QObject *parent = nullptr);
  ~ComfyClient() override;

  void setBaseUrl(const QString &url) { m_baseUrl = url; }
  QString baseUrl() const { return m_baseUrl; }
  bool isBusy() const { return m_busy; }

  //! Submete o grafo. outputNodeId e o no marcado AI_OUTPUT.
  void generate(const QJsonObject &graph, const QString &outputNodeId);

  //! Cancela a execucao em voo no servidor.
  void interrupt();

signals:
  void status(const QString &message);
  void imageReady(const QImage &image);
  void failed(const QString &message);

private slots:
  void onSubmitFinished();
  void pollHistory();
  void onHistoryFinished();
  void onImageFinished();

private:
  void fail(const QString &message);
  void finish();

  QNetworkAccessManager *m_nam;
  QTimer *m_pollTimer;
  QString m_baseUrl;
  QString m_promptId;
  QString m_outputNodeId;
  int m_elapsedMs = 0;
  bool m_busy     = false;

  //! Contador de ciclo. Toda requisicao HTTP carrega o valor vigente na
  //! propriedade "aiGeneration"; os handlers descartam resposta cujo valor nao
  //! seja mais o atual. Sem isto, um /view em voo de um ciclo interrompido
  //! poderia entregar uma imagem velha por cima de uma nova - o resultado
  //! "voltando no tempo" na tela.
  quint64 m_generation = 0;
};

#endif  // AIASSIST_COMFYCLIENT_H
