# 📚 OpenToonz Optimization & Enhancement Scratchpad

## 🎯 Phase 1: Mathematical Foundations
*Check off as you review or feel comfortable with these concepts. These are the mathematical building blocks of 2D graphics software.*

- [ ] **Linear Algebra & Affine Transformations**
  - Matrix multiplication, translation, rotation, scaling in 2D.
  - *Context:* Used heavily in `toonz/sources/common/tgeometry/` and for camera/layer manipulations.
- [ ] **Calculus & Differential Geometry**
  - Bézier curves (quadratic, cubic), B-Splines, finding tangents and normals.
  - *Context:* Core to how strokes are mathematically represented and rendered smoothly.
- [ ] **Computational Geometry**
  - Delaunay Triangulations, Polygon clipping, mesh generation.
  - *Context:* The math behind the "Plastic Tool" (mesh deformation) and boolean vector operations.
- [ ] **Digital Signal & Image Processing (DSP)**
  - Convolution filters, anti-aliasing algorithms, color space conversions.
  - *Context:* Raster brushing, bucket fill algorithms, and visual effects (Fx).

## 📄 Phase 2: Core Reading & Research Checklist
*Search these papers/topics to understand the bleeding-edge of 2D software optimization.*

- [ ] **Rendering Optimization:** Read *"Massively-Parallel Vector Graphics"* (Ganacim et al., 2014).
- [ ] **Mesh Deformation:** Read *"As-Rigid-As-Possible Shape Manipulation"* (Takeo Igarashi, 2005).
- [ ] **Brush Engine Physics:** Search for "Stroke-Based Rendering" or examine `libmypaint` documentation (the library used by OpenToonz for raster brushes).
- [ ] **AI/Deep Learning in Animation:** Read *"Deep Animation Video Interpolation in the Wild" (AnimeInterp)* (Li Siyao, 2021).
- [ ] **AI/Deep Learning for Line Art:** Search for *"Sketch-Aware Interpolation Network (SAIN)"*.

## 🗺️ Phase 3: OpenToonz Codebase Entrypoints
*Where to start exploring the `opentoonz-eva` repository based on your goals:*

### 1. Vector Graphics & Stroke Engine
*If you want to optimize how drawing feels or how vector lines are calculated:*
- **Directory:** `toonz/sources/common/tvectorimage/` and `toonz/sources/common/tvrender/`
- **Key Classes:** `TStroke`, `TVectorImage`
- **What it does:** Handles the math for Bezier strokes, curve smoothing, and converting input device points into mathematical paths.

### 2. Raster Graphics & Image Processing
*If you want to optimize rendering speed, blending, or brush performance:*
- **Directory:** `toonz/sources/common/traster/`
- **Key Classes:** `TRaster`, `TRaster32P`
- **What it does:** The core image structures for pixel manipulation. High impact area for SIMD or multithreading optimizations.

### 3. Image I/O & Serialization (You are here!)
*If you are looking to optimize how fast projects save/load or reduce file sizes/memory usage:*
- **Directory:** `toonz/sources/image/tzl/` (e.g., `tiio_tzl.cpp`)
- **What it does:** TZL is OpenToonz's custom format for saving palettes and levels. Optimizing chunk reading/writing, memory mapping, or caching here directly speeds up disk operations and prevents crashes.

### 4. Interactive Tools (Brush, Plastic, etc.)
*If you want to improve a specific tool's algorithm or user interaction:*
- **Directory:** `toonz/sources/tnztools/`
- **What it does:** Contains the logic for what happens when you drag the mouse/stylus on the canvas for every tool (Brush, Fill, Plastic Mesh, Control Point Editor, etc.).

### 5. Base Math & Geometry Utilities
*If you want to upgrade the fundamental math routines for performance:*
- **Directory:** `toonz/sources/common/tgeometry/` and `toonz/sources/common/tmath/`
- **What it does:** Low-level matrix classes, spline calculations, and intersection testing.
