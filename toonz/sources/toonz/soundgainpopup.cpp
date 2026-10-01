#include "soundgainpopup.h"

// Tnz includes
#include "tapp.h"
// TnzQt includes
#include "toonzqt/doublefield.h"
// TnzLib includes
#include "toonz/txsheet.h"
#include "toonz/txshcolumn.h"
#include "toonz/txshsoundcolumn.h"
// Qt includes
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>

#include <algorithm>

namespace {

QString formatDb(double db) {
  if (db <= -120.0) return QObject::tr("silence", "SoundGainPopup");
  return QString("%1 dBFS").arg(db, 0, 'f', 1);
}

}  // namespace

//-----------------------------------------------------------------------------

SoundGainPopup::SoundGainPopup(TXsheet *xsh, const std::vector<int> &columns,
                               int r0, int r1)
    : Dialog(TApp::instance()->getMainWindow(), true, true, "SoundGainPopup")
    , m_gainFld(nullptr)
    , m_resultLbl(nullptr)
    , m_peakDb(-120.0)
    , m_hasAudio(false)
    , m_removeRequested(false) {
  setWindowTitle(tr("Adjust Sound Gain"));

  // Measure the raw audio and read the gain already set, column by column.
  // The loudest column decides the readout: it is the one that clips first.
  double rmsDb     = -120.0;
  double currentDb = 0.0;
  bool uniform     = true;
  bool first       = true;
  for (int c : columns) {
    TXshColumn *column = xsh->getColumn(c);
    TXshSoundColumn *sc = column ? column->getSoundColumn() : nullptr;
    if (!sc) continue;
    double peak, rms;
    if (sc->measureRows(r0, r1, peak, rms)) {
      m_peakDb   = m_hasAudio ? std::max(m_peakDb, peak) : peak;
      rmsDb      = m_hasAudio ? std::max(rmsDb, rms) : rms;
      m_hasAudio = true;
    }
    for (int r = r0; r <= r1; ++r) {
      if (sc->isCellEmpty(r)) continue;
      double g = sc->getGainDbAtRow(r);
      if (first) {
        currentDb = g;
        first     = false;
      } else if (g != currentDb)
        uniform = false;
    }
  }

  m_gainFld = new DVGui::DoubleLineEdit(this, uniform ? currentDb : 0.0);
  m_gainFld->setRange(-60.0, 24.0);
  m_gainFld->setDecimals(1);
  m_gainFld->setObjectName("LargeSizedText");

  QString scope = (r0 == r1) ? tr("Row %1").arg(r0 + 1)
                             : tr("Rows %1 to %2").arg(r0 + 1).arg(r1 + 1);
  if (columns.size() > 1)
    scope += tr(" in %1 sound columns").arg((int)columns.size());
  QLabel *scopeLbl = new QLabel(scope, this);

  QLabel *measureLbl = new QLabel(
      m_hasAudio ? tr("Before gain: peak %1, RMS %2")
                       .arg(formatDb(m_peakDb))
                       .arg(formatDb(rmsDb))
                 : tr("No audio under the selected rows."),
      this);
  QLabel *currentLbl = new QLabel(
      uniform ? tr("Current gain: %1 dB").arg(currentDb, 0, 'f', 1)
              : tr("Current gain: mixed"),
      this);
  m_resultLbl = new QLabel(this);

  QGridLayout *lay = new QGridLayout();
  lay->setContentsMargins(0, 0, 0, 0);
  lay->setSpacing(5);
  {
    lay->addWidget(scopeLbl, 0, 0, 1, 3);
    lay->addWidget(measureLbl, 1, 0, 1, 3);
    lay->addWidget(currentLbl, 2, 0, 1, 3);
    lay->addWidget(new QLabel(tr("Gain:"), this), 3, 0,
                   Qt::AlignRight | Qt::AlignVCenter);
    lay->addWidget(m_gainFld, 3, 1);
    lay->addWidget(new QLabel(tr("dB"), this), 3, 2);
    lay->addWidget(m_resultLbl, 4, 0, 1, 3);
  }
  m_topLayout->addLayout(lay);

  QPushButton *applyBtn  = new QPushButton(tr("Apply"), this);
  QPushButton *removeBtn = new QPushButton(tr("Remove Gain"), this);
  QPushButton *cancelBtn = new QPushButton(tr("Cancel"), this);
  applyBtn->setDefault(true);
  m_buttonLayout->addWidget(applyBtn);
  m_buttonLayout->addWidget(removeBtn);
  m_buttonLayout->addWidget(cancelBtn);

  connect(m_gainFld, &QLineEdit::textChanged, this,
          &SoundGainPopup::onGainEdited);
  connect(applyBtn, &QPushButton::clicked, this, &QDialog::accept);
  connect(removeBtn, &QPushButton::clicked, this, &SoundGainPopup::onRemove);
  connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

  onGainEdited();
}

//-----------------------------------------------------------------------------

double SoundGainPopup::getGainDb() const {
  return m_removeRequested ? 0.0 : m_gainFld->getValue();
}

//-----------------------------------------------------------------------------

void SoundGainPopup::onGainEdited() {
  if (!m_hasAudio) {
    m_resultLbl->clear();
    return;
  }
  double after = m_peakDb + m_gainFld->getValue();
  if (after > 0.0) {
    m_resultLbl->setText(
        tr("After gain: peak %1. The loudest samples will clip.")
            .arg(formatDb(after)));
    m_resultLbl->setStyleSheet("color: #e08030;");
  } else {
    m_resultLbl->setText(tr("After gain: peak %1").arg(formatDb(after)));
    m_resultLbl->setStyleSheet("");
  }
}

//-----------------------------------------------------------------------------

void SoundGainPopup::onRemove() {
  m_removeRequested = true;
  accept();
}
