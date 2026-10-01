# Note Level cell colors — change explanation

Task: `tasks/note_level_cell_colors.xml` · Summary: `tasks/knowledge/note_level_colors__change_summary.md`

## How it worked before

A **Note Level** in OpenToonz is a `TXshSoundTextColumn` whose cells reference a
`TXshSoundTextLevel`. That level holds one text string per frame (`m_framesText`), and a run of
consecutive cells pointing at the same `TFrameId` renders as one note block with the text spread
across it (`CellArea::drawSoundTextColumn`).

Every note cell in the sheet was painted the same flat grey: `SoundTextColumnColor(200,200,200)`,
or the selected variant. The only per-cell differentiation available was a **Cell Mark** — a small
semi-transparent dot in the corner, stored per *row* on the column (`TXshCellColumn::m_cellMarkIds`).
For organizing a column of notes by department, priority, or status, a corner dot is weak signal:
you cannot scan a sheet by it, and it is attached to the row rather than to the note, so it does not
survive moving the note.

## What changed

Note cells can now be tinted with a color chosen from the scene's existing 12-entry **Cell Mark**
palette, via a **Note Color** submenu on the cell context menu. The color belongs to the note, so:

- it covers the note's whole run of cells, not one row;
- it travels with the note when the cell is copied or moved, because it is keyed on the level frame;
- it is saved with the scene, undoable in one step (even across a multi-note selection), and marks
  the scene dirty.

## Where it changed

| Artifact | Change |
|---|---|
| `toonz/sources/include/toonz/txshsoundtextlevel.h:34,49-50` | `QList<int> m_framesColorId` parallel to `m_framesText`; `setFrameColorId` / `getFrameColorId` |
| `toonz/sources/toonzlib/txshsoundtextlevel.cpp:27,51-63,85-93,110-124` | accessors, `clone()` now copying text + colors, `frameColors` load/save |
| `toonz/sources/toonz/xshcellviewer.h:40-62,213` | `SetNoteColorUndo`, `onSetNoteColor` slot |
| `toonz/sources/toonz/xshcellviewer.cpp:2580,2637-2663,4234-4266,4535-4575,624-664` | blend helper, per-block tint, "Note Color" submenu, slot, undo |
| `tasks/knowledge/note_level_colors__change_summary.md` | durable summary |

## Why these changes were made

Three forks decided the shape of the implementation.

**Color on the note, not the row.** A row-keyed color (the Cell Mark model) is cheaper — the storage
already exists — but it detaches the color from its meaning: duplicate a note and the color stays
behind. Keying on `(level, frameIndex)` costs a parallel list on the level and makes the color part
of the note's identity. `frameIndex = cell.m_frameId.getNumber() - 1` is the same key
`drawSoundTextColumn` already uses to fetch the note's text, so no new addressing scheme was
introduced.

**Reuse the Cell Mark palette.** The alternatives were the 7 memo-note colors (`TXshNoteSet`) or a
free `QColorDialog` per note. The cell-mark palette wins on three counts: it is already
scene-configurable through Scene Settings, it is already named (so the undo history string and the
menu labels come for free), and it needs no new preference UI or wider persistence. A free picker
would have added a dialog, a per-note `TPixel32` to serialize, and no shared vocabulary between
notes.

**Blend, don't replace.** Several palette entries (Dark Blue `60,49,187`, Purple, Dark Pink) are
dark saturated colors, and the note text is drawn with `p.setPen(Qt::black)`. A flat fill would have
made those notes unreadable and would also have erased the difference between selected and
unselected cells. Mixing the note color into the *existing* base color at ratio 0.45 keeps both
properties: text contrast survives, and because the blend is applied to the selected base color too,
selection remains visible on a colored note.

## How it works now

```
right-click a note cell
  └─ createCellMenu  ...  cell.getSoundTextLevel() != nullptr
       └─ "Note Color" submenu, entries carry data {row, col, colorId}
            └─ CellArea::onSetNoteColor
                 ├─ clicked cell inside the current selection? → widen to the selection
                 ├─ walk the range, de-dupe notes on (level, frameIndex) with a std::set
                 ├─ build QList<SetNoteColorUndo::Note>{level, frameIndex, idBefore}
                 └─ SetNoteColorUndo(notes, colorId) → redo() → TUndoManager

paint pass
  └─ CellArea::drawSoundTextColumn
       ├─ per note block: getFrameColorId → TSceneProperties::getCellMark(id).color
       └─ per cell: tmpCellColor = blendNoteColor(base_or_selected_base, noteColor)

save
  └─ TXshSoundTextLevel::saveData → "frameColors" child, only if some note has a color
load
  └─ TXshSoundTextLevel::loadData  → "frameColors" branch; absent ⇒ every note is -1
```

## Core concepts

**Level-frame keying.** In OpenToonz a cell is a `(level, frameId)` pair, and many cells can share
one pair. Attaching data to the pair rather than to the cell position is what makes the color a
property of the note. Failure mode: `frameId` numbering is 1-based while the list is 0-based, so
every access must go through `getNumber() - 1`; an off-by-one here silently colors the neighboring
note.

**Guarded additive serialization.** `saveData` emits the `frameColors` child only when at least one
note is colored. That single `hasColor` guard is what keeps the format unchanged for scenes that do
not use the feature — an unconditional tag would have made every re-saved scene unreadable by older
builds. The read side is symmetric: an unknown tag still throws, but the new tag is a recognized
branch, and its absence leaves the list empty, which `getFrameColorId` reports as `-1`.

**Grow-on-write sparse lists.** `setFrameColorId` pads with `-1` up to the requested index, exactly
as `setFrameText` pads with a blank string. It keeps the two lists index-compatible without
requiring the caller to size anything. Tradeoff: coloring note #200 allocates 200 slots — negligible
here, wrong for a genuinely sparse key space (which is why cell marks use a `QMap` instead).

**One undo entry per user action.** `SetNoteColorUndo` takes a *list* of notes and one target id, so
recoloring a 30-note selection is one history entry rather than 30. It stores `TXshSoundTextLevelP`
smart pointers, not raw pointers: an undo stack can outlive the removal of a level from the scene,
and a raw pointer there is a use-after-free waiting for the user to press Ctrl+Z.

**Single paint path, two orientations.** `drawSoundTextColumn` serves both the vertical Xsheet and
the horizontal Timeline; the orientation branches sit *below* where the tint is applied. Hooking the
blend above those branches is what makes Timeline support free rather than a second implementation.

## Verification evidence

- **Build:** `cmake --build toonz/build --config Release --target OpenToonz` succeeded after each
  of the three phases, producing `toonz/build/Release/OpenToonz.exe` (final build 2026-08-31 11:04).
  One intermediate link failure was environmental — the running p1 binary held `toonzlib.dll`; the
  app was closed and the link succeeded.
- **Run:** the Release binary launches and reaches its main window.
- **Static, persistence:** the `hasColor` guard means an uncolored scene writes no new tag; the
  absent-tag path defaults every note to `-1`; the unknown-tag `throw` is intact. The serialization
  shape (space-separated ints in a `QString` child) is the one
  `TXshCellColumn::saveCellMarks`/`loadCellMarks` already round-trips
  (`toonz/sources/toonzlib/txshcolumn.cpp:433-474`).
- **Static, interaction:** multi-note recolor is one undo entry by construction; the de-dupe set
  prevents a note being recorded once per cell of its run.

**Blind spot, stated plainly:** the GUI was never driven — this environment cannot click through the
OpenToonz UI. No rendered tint, menu interaction, save/reload cycle or undo was *observed*. Every
success criterion was carried by build plus code review. Runtime confirmation is the user's
checkpoint, and the three checkpoints in the task's root `<notes>` say exactly what to try.

## Remaining risks and next moves

- **Blend ratio is a guess (0.45).** It was chosen to keep black text legible against the darkest
  palette entries, not tuned against a real sheet. If light colors read too weakly or dark ones too
  strongly, it is a one-constant change in `blendNoteColor`
  (`toonz/sources/toonz/xshcellviewer.cpp:2580`).
- **The column header stays grey.** A colored note column looks unchanged when collapsed or viewed
  from the header. `XsheetViewer::getColumnColor` derives the header color from the top-most
  occupied cell and was deliberately left alone (declared non-goal).
- **Not carried into exports.** `exportxsheetpdf` and the renderer's note text (`scenefx`) ignore the
  color — both were declared non-goals. A printed sheet loses the organization the color provides,
  which is worth revisiting if notes are used in printed exposure sheets.
- **No Scene Settings label for the note-color use.** The palette is shared with cell marks, so
  editing a color there changes both. That is intentional reuse, but a user who retunes "Red" for
  cell marks also retunes it for notes.
- **A copied note shares its color by identity, not by copy.** Because the color lives on the level
  frame, two cells referencing the same note frame cannot have different colors — by design, but it
  means "duplicate this note and recolor the copy" requires a genuinely new note frame.
