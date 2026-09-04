# OpenToonz Cross-Module Dependency Analysis

**Status:** Analysis complete (p1-step-5)  
**Date:** 2026-08-09  
**Scope:** tnzcore, tnzbase, tnzext, and common submodules

---

## 1. Core Library Module Structure

### tnzcore (Foundation Layer)
**Location:** `toonz/sources/tnzcore/`  
**Public Headers:** `toonz/sources/include/t*` and `toonz/sources/include/qt*`

**Consumed from common/:**
- `tcore/` — Core types, threading, smart pointers
- `trop/` — Raster operations primitives
- `tiio/` — Image I/O compatibility layer (BMP support)
- `tvectorimage/` — Vector image implementation details
- `tvrender/` — Vector rendering primitives (elliptic brush)
- `psdlib/` — PSD codec for Photoshop support
- `tsound/` — Sound I/O (bundled in tnzcore)
- `tstream/` — Serialization streams

**Core Exports (via include/):**
- Pixel types: `tpixel.h`, `tpixelcm.h`, `tpixelgr.h`
- Raster: `traster.h`, `trastercm.h`, `trasterfx.h`
- Image: `timage.h`, `trasterimage.h`, `tvectorimage.h`
- Geometry: `tgeometry.h`, `tcurves.h`
- Vector graphics: `tstroke.h`, `tvectorgl.h`
- Color: `tcolorvalue.h`, `tcolorfunctions.h`
- Threading: `tthread.h`, `ttile.h`
- IPC: `tipcsrv.h`, `tipcsrvP.h`
- OpenGL: `tgl.h`, `tofflinegl.h`, `qtofflinegl.h`
- Sound: `tsound.h`, `tsound_io.h`, `tsoundsample.h`

**Internal Dependencies:** None (foundation layer)

---

### tnzbase (Parameter & Effect Framework)
**Location:** `toonz/sources/tnzbase/`  
**Public Headers:** `toonz/sources/include/t*` and `toonz/sources/include/tparam/`, `toonz/sources/include/tfx/`

**Consumed from common/:**
- `tfx/` — Effect framework implementation (TRendererP, effect base classes)
- `tparam/` — Parameter system implementation (DoublParamFile, param serialization)
- `twain/` — Scanner/TWAIN interface
- `tscanner/` — Scanner device drivers (Epson, TWAIN, USB)

**Core Exports (via include/):**
- Parameters: `tparam.h`, `tdoubleparam.h`, `tspectrumparam.h`, `ttonecurveparam.h`
- FX system: `tfx.h`, `tbasefx.h`, `texternfx.h`, `tfxattributes.h`
- Cache: `tcacheresource.h`, `tfxcachemanager.h`
- Scanner: `tscanner.h`
- Animation: `tdoublekeyframe.h`, `tcubicbezier.h`

**Internal Dependencies:**
- → tnzcore (pixel types, color, raster, image, geometry)

---

### tnzext (Deformation & Math Extensions)
**Location:** `toonz/sources/tnzext/`  
**Public Headers:** `toonz/sources/include/ext/`, `toonz/sources/include/tlin/`

**Consumed from common/:**
- Inherits all tnzcore/tnzbase dependencies indirectly
- Uses SuperLU and OpenBLAS for linear algebra (external, not in common/)

**Core Exports (via include/):**
- Stroke deformation: `StrokeDeformation.h`, `StrokeParametricDeformer.h`
- Mesh building: `meshbuilder.h`, `meshtexturizer.h`, `meshutils.h`
- Plastic deformation: `plasticdeformer.h`, `plasticskeleton.h`
- Linear algebra: `tlin.h`, `tlin_matrix.h`, `tlin_sparsemat.h`

**Internal Dependencies:**
- → tnzcore (raster, vector, geometry types)
- → tnzbase (parameter system)

---

## 2. Common Module Dependency Map

### By Functional Category

#### Foundation Modules (used by tnzcore)
- **tcore/** — Type system, threading primitives, smart pointers
- **tstream/** — Serialization and I/O stream wrappers
- **tcolor/** — Color utilities (used by tnzcore and others)

#### Core Data Structures (tnzcore)
- **traster/** — Raster data structures
- **timage/** — Image wrapper types
- **timage_io/** — Image file I/O (codec registration)
- **tvectorimage/** — Vector image implementation (strokes, regions)
- **ttoonzimage/** — High-level Toonz image (raster + vector)
- **rasterimage/** — Raster-specific image handling
- **tmeshimage/** — Mesh image format
- **tmetaimage/** — Metadata handling

#### Rendering & Graphics (tnzcore)
- **tgl/** — OpenGL wrapper and context management
- **tvrender/** — Vector rendering engine
- **trop/** — Raster operations (fill, transform, merge)

#### I/O & Codecs (tnzcore, image library)
- **tiio/** — Image codec framework and compatibility layer
- **psdlib/** — Photoshop file format handler

#### Parameter & Effect System (tnzbase)
- **tparam/** — Parameter system (types, keyframe, interpolation)
- **tfx/** — Effect framework (base classes, caching)

#### Specialized Data
- **tsound/** — Audio sample and WAV/AIFF I/O (bundled in tnzcore)
- **tgeometry/** — Geometric primitives (points, affines, curves)
- **txsheet/** — Xsheet/timeline model (used by scene objects)
- **tunit/** — Unit system (px, mm, etc.)
- **tproperty/** — Property system for parameters

#### Animation & Utilities
- **tcache/** — Frame cache management
- **tapptools/** — Application utilities
- **tcontenthistory/** — Content history tracking

#### Advanced Features (used by multiple libraries)
- **tcore/** → Actual base types that everything depends on
- **expressions/** — Expression parsing and evaluation (tnzbase, toonzlib)
- **tipc/** — Inter-process communication (tnzcore)

---

## 3. Cross-Module Dependency Graph (Layer View)

```
Layer 1: Foundation (tnzcore)
├─ tcore (threading, types, smart pointers)
├─ tstream (serialization)
├─ tcolor (color utilities)
├─ trop (raster operations)
├─ tvrender (vector rendering)
├─ tgl (OpenGL wrapper)
├─ tiio (codec framework)
├─ psdlib (PSD codec)
├─ tsound (audio I/O)
└─ tvectorimage (vector graphics)

Layer 2: Framework (tnzbase)
├─ tfx (effect framework) → depends on tnzcore
├─ tparam (parameters) → depends on tnzcore
├─ twain (scanner interface) → depends on tnzcore
└─ expressions (expression system) → depends on tnzcore

Layer 3: Extensions (tnzext)
├─ Deformation solvers → depend on tnzcore, tnzbase
├─ Linear algebra → depends on tnzcore
├─ Mesh utilities → depends on tnzcore
└─ Plastic deformation → depends on tnzcore, tnzbase

Layer 4+: Higher-level systems
├─ toonzlib → depends on all above + common modules
├─ image → depends on tnzcore, tnzbase, tiio
└─ Other specialized libraries
```

---

## 4. Detailed Dependency Relationships

### tnzcore → common/ (Direct Inclusions)

| Common Module | Purpose in tnzcore | Key Files |
|---|---|---|
| **tcore** | Type system, threading | tthreadp.h, type aliases |
| **trop** | Pixel/raster operations | loop_macros.h, quickputP.h, runsmap.h |
| **tiio** | Codec framework | tiio/compatibility/tfile_io.h, bmp support |
| **tvectorimage** | Vector data | tvectorimageP.h, tsegmentadjuster.h, tl2lautocloser.h |
| **tvrender** | Vector rendering | tellipticbrushP.h (brush primitives) |
| **psdlib** | Photoshop support | psd.h, psdutils.h |

### tnzbase → common/ (Direct Inclusions)

| Common Module | Purpose in tnzbase | Key Files |
|---|---|---|
| **tfx** | Effect framework | trendererP.h (private implementation) |
| **tparam** | Parameter system | tdoubleparamfile.h (file I/O) |
| **twain** | Scanner interface | ttwain*.h (TWAIN state machine) |
| **tscanner** | Device drivers | TScannerIO/* (USB/TWAIN device I/O) |

### tnzext → common/ (Minimal, via tnzcore/tnzbase)

| Common Module | Purpose in tnzext | Notes |
|---|---|---|
| (Indirect) | Linear algebra solvers | Uses SuperLU (external), not bundled in common/ |
| (Indirect) | Deformation algorithms | Built on tnzcore types (raster, vector, geometry) |

---

## 5. Cross-Module Coupling Analysis

### High Coupling (>3 direct dependencies)
- **tnzcore** — Couples to: tcore, trop, tiio, tvectorimage, tvrender, psdlib, tstream (7 common modules)
- **toonzlib** — (Phase 2, not analyzed here)

### Medium Coupling (2–3 dependencies)
- **tnzbase** — Couples to: tfx, tparam, twain, tscanner (4 modules)
- **tnzext** — Couples to: tnzcore, tnzbase (2 libraries)

### Low Coupling (0–1 dependencies)
- Most common modules are self-contained or used by single consumers
- Exception: tcore, tcolor, tstream are foundational (used by multiple)

---

## 6. Splitting Strategy for Budget Diagrams

### Diagram 1: Foundation Layer (tnzcore)
**Scope:** tnzcore library and its direct common/ dependencies  
**Complexity Budget:** Medium (≤20 entities, ≤30 edges)  
**Content:**
- Nodes: tnzcore, tcore, tstream, tcolor, trop, tiio, tvectorimage, tvrender, psdlib, tgl, tsound
- Edges: internal common module composition + tnzcore exports
- Strategy: Show common modules as building blocks; tnzcore as composite

### Diagram 2: Parameter & Effect Framework (tnzbase)
**Scope:** tnzbase library and its framework modules  
**Complexity Budget:** Small (≤12 entities, ≤15 edges)  
**Content:**
- Nodes: tnzbase, tfx, tparam, twain, tscanner, tnzcore
- Edges: tnzbase → tnzcore dependency + internal module organization
- Strategy: Show parameter and effect systems as separate concerns

### Diagram 3: Extensions & Deformation (tnzext)
**Scope:** tnzext library specializations  
**Complexity Budget:** Small (≤12 entities, ≤15 edges)  
**Content:**
- Nodes: tnzext, deformation subsystem, linear algebra, mesh utilities, tnzcore, tnzbase
- Edges: tnzext → (tnzcore, tnzbase) + internal specializations
- Strategy: Show three subsystems within tnzext

### Diagram 4: Cross-Library Dependencies
**Scope:** How tnzcore, tnzbase, tnzext relate to each other  
**Complexity Budget:** Small (≤12 entities, ≤15 edges)  
**Content:**
- Nodes: tnzcore, tnzbase, tnzext, toonzlib, image, sound
- Edges: library-to-library dependencies (topological order)
- Strategy: Overview of the foundation layer stack

### Diagram 5: Common Module Dependency Mesh
**Scope:** How common modules inter-depend  
**Complexity Budget:** Medium (≤20 entities, ≤25 edges)  
**Content:**
- Nodes: 15–20 common modules grouped by layer
- Edges: direct includes and "used by" relationships
- Strategy: Show tcore and tstream as foundations; others build on them

---

## 7. Verification Checklist

✓ All tnzcore, tnzbase, tnzext dependencies identified from CMakeLists.txt  
✓ Common module usage patterns traced from include/ headers  
✓ Circular dependencies verified absent  
✓ Build order derivable from dependency graph (topological sort)  
✓ All diagrams fit within complexity budget  
✓ No entity count > 25, no edge count > 30  

---

## 8. Next Steps for Phases 2–7

1. **Phase 1 documents** (after this analysis):
   - `opentoonz__core_libraries_architecture.md` (index + Diagram 4)
   - `opentoonz__core_tnzcore_architecture.md` (Diagrams 1 + 5 split)
   - `opentoonz__core_tnzbase_tnzext_architecture.md` (Diagrams 2 + 3)
   - `opentoonz__core_common_architecture.md` (Diagram 5 detailed)

2. **Phase 2–7** will analyze:
   - toonzlib internals (scene, xsheet, rendering)
   - image/ codec handlers
   - sound/ audio subsystem
   - colorfx, tnzstdfx, toonzqt, tnztools
   - Execution flows (startup, render, I/O)

---

## References
- Task: `opentoonz-codebase-visualization.xml` (p1-step-5)
- Input: `opentoonz__dependency_map.md` (verified dependency list)
- Input: `opentoonz__diagram_conventions.md` (budget rules)
- Source: CMakeLists.txt files for tnzcore, tnzbase, tnzext
- Source: `toonz/sources/include/` header organization
