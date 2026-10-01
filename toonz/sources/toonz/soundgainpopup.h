#pragma once

#ifndef SOUNDGAINPOPUP_H
#define SOUNDGAINPOPUP_H

#include "toonzqt/dvdialog.h"

#include <vector>

class TXsheet;
class QLabel;

namespace DVGui {
class DoubleLineEdit;
}

//=============================================================================
// SoundGainPopup
//-----------------------------------------------------------------------------

//! Asks for a gain, in dB, to set on the rows selected in one or more sound
//! columns. The audio under the rows is measured first, so the user sees the
//! peak before the gain and whether the chosen value would push it past full
//! scale.
class SoundGainPopup final : public DVGui::Dialog {
  Q_OBJECT

  DVGui::DoubleLineEdit *m_gainFld;
  QLabel *m_resultLbl;
  double m_peakDb;
  bool m_hasAudio;
  bool m_removeRequested;

public:
  SoundGainPopup(TXsheet *xsh, const std::vector<int> &columns, int r0, int r1);

  //! 0 when "Remove Gain" was pressed, otherwise the entered value.
  double getGainDb() const;

protected slots:
  void onGainEdited();
  void onRemove();
};

#endif  // SOUNDGAINPOPUP_H
