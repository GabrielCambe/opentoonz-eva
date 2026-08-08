#pragma once

#ifndef INFINITEBACKGROUNDPANEL_H
#define INFINITEBACKGROUNDPANEL_H

#include <QWidget>
#include <QImage>
#include <QPoint>

class InfiniteBackgroundWidget final : public QWidget {
  Q_OBJECT

  QImage m_image;
  QPoint m_lastMousePos;
  double m_zoom;
  QPointF m_pan;

  void drawCameraMask(QPainter &p);

public:
  InfiniteBackgroundWidget(QWidget *parent = nullptr);
  ~InfiniteBackgroundWidget() = default;

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

public slots:
  void exportAndImport();
};

#endif // INFINITEBACKGROUNDPANEL_H
