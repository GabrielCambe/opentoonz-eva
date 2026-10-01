# Note Level cell colors — change summary

Task plan: `tasks/note_level_cell_colors.xml`

## What was added

Note Level cells in the Xsheet/Timeline can be tinted with a color chosen from the scene's
existing **Cell Mark** palette, so notes can be organized visually. The color is attached to the
**note**, not to the row: it applies to every cell in the note's run and travels with the note
when the cell is copied or moved.

Reached from the Xsheet cell context menu on a note cell → **Note Color** → `None` or one of the
12 palette entries. The existing **Cell Mark** submenu is untouched and still works on the same
cells.

## Where the color lives

The key is `(TXshSoundTextLevel*, frameIndex)` where `frameIndex = cell.m_frameId.getNumber() - 1`
— the same key the viewer already uses to read a note's text. `TXshSoundTextColumn::createSoundTextLevel`
assigns `TFrameId(i + 1)` per note entry, which is what makes that key stable.

- `TXshSoundTextLevel::m_framesColorId` — `QList<int>` parallel to `m_framesText`
  ([txshsoundtextlevel.h:34](toonz/sources/include/toonz/txshsoundtextlevel.h#L34)).
- `setFrameColorId` / `getFrameColorId`
  ([txshsoundtextlevel.cpp:51](toonz/sources/toonzlib/txshsoundtextlevel.cpp#L51),
  [:61](toonz/sources/toonzlib/txshsoundtextlevel.cpp#L61)). Grow-on-write with `-1`, mirroring
  `setFrameText`. **`-1` means "no color"** and is the default for every note.
- Ids index `TSceneProperties::getCellMarks()` — the same 12 named, scene-configurable colors as
  cell marks. No new palette, no new preference.
- `clone()` now copies both `m_framesText` and `m_framesColorId`
  ([txshsoundtextlevel.cpp:27](toonz/sources/toonzlib/txshsoundtextlevel.cpp#L27)); it previously
  copied only the name.

## On-disk format

`TXshSoundTextLevel::saveData` writes **one extra child, `frameColors`**, holding the ids as a
space-separated string, placed after the `frame` children and before `type`
([txshsoundtextlevel.cpp:110](toonz/sources/toonzlib/txshsoundtextlevel.cpp#L110)).

The backward-compatibility rule is the `hasColor` guard: **the tag is written only when at least
one note in the level actually has a color**, so a scene that does not use the feature is saved in
exactly the format it had before. `loadData` accepts the tag as a new branch and keeps the existing
unknown-tag `throw` ([txshsoundtextlevel.cpp:85](toonz/sources/toonzlib/txshsoundtextlevel.cpp#L85));
a scene without the tag leaves the list empty, which `getFrameColorId` reports as `-1` everywhere.

The serialization shape (a `QString` of space-separated ints written with `os.child(...) << QString`
and read back with `is >> QString` then split) is the same one
`TXshCellColumn::saveCellMarks`/`loadCellMarks` already round-trips
([txshcolumn.cpp:433](toonz/sources/toonzlib/txshcolumn.cpp#L433)).

## Rendering

`CellArea::drawSoundTextColumn` resolves the note color once per cell block and blends it into the
cell background rather than replacing it
([xshcellviewer.cpp:2637](toonz/sources/toonz/xshcellviewer.cpp#L2637)):

- `blendNoteColor(base, note, ratio)` mixes the note color over a base color
  ([xshcellviewer.cpp:2580](toonz/sources/toonz/xshcellviewer.cpp#L2580)). Two ratios are used:
  **0.45 for the cell body**, and **0.8 for the drag-handle strip** on the leading edge of the cell
  (`drawDragHandle`), which carries no text and so takes nearly the full note color — that stripe
  is what makes a column of notes scannable.
- It is applied to **both** the normal and the selected base color, so selected note cells stay
  distinguishable from unselected ones.
- The note text (drawn in black) stays legible because the body tint never reaches full saturation —
  several palette entries (Dark Blue, Purple, Dark Pink) would be unreadable as a flat fill.
- One code path serves both orientations; the vertical/horizontal branches below it, the text
  eliding logic, and the cell-mark dot are untouched.

## Interaction and undo

- `CellArea::onSetNoteColor` ([xshcellviewer.cpp:4535](toonz/sources/toonz/xshcellviewer.cpp#L4535))
  recolors **every distinct note in the current cell selection** when the clicked cell belongs to
  that selection, and only the clicked note otherwise. A `std::set<(level, frameIndex)>` de-dupes,
  because one note spans many cells.
- `SetNoteColorUndo` ([xshcellviewer.h:40](toonz/sources/toonz/xshcellviewer.h#L40),
  [xshcellviewer.cpp:627](toonz/sources/toonz/xshcellviewer.cpp#L627)) stores every affected note as
  `(TXshSoundTextLevelP, frameIndex, idBefore)` plus the single new id, so recoloring a whole
  selection is **one** undo entry. It holds smart pointers, so an undone note level stays alive in
  the history. Undo/redo notify the xsheet and set the scene dirty flag.

## Touched files

| File | What |
|---|---|
| `toonz/sources/include/toonz/txshsoundtextlevel.h` | color storage + accessors |
| `toonz/sources/toonzlib/txshsoundtextlevel.cpp` | accessors, persistence, clone |
| `toonz/sources/toonz/xshcellviewer.h` | `SetNoteColorUndo`, `onSetNoteColor` slot |
| `toonz/sources/toonz/xshcellviewer.cpp` | tint, "Note Color" submenu, undo, slot |

## Deliberately out of scope

Column header color, `exportxsheetpdf` output, the renderer (`scenefx`) note text, a free color
picker, and the separate memo-note colors (`TXshNoteSet`) — all unchanged. `drawSoundTextCell`
(xshcellviewer.cpp) was left alone: its only call site is commented out, so it is dead code.
