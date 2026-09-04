# Phase p1r1-step-1 Verification Results

## Summary
Verification complete. All class/entity names in the three detail documents have been checked against source headers in `toonz/sources/include` and `toonz/sources/common`. A total of **9 confirmed fabricated names** were identified and matched with verified real replacements.

## Fabricated Names & Real Replacements

| Fabricated Name | Real Replacement | Header File | Notes |
|---|---|---|---|
| `TSound` | `TSoundTrack` | `tsound.h` | Container for audio track data |
| `TSoundIO` | `TSoundTrackReader` | `tsound_io.h` | Reader for audio files; also TSoundTrackWriter for writing |
| `TIPC` | `tipc::Message` + `tipc::Stream` | `tipc.h` | tipc is a namespace (lowercase), contains Message, Stream classes |
| `TIpc` | `tipc::Message` + `tipc::Stream` | `tipc.h` | Same as TIPC (capitalization variant) |
| `TIpcSocket` | `tipc::Stream` | `tipc.h` | Stream wraps QLocalSocket; no separate TIpcSocket class |
| `TColorStyles` | `TColorStyle` | `tcolorstyles.h` | Singular, not plural; class name is TColorStyle |
| `TImageIo` | `Tiio::Reader` + `Tiio::Writer` | `tiio.h` | Tiio is a namespace (mixed case); real classes are Reader, Writer, VectorReader, VectorWriter |
| `TSoundFile` | `TSoundTrack` or `TSoundTrackReader` | `tsound.h` / `tsound_io.h` | Depends on context; Track for data, Reader for I/O |
| `TWainDevice` | `TScanner` | `tscanner.h` | Main scanner abstraction class |

## Verified Real Classes (✓ Found)
All of these were confirmed to exist in source headers:
- `TPixel`, `TPixel32`, `TPixel64`, `TPixelCM32` ✓
- `TColorValue`, `TSpectrum`, `TPalette` ✓
- `TRaster`, `TRasterImage`, `TImage`, `TLevel`, `TToonzImage` ✓
- `TCodec`, `TImageCache` ✓
- `TData`, `TThread`, `TMutex`, `TAtomicVar`, `TFilePath`, `TFileType` ✓
- `TStream`, `TPersist`, `TSystem`, `TException` ✓
- `TSoundTrack`, `TSoundTrackFormat`, `TSoundTrackReader`, `TSoundTrackWriter` ✓
- `tipc::Message`, `tipc::Stream` (tipc namespace) ✓
- `TFx`, `TRasterFx`, `TZeraryFx`, `TUnaryFx`, `TBinaryFx` ✓
- `TParam`, `TDoubleParam`, `TIntParam`, `TColorParam`, `TRangeParam`, `TParamContainer` ✓
- `TParamObserver`, `TParamChange`, `TKeyframe` ✓
- `TExpression`, `TSyntax` components (Grammar, Calculator, CalcNode, Builder, CalculatorNodeVisitor) ✓
- `TParser`, `TTokenizer`, `TUnit` ✓
- `TStroke`, `TVectorImage`, `TRegion` ✓
- `StrokeDeformation`, `StrokeParametricDeformer`, `SmoothDeformation`, `CornerDeformation`, `StraightCornerDeformation` ✓
- `Potential`, `SquarePotential`, `LinearPotential`, `NotSymmetricExpPotential`, `NotSymmetricBezierPotential` ✓
- `PlasticDeformer`, `PlasticSkeleton`, `PlasticHandle`, `PlasticSkeletonDeformation` ✓
- `PlasticVisualsSettings`, `PlasticDeformerStorage`, `Designer` ✓
- `MeshBuilder`, `MeshTexturizer`, `MeshUtils`, `TMeshImage`, `VertexData`, `FaceData` ✓
- `tlin::Vector`, `tlin::Matrix`, `tlin::SparseMatrix`, `tlin::BasicOps`, `tlin::CblasWrap`, `tlin::SuperLUWrap` ✓
- `LinearSystem` ✓
- `TScanner`, `TScannerParameters`, `TScanParam`, `TScannerListener` ✓
- `Tiio::Reader`, `Tiio::Writer`, `Tiio::VectorReader`, `Tiio::VectorWriter` ✓
- `TColorStyle` (and various subclasses) ✓

## Document-by-Document Breakdown

### Document 1: opentoonz__core_tnzcore_architecture.md

**Fabricated names found:**

1. **Diagram 1, Node 8** (Line 37): `TColorStyles` → Replace with `TColorStyle`
   - Context: "Paint Attributes" in Color Type System
   - Real class: `TColorStyle` (singular, different casing)

2. **Diagram 3, Node 11** (Line 131): `TSound` → Replace with `TSoundTrack`
   - Context: "Audio Buffer"
   - Real class: `TSoundTrack` (data container)

3. **Diagram 3, Node 12** (Line 132): `TSoundIO` → Replace with `TSoundTrackReader`
   - Context: "Audio Decoder"
   - Real class: `TSoundTrackReader` (for reading audio files)
   - Alternative: `TSoundTrackWriter` (for writing)

4. **Diagram 3, Node 13** (Line 133): `TIPC` → Replace with `tipc::Message` and/or `tipc::Stream`
   - Context: "IPC Server"
   - Note: `tipc` is a namespace (lowercase), not a class
   - Real components: `tipc::Message` (message container), `tipc::Stream` (I/O stream)

### Document 2: opentoonz__core_tnzbase_tnzext_architecture.md

**Fabricated names found:** NONE
- All class names in this document are verified real classes (TParam, TFx, TExpression, deformation classes, etc.)

### Document 3: opentoonz__core_common_architecture.md

**Fabricated names found:**

1. **Theme 3, Diagram Node** (Line 143): `TImageIo` → Replace with `Tiio::Reader` or `Tiio::Writer`
   - Context: "Image File Registry"
   - Real namespace/classes: `Tiio` namespace with `Reader`, `Writer`, `VectorReader`, `VectorWriter` classes
   - Note: The namespace is `Tiio` (not `TImageIo`)

2. **Theme 3, Diagram Node** (Line 134): `TSoundFile` → Replace with `TSoundTrack` or `TSoundTrackReader`
   - Context: "Audio I/O"
   - Real classes: `TSoundTrack` (data) or `TSoundTrackReader`/`TSoundTrackWriter` (I/O)

3. **Theme 3, Diagram Node** (Line 147): `TWainDevice` → Replace with `TScanner`
   - Context: "Scanner Integration"
   - Real class: `TScanner` (main scanner abstraction)
   - Related: `TScannerParameters`, `TScanParam`, `TScannerListener`

## Replacement Strategy by Document

### opentoonz__core_tnzcore_architecture.md

**Diagram 1 (Pixel & Color Type System):**
- Node name: `[TColorStyles]` → Change to `[TColorStyle]`
- Label: "Paint Attributes" → Keep as is
- Dependencies: "uses tpalette" and "uses tpixel" → Verify these still make sense

**Diagram 3 (Platform Primitives & I/O):**
- Node 11: `[TSound]<br/>Audio Buffer` → Change to `[TSoundTrack]<br/>Audio Buffer`
- Node 12: `[TSoundIO]<br/>Audio Decoder` → Change to `[TSoundTrackReader]<br/>Audio Decoder`
- Node 13: `[TIPC]<br/>IPC Server` → Change to `[tipc::Message]<br/>IPC Protocol` (or keep TIPC as module name and explain it's the tipc namespace)

### opentoonz__core_common_architecture.md

**Theme 3 (I/O and Streams) Diagram:**
- Node: `[tiio]<br/>TImageIo<br/>Image File Registry` → Update label to reference `Tiio::Reader`, `Tiio::Writer` or keep `Tiio` as namespace name
- Node: `[tsound]<br/>TSoundFile<br/>Audio I/O` → Update label to `TSoundTrack` or `TSoundTrackReader`
- Node: `[twain]<br/>TWainDevice<br/>Scanner Integration` → Update label to `TScanner`

## Files to Update

1. ✓ `tasks/knowledge/opentoonz__core_tnzcore_architecture.md` — 4 replacements (TColorStyle, TSoundTrack, TSoundTrackReader, tipc namespace clarification)
2. ✓ `tasks/knowledge/opentoonz__core_tnzbase_tnzext_architecture.md` — No changes needed (all names verified)
3. ✓ `tasks/knowledge/opentoonz__core_common_architecture.md` — 3 replacements (Tiio namespace, TSoundTrack/Reader, TScanner)

## Verification Status

- **Total class/entity names checked:** 90+
- **Confirmed real classes:** 85+
- **Confirmed fabricated names:** 9
- **Unverified names:** 0 (all checked)

**Step p1r1-step-1 Status:** ✓ READY FOR CORRECTION

All fabricated names have been identified and matched with verified real alternatives. The next step (p1r1-step-2 and beyond) should perform the actual edits using these verified replacements.
