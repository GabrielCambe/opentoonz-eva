#include "infinitebackgroundpanel.h"
#include "tapp.h"
#include "toonz/tcamera.h"
#include "toonz/tscenehandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/toonzscene.h"
#include "toonz/tproject.h"
#include "tpanels.h"
#include "iocommand.h"
#include "mainwindow.h"
#include "menubarcommandids.h"
#include "floatingpanelcommand.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QMouseEvent>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

//=============================================================================
// InfiniteBackgroundWidget
//-----------------------------------------------------------------------------

InfiniteBackgroundWidget::InfiniteBackgroundWidget(QWidget *parent)
    : QWidget(parent)
    , m_image(10000, 10000, QImage::Format_ARGB32_Premultiplied)
    , m_zoom(1.0)
    , m_pan(0, 0) {
  
  m_image.fill(Qt::white);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  QPushButton *exportBtn = new QPushButton(tr("Export and Auto-Import to Scene"), this);
  connect(exportBtn, SIGNAL(clicked()), this, SLOT(exportAndImport()));
  layout->addWidget(exportBtn, 0, Qt::AlignTop);
  
  setMouseTracking(true);
}

void InfiniteBackgroundWidget::drawCameraMask(QPainter &p) {
  TApp *app = TApp::instance();
  if (!app || !app->getCurrentScene() || !app->getCurrentScene()->getScene())
    return;
    
  TCamera *camera = app->getCurrentScene()->getScene()->getCurrentCamera();
  if (!camera) return;

  TDimension res = camera->getRes();
  
  p.save();
  p.setPen(QPen(Qt::red, 2));
  p.setBrush(Qt::NoBrush);
  
  // Center camera in the 10000x10000 image
  QRectF cameraRect(m_image.width() / 2.0 - res.lx / 2.0,
                    m_image.height() / 2.0 - res.ly / 2.0,
                    res.lx, res.ly);
  
  p.drawRect(cameraRect);
  p.restore();
}

void InfiniteBackgroundWidget::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.fillRect(rect(), Qt::gray);
  
  p.translate(width() / 2.0, height() / 2.0);
  p.scale(m_zoom, m_zoom);
  p.translate(-m_image.width() / 2.0 + m_pan.x(), -m_image.height() / 2.0 + m_pan.y());
  
  p.drawImage(0, 0, m_image);
  
  drawCameraMask(p);
}

void InfiniteBackgroundWidget::mousePressEvent(QMouseEvent *event) {
  m_lastMousePos = event->pos();
}

void InfiniteBackgroundWidget::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::MiddleButton) {
    QPointF delta = event->pos() - m_lastMousePos;
    m_pan += delta / m_zoom;
    m_lastMousePos = event->pos();
    update();
  } else if (event->buttons() & Qt::LeftButton) {
    QPointF currentCanvasPos = (event->pos() - QPointF(width() / 2.0, height() / 2.0)) / m_zoom 
                               + QPointF(m_image.width() / 2.0, m_image.height() / 2.0) - m_pan;
    QPointF lastCanvasPos = (m_lastMousePos - QPointF(width() / 2.0, height() / 2.0)) / m_zoom 
                               + QPointF(m_image.width() / 2.0, m_image.height() / 2.0) - m_pan;
                               
    QPainter imgPainter(&m_image);
    imgPainter.setPen(QPen(Qt::black, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    imgPainter.drawLine(lastCanvasPos, currentCanvasPos);
    
    m_lastMousePos = event->pos();
    update();
  }
}

void InfiniteBackgroundWidget::mouseReleaseEvent(QMouseEvent *event) {
  // Do nothing
}

void InfiniteBackgroundWidget::wheelEvent(QWheelEvent *event) {
  double delta = event->angleDelta().y();
  if (delta > 0)
    m_zoom *= 1.1;
  else
    m_zoom /= 1.1;
  update();
}

void InfiniteBackgroundWidget::exportAndImport() {
  QString filters = "PNG (*.png);;TIFF (*.tif)";
  QString path = QFileDialog::getSaveFileName(this, tr("Export Background"), "", filters);
  
  if (path.isEmpty()) return;
  
  m_image.save(path);
  
  IoCmd::LoadResourceArguments args(TFilePath(path.toStdWString()));
  args.expose = true;
  IoCmd::loadResources(args);
}

//=============================================================================
// InfiniteBackgroundPanelFactory
//-----------------------------------------------------------------------------

class InfiniteBackgroundPanelFactory final : public TPanelFactory {
public:
  InfiniteBackgroundPanelFactory() : TPanelFactory("InfiniteBackgroundPanel") {}

  void initialize(TPanel *panel) override {
    InfiniteBackgroundWidget *widget = new InfiniteBackgroundWidget(panel);
    panel->setWidget(widget);
    panel->setWindowTitle(QObject::tr("Infinite Background"));
    panel->setIsMaximizable(true);
  }
} infiniteBackgroundPanelFactory;

OpenFloatingPanel openInfiniteBackgroundPanelCommand(
    "MI_OpenInfiniteBackgroundPanel", "InfiniteBackgroundPanel",
    QObject::tr("Infinite Background"));
