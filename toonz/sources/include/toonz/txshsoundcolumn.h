#pragma once

#ifndef TXSHSOUNDCOLUMN_INCLUDED
#define TXSHSOUNDCOLUMN_INCLUDED

#include "toonz/txshcolumn.h"
#include "toonz/txshcell.h"
#include "toonz/txshsoundlevel.h"
#include "tsound.h"
#include "tsop.h"

#include <QList>
#include <QTimer>
#include <vector>

#undef DVAPI
#undef DVVAR
#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

//=============================================================================
// forward declarations
class TFilePath;

//=============================================================================
//  SoundGainSection
//=============================================================================

/*! A stretch of a sound clip whose level is changed by m_gainDb when the
    clip is mixed. Frames are clip-local, [m_startFrame, m_endFrame) with
    frame 0 the first frame of the audio file, so trimming or moving the clip
    keeps the gain attached to the same audio. The source file is never
    modified. */
struct SoundGainSection {
  int m_startFrame;
  int m_endFrame;
  double m_gainDb;
};

//=============================================================================
//  SoundGainOmitScope
//=============================================================================

/*! While one of these is alive, ColumnLevel::saveData leaves the gain
    sections out of the stream, so the file opens in builds that do not know
    the tag. The sections stay in memory and are written again by any save
    made after the scope ends. */
class DVAPI SoundGainOmitScope {
public:
  SoundGainOmitScope();
  ~SoundGainOmitScope();
  static bool isActive();
};

//=============================================================================
//  ColumnLevel
//=============================================================================

class ColumnLevel {
  TXshSoundLevelP m_soundLevel;

  //! Sorted and non-overlapping; see SoundGainSection.
  std::vector<SoundGainSection> m_gainSections;

  /*!Offsets: in frames. Start offset is a positive number.*/
  int m_startOffset;
  /*!Offsets: in frames. End offset is a positive number(to subtract to..).*/
  int m_endOffset;

  //! Starting frame in the timeline
  int m_startFrame;

  //! frameRate
  double m_fps;

public:
  ColumnLevel(TXshSoundLevel *soundLevel = 0, int startFrame = -1,
              int startOffset = -1, int endOffset = -1, double fps = -1);
  ~ColumnLevel();
  ColumnLevel *clone() const;

  //! Overridden from TXshLevel
  TXshSoundLevel *getSoundLevel() const { return m_soundLevel.getPointer(); }
  void setSoundLevel(TXshSoundLevelP level) { m_soundLevel = level; }

  void loadData(TIStream &is);
  void saveData(TOStream &os);

  void setStartOffset(int value);
  int getStartOffset() const { return m_startOffset; }

  void setEndOffset(int value);
  int getEndOffset() const { return m_endOffset; }

  void setOffsets(int startOffset, int endOffset);

  //! Return the starting frame without offsets.
  void setStartFrame(int frame) { m_startFrame = frame; }
  int getStartFrame() const { return m_startFrame; }

  //! Return the ending frame without offsets.
  int getEndFrame() const;

  //! Return frame count without offset.
  int getFrameCount() const;

  //! Return frame count with offset.
  int getVisibleFrameCount() const;

  //! Return start frame with offset.
  int getVisibleStartFrame() const;
  //! Return last frame with offset.
  int getVisibleEndFrame() const;
  //! Updates m_startOfset and m_endOffset.
  void updateFrameRate(double newFrameRate);

  void setFrameRate(double fps) { m_fps = fps; }

  const std::vector<SoundGainSection> &getGainSections() const {
    return m_gainSections;
  }
  //! Replaces every section; the list is sorted and merged on the way in.
  void setGainSections(const std::vector<SoundGainSection> &sections);
  //! Sets [startFrame, endFrame) to gainDb, cutting whatever sections were
  //! there. A gain of 0 dB removes the stretch instead of storing it.
  void setGain(int startFrame, int endFrame, double gainDb);
  //! Gain at a clip-local frame; 0 outside every section.
  double getGainDbAt(int frame) const;
  //! The ranges TSop::gain must scale when the clip's samples [s0, s1] are
  //! played, expressed as offsets inside that extract.
  std::vector<TSop::GainRange> getGainRanges(TINT32 s0, TINT32 s1,
                                             double samplePerFrame) const;
};

//=============================================================================
//! The TXshSoundColumn class provides a sound column in xsheet and allows its
//! management through cell concept.
/*!Inherits \b TXshCellColumn. */
//=============================================================================

class DVAPI TXshSoundColumn final : public QObject, public TXshCellColumn {
  Q_OBJECT

  PERSIST_DECLARATION(TXshSoundColumn)
  TSoundOutputDevice *m_player;

  QList<ColumnLevel *> m_levels;  // owner

  /*!Used to menage current playback.*/
  TSoundTrackP m_currentPlaySoundTrack;

  //! From 0.0 to 1.0
  double m_volume;
  bool m_isOldVersion;

  QTimer m_timer;

public:
  TXshSoundColumn();
  ~TXshSoundColumn();

  TXshColumn::ColumnType getColumnType() const override;
  TXshSoundColumn *getSoundColumn() override { return this; }

  bool canSetCell(const TXshCell &cell) const override;

  TXshColumn *clone() const override;

  /*! Clear column and set src level. */
  void assignLevels(const TXshSoundColumn *src);

  void loadData(TIStream &is) override;
  void saveData(TOStream &os) override;

  /*! r0 : min row not empty, r1 : max row not empty. Return row count.*/
  int getRange(int &r0, int &r1) const override;
  /*! Last not empty row - first not empty row. */
  int getRowCount() const override;
  /*! Return max row not empty. */
  int getMaxFrame() const override;
  /*! Return min row not empty.*/
  int getFirstRow() const override;

  const TXshCell &getCell(int row) const override;
  TXshCell getSoundCell(int row);
  void getCells(int row, int rowCount, TXshCell cells[]) override;

  bool setCell(int row, const TXshCell &cell) override;
  /*! Return false if cannot set cells.*/
  bool setCells(int row, int rowCount, const TXshCell cells[]) override;

  void insertEmptyCells(int row, int rowCount) override;

  void clearCells(int row, int rowCount) override;
  void removeCells(int row, int rowCount) override;

  /*! Check if frames from \b row to \b row+rowCount are in sequence and
   * collapse level if it is true. */
  void updateCells(int row, int rowCount);

  /*! Modify range of level sound in row. Return new range value of the level in
row.
N.B. Row must be the first or last cell of a sound level. */
  int modifyCellRange(int row, int delta, bool modifyStartValue);

  bool isCellEmpty(int row) const override;
  /*! r0 : min row not empty of level in row, r1 : max row not empty of level in
row.
Return true if level range is not empty.*/
  bool getLevelRange(int row, int &r0, int &r1) const override;
  /*! r0 : min possible (without offset) row of level in row, r1 : max possible
(without offset) row of level in row.
Return true if level range is not empty.*/
  bool getLevelRangeWithoutOffset(int row, int &r0, int &r1) const;

  /*! Only debug. */
  void checkColumn() const override;

  void setXsheet(TXsheet *xsheet) override;
  void setFrameRate(double fps);
  void updateFrameRate(double fps);

  //! From 0.0 to 1.0
  void setVolume(double value);
  double getVolume() const;

  TSoundTrackP getCurrentPlaySoundTruck() { return m_currentPlaySoundTrack; }

  //! s0 and s1 are samples
  void play(TSoundTrackP soundtrack, int s0, int s1, bool loop);
  /*! Play the whole soundSequence, currentFrame it is used to compute an offset
when the user play a single level and hence the audio behind..*/
  void play(ColumnLevel *ss, int currentFrame);
  void play(int currentFrame = 0);
  void stop();

  bool isPlaying() const;

  void scrub(int fromFrame, int toFrame);

  TSoundTrackP getOverallSoundTrack(
      int fromFrame = -1, int toFram = -1, double fps = -1,
      TSoundTrackFormat format = TSoundTrackFormat());

  TSoundTrackP mixingTogether(const std::vector<TXshSoundColumn *> &vect,
                              int fromFrame = -1, int toFram = -1,
                              double fps = -1);

  // Gain sections addressed by xsheet rows, [r0, r1] inclusive. A range
  // spanning several clips is split between them.
  void setGainForRows(int r0, int r1, double gainDb);
  double getGainDbAtRow(int row) const;
  //! True on the first visible row of a section, where its label is drawn.
  bool isGainSectionStart(int row) const;

  // Per-clip access for undo: the sections of clip levelIndex, in the order
  // the clips are stored.
  int getColumnLevelCount() const { return m_levels.size(); }
  std::vector<SoundGainSection> getGainSections(int levelIndex) const;
  void setGainSections(int levelIndex,
                       const std::vector<SoundGainSection> &sections);

  //! Peak and RMS of the audio under the rows before any gain, in dBFS.
  //! Returns false when the rows hold no audio.
  bool measureRows(int r0, int r1, double &peakDb, double &rmsDb) const;

protected:
  bool setCell(int row, const TXshCell &cell, bool updateSequence);
  void removeCells(int row, int rowCount, bool shift);

  void setCellInEmptyFrame(int row, const TXshCell &cell);

  /*! If index == -1 insert soundColumnLevel at last and than order
   * soundColumnLevel by startFrame. */
  void insertColumnLevel(ColumnLevel *columnLevel, int index = -1);
  void removeColumnLevel(ColumnLevel *columnLevel);

  ColumnLevel *getColumnLevelByFrame(int frame) const;
  ColumnLevel *getColumnLevel(int index);
  int getColumnLevelIndex(ColumnLevel *ss) const;
  void clear();

protected slots:
  void onTimerOut();
};

#ifdef _WIN32
template class DV_EXPORT_API TSmartPointerT<TXshSoundColumn>;
#endif
typedef TSmartPointerT<TXshSoundColumn> TXshSoundColumnP;

#endif
