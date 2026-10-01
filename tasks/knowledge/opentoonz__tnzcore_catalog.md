# tnzcore Type System & File Inventory Catalog

**Date:** 2026-08-09  
**Status:** Research Complete  
**Scope:** tnzcore foundational library source catalog and core type system inventory

---

## Overview

The **tnzcore** library is the foundational layer of OpenToonz, providing:
- **Core data types** (pixels, colors, rasters, images, vectors, geometry)
- **Rendering infrastructure** (OpenGL wrappers, offline GL, tessellation)
- **I/O operations** (file paths, stream serialization, image I/O)
- **Threading & synchronization** (mutexes, thread management, messaging)
- **Audio processing** (sound buffers, WAV/AIFF decoding)
- **IPC mechanisms** (TCP/IP server/client for 32-bit rendering service)
- **Raster operations** (blur, dithering, morphological ops, etc.)

tnzcore is built from **source files in `toonz/sources/common/`** (not in the empty `toonz/sources/tnzcore/` directory). All public headers are in `toonz/sources/include/`.

---

## Module Inventory

The tnzcore library is organized into **18 core modules**. Below is the complete catalog:

### 1. **tcore/** – Core Infrastructure
**Responsibility:** Fundamental types, utilities, threading, undo/redo, messaging  
**Key Headers:**
- `tcommon.h` – Common includes, macros, platform definitions
- `tundecpp.h` – Undo/redo system (C++ wrapper)
- `tthreadp.h` – Private threading declarations (Qt-based)
- `tfunctorinvoker.h` – Async function invocation via message queue
- `tmsgcore.h` – Message-based threading

**Key Source Files:**
- `tdata.cpp` – Data serialization base
- `tdebugmessage.cpp` – Debug message routing
- `texception.cpp` – Exception system
- `threadmessage.cpp` – Thread messaging
- `tmathutil.cpp` – Math utilities
- `trandom.cpp` – Random number generation
- `tsmartpointer.cpp` – Smart pointer implementation
- `tstopwatch.cpp` – Performance timing
- `tstring.cpp` – String utilities
- `tthread.cpp` – Threading primitives (Qt-based)
- `tundo.cpp` – Undo/redo history management
- `tfunctorinvoker.cpp` – Async function dispatch

**Key Types:**
- `TSmartPointerT<T>` – Reference-counted smart pointers
- `TRectT<T>`, `TRectI`, `TRectD` – Rectangle types (int/double)
- `TThread`, `TMutex`, `TCondition` – Synchronization primitives
- `TUndoManager` – Undo/redo history
- `TMessageQueue` – Thread-safe message passing

---

### 2. **tcolor/** – Color & Pixel Processing
**Responsibility:** Color types, pixel formats, color conversions, palette management  
**Key Headers:**
- `tpixel.h` – Pixel type definitions (TPixelRGBM32, TPixelRGBM64, TPixelD, TPixelGR8, etc.)
- `tpixelcm.h` – Color-mapped pixel types (indexed color)
- `tpixelgr.h` – Grayscale pixel types
- `tpixelutils.h` – Pixel manipulation utilities
- `tcolorfunctions.h` – Color space conversions, blending
- `tcolorvalue.h` – Color value abstractions
- `tspectrum.h` – Spectrum/gradient color data

**Key Source Files:**
- `tpixel.cpp` – Pixel type implementations
- `tpixelutils.cpp` – Pixel operation kernels
- `tcolorfunctions.cpp` – Color math
- `tcolorvalue.cpp` – Color value operations
- `tspectrum.cpp` – Spectrum operations

**Key Types:**
- `TPixelRGBM32` – Standard 32-bit RGBA (1 byte per channel)
- `TPixelRGBM64` – 64-bit RGBA (2 bytes per channel)
- `TPixelD` – Double-precision RGBA
- `TPixelGR8` – 8-bit grayscale
- `TPixelCM8` – 8-bit color-mapped
- `TSpectrum` – Color gradient with control points

---

### 3. **tgeometry/** – 2D/3D Geometry Types
**Responsibility:** Points, rectangles, affine transforms, curves, bezier paths  
**Key Headers:**
- `tgeometry.h` – Core geometry types
- `tcurves.h` – Cubic bezier curve representation and evaluation
- `tcurveutil.h` – Curve manipulation utilities

**Key Source Files:**
- `tgeometry.cpp` – Geometry implementations
- `tcurves.cpp` – Bezier curve implementations
- `tcurveutil.cpp` – Curve utility operations

**Key Types:**
- `TPointT<T>`, `TPoint`, `TPointD` – 2D points (int/double)
- `TRectT<T>`, `TRect`, `TRectD` – Axis-aligned rectangles
- `T3DPointT<T>`, `T3DPoint`, `T3DPointD` – 3D points
- `TAffineT<T>`, `TAffine`, `TAffineD` – 2D affine transforms (translation, rotation, scale, skew)
- `TCubicBezierT<T>` – Cubic bezier curve (4 control points)
- `TCurveT<T>` – Piecewise cubic bezier path (multiple segments)
- `TQuadraticBezierT<T>` – Quadratic bezier (3 control points)

---

### 4. **traster/** – Raster Image Data Structures
**Responsibility:** Bitmap storage, pixel buffers, raster type system, memory management  
**Key Headers:**
- `traster.h` – Main raster class and subtypes
- `trastercm.h` – Color-mapped raster (indexed color)

**Key Source Files:**
- `traster.cpp` – Raster implementation (buffer management, locking, tiling)

**Key Types:**
- `TRaster` – Abstract base for bitmap storage
  - Subclasses: `TRaster32`, `TRaster64`, `TRasterCM8`, `TRasterGR8`, etc.
  - Features: Copy-on-write semantics, lock count, parent/sub-raster support
- `TRasterP` – Smart pointer to TRaster (reference-counted)
- `TRasterPT<T>` – Template smart pointer with pixel type info
- `TRasterType` – Runtime type identifier for rasters

**Raster Operations:** Pixel access, wrapping, tiling, locking, linearization checks

---

### 5. **timage/** – Image Types & Levels
**Responsibility:** Level containers, frame management, image caching, level hierarchy  
**Key Headers:**
- `timage.h` – Image container (TImage wraps TRaster with metadata)
- `tlevel.h` – Level type (TLevel wraps multiple frames)
- `tcontenthistory.h` – Frame history/frame variation support

**Key Source Files:**
- `timage.cpp` – Image implementations
- `tlevel.cpp` – Level implementations
- `tcontenthistory.cpp` – Content history management

**Key Types:**
- `TImage` – Single frame with raster data and metadata
- `TImageP` – Smart pointer to TImage
- `TLevel` – Collection of frames (raster level or vector level)
- `TLevelP` – Smart pointer to TLevel
- `TFrameId` – Frame identifier (frame number or string name)
- `TContentHistory` – Frame variation tracking

---

### 6. **tsystem/** – System Utilities & File Management
**Responsibility:** File paths, logging, plugin management, memory management, platform abstractions  
**Key Headers:**
- `tfilepath.h` – File path manipulation and utilities
- `tfilepath_io.h` – File path I/O and streaming
- `tfiletype.h` – File type detection and extension management
- `tsystem.h` – System-level functions (time, process, memory, user info)
- `tlogger.h` – Logging system
- `tpluginmanager.h` – Dynamic plugin loading and management
- `tbigmemorymanager.h` – Large memory buffer management (Windows 64-bit)

**Key Source Files:**
- `tfilepath.cpp` – Path parsing and normalization
- `tfilepath_io.cpp` – Path streaming
- `tfiletype.cpp` – File type handling
- `tsystem.cpp` – System functions
- `tsystempd.cpp` – Platform-dependent system functions
- `tlogger.cpp` – Logging implementation
- `tpluginmanager.cpp` – Plugin loading (dlopen/LoadLibrary)
- `tbigmemorymanager.cpp` – Large allocation management
- `uncpath.cpp` – UNC path handling (Windows network paths)
- `cpuextensions.cpp` – CPU feature detection (SSE, AVX, etc.)

**Key Types:**
- `TFilePath` – Cross-platform file path abstraction
- `TFileType` – File type identifier with extensions
- `TLogger` – Logging sink with levels (DEBUG, INFO, WARNING, ERROR)
- `TSystemInfo` – System information (memory, CPUs, user, time)

---

### 7. **tvectorimage/** – Vector Image Data & Operations
**Responsibility:** Vector shapes, strokes, regions, vectorization, approximate rendering  
**Key Headers:**
- `tvectorimage.h` – Vector image container and API
- `tstroke.h` – Stroke (polyline) representation and operations
- `tstrokeoutline.h` – Stroke outline (thick/thin boundary)
- `tregion.h` – Region (closed area) representation
- `tregionprop.h` – Region properties (paint attributes)
- `tstrokeprop.h` – Stroke properties (color, thickness)
- `tstrokeutil.h` – Stroke manipulation utilities
- `tstrokedeformations.h` – Stroke deformation operations (bend, bulge, twist)
- `tsegmentadjuster.h` – Segment adjustment for alignment
- `tl2lautocloser.h` – Line-to-line auto-closing algorithm

**Key Source Files:**
- `tvectorimage.cpp` – Vector image container
- `tstroke.cpp` – Stroke implementation
- `tstrokeoutline.cpp` – Stroke outline generation
- `tregion.cpp` – Region implementation
- `tregionprop.cpp` – Region properties
- `tstrokeprop.cpp` – Stroke properties
- `tstrokeutil.cpp` – Stroke utilities (simplification, resampling)
- `tstrokedeformations.cpp` – Deformation operations
- `cornerdetector.cpp` – Corner/endpoint detection
- `drawutil.cpp` – Drawing utilities
- `tcomputeregions.cpp` – Region flood-fill algorithm
- `tsegmentadjuster.cpp` – Segment adjustment
- `outlineApproximation.cpp` – Approximate outline rendering
- `tl2lautocloser.cpp` – Auto-closure algorithm

**Key Types:**
- `TVectorImage` – Vector graphic container (strokes + regions)
- `TVectorImageP` – Smart pointer to TVectorImage
- `TStroke` – Polyline stroke with velocity/pressure info
- `TStrokeOutline` – Thick stroke boundary (for coverage)
- `TRegion` – Closed area (series of stroke segments)
- `TFilledRegionSet` – Collection of connected regions

---

### 8. **tgl/** – OpenGL Integration & Rendering
**Responsibility:** OpenGL context management, display lists, rendering utilities  
**Key Headers:**
- `tgl.h` – OpenGL wrapper functions and utilities
- `tgldisplaylistsmanager.h` – Display list caching
- `tstencilcontrol.h` – Stencil buffer manipulation

**Key Source Files:**
- `tgl.cpp` – OpenGL wrapper implementations
- `tgldisplaylistsmanager.cpp` – Display list manager
- `tstencilcontrol.cpp` – Stencil control

**Key Functions & Types:**
- `tglDraw*()` – OpenGL drawing commands (points, lines, triangles)
- `tglMultMatrix()`, `tglTranslate()`, etc. – Transform wrappers
- `TGLDisplayListsManager` – Manages GPU display list caching
- `TStencilControl` – Stencil buffer state management

---

### 9. **tsound/** – Audio I/O & Playback
**Responsibility:** Sound buffer management, audio file I/O (WAV, AIFF), platform-specific playback  
**Key Headers:**
- `tsound.h` – Sound API and buffer types
- `tsound_t.h` – Sound template types
- `tsound_io.h` – Sound I/O (read/write)
- `tsoundsample.h` – Individual sample and sample buffer types
- `tsop.h` – Sound operations (mixing, resampling, etc.)

**Key Source Files:**
- `tsound.cpp` – Sound buffer and playback
- `tsound_io.cpp` – Audio format I/O
- `tsound_qt.cpp` – Qt-based playback (macOS/Linux)
- `tsound_nt.cpp` – Windows DirectSound playback
- `tsop.cpp` – Sound operations (mixing, level adjustment)

**Key Types:**
- `TSoundTrack` – Audio data container (interleaved samples, sample rate)
- `TSoundTrackP` – Smart pointer to TSoundTrack
- `TSoundSample` – Single audio sample (float or int)
- `TSoundRenderQueue` – Queue of audio frames for playback

---

### 10. **timage_io/** – Image Format I/O
**Responsibility:** High-level image file I/O (raster + vector level loading/saving)  
**Key Headers:**
- `timage_io.h` – Image I/O API
- `tlevel_io.h` – Level I/O (multi-frame sequences)

**Key Source Files:**
- `timage_io.cpp` – Image read/write dispatcher
- `tlevel_io.cpp` – Level sequence I/O

**Key Functions:**
- `TImageReader`, `TImageWriter` – Codec-agnostic I/O abstraction
- Format detection and delegating to specific codec (JPEG, PNG, TIFF, etc.)

---

### 11. **trasterimage/** – Raster-Specific Image Containers
**Responsibility:** Raster-specific image wrapping, codec management, caching  
**Key Headers:**
- `trasterimage.h` – Raster image container
- `tcodec.h` – Compression codec interface (tile-based codecs)

**Key Source Files:**
- `trasterimage.cpp` – Raster image implementations
- `tcodec.cpp` – Codec management

**Key Types:**
- `TRasterImage` – Wraps TRaster with metadata and loading state
- `TRasterImageP` – Smart pointer
- `TCodec` – Abstract compression codec interface

---

### 12. **tvrender/** – Vector Rendering & Tessellation
**Responsibility:** Vector-to-raster rendering, anti-aliasing, tessellation, color styles  
**Key Headers:**
- `tofflinegl.h` – Offline OpenGL rendering (CPU-based)
- `qtofflinegl.h` – Qt-based offline GL wrapper
- `ttessellator.h` – Polygon tessellation (ear-clipping)
- `tvectorgl.h` – OpenGL-based vector rendering
- `tcolorstyles.h` – Paint/pattern styles for strokes/regions
- `tsimplecolorstyles.h` – Basic solid/gradient/pattern styles
- `tvectorbrushstyle.h` – Brush texture styles
- `tstrokedeformations.h` – Deformations (bend, bulge, wave, etc.) [also in tvectorimage]
- `tvectorrenderdata.h` – Render data caching for vectors
- `tellipticbrush.h` – Elliptical brush shape definitions
- `tpalette.h` – Palette (color table + style assignment)
- `tpaletteutil.h` – Palette utilities
- `tinbetween.h` – Morph/tween interpolation

**Key Source Files:**
- `tofflinegl.cpp` – Offline GL implementation
- `qtofflinegl.cpp` – Qt offline GL
- `ttessellator.cpp` – Tessellation algorithm
- `tvectorbrush.cpp` – Brush stroke rasterization
- `tvectorbrushstyle.cpp` – Brush style rendering
- `tcolorstyles.cpp` – Style rendering (solid, gradient, texture)
- `tsimplecolorstyles.cpp` – Simple style implementation
- `tpalette.cpp` – Palette management
- `tpaletteutil.cpp` – Palette utilities
- `tinbetween.cpp` – Morphing interpolation
- `tstrokedeformations.cpp` – Deformation rendering
- `tstrokeprop.cpp` – Stroke property rendering
- `tstrokeutil.cpp` – Stroke rendering utilities
- `tellipticbrush.cpp` – Brush shape rasterization
- `tfont_qt.cpp` – Text rendering via Qt
- `tglcurves.cpp` – Curve rendering in OpenGL
- `tglregions.cpp` – Region rendering in OpenGL
- `tregionprop.cpp` – Region property rendering

**Key Types:**
- `TOutlineStyle` – Base class for stroke/region styles (solid, gradient, etc.)
- `TColorStyle` – Concrete style (TPixelPattern, TLinearGradient, etc.)
- `TPalette` – Color palette + style mapping
- `TTessellator` – Polygon triangulation
- `TOGLRenderData` – Cached render data for vectors
- `TTextStyle` – Text rendering style

---

### 13. **trop/** – Raster Operations Library
**Responsibility:** Pixel-level raster processing (blur, morphology, color conversion, etc.)  
**Key Headers:**
- `trop.h` – Main raster operations API
- `trop_borders.h` – Border/padding operations

**Key Source Files:**
- `trop.cpp` – Core operations
- `trop_borders.cpp` – Border handling
- `tropcm.cpp` – Color-mapped raster operations
- `tblur.cpp` – Gaussian and box blur
- `tantialias.cpp` – Anti-aliasing filters
- `tcheckboard.cpp` – Checkerboard pattern generation
- `tconvert.cpp` – Pixel format conversion
- `tconvolve.cpp` – General convolution
- `tdespeckle.cpp` – Noise removal
- `tdistancetransform.cpp` – Distance transform
- `terodilate.cpp` – Erosion/dilation morphology
- `tinvert.cpp` – Color inversion
- `toperators.cpp` – Basic arithmetic operators (add, subtract, multiply)
- `tover.cpp` – Over/compositing operator
- `traylit.cpp` – Ray-traced lighting
- `tresample.cpp` – Image resampling/scaling
- `trgbmscale.cpp` – RGB/matte channel scaling
- `quickput.cpp` – Fast pixel writing
- `brush.cpp` – Brush stroke rasterization
- `bbox.cpp` – Bounding box computation
- `runsmap.cpp` – Run-length encoding for regions
- `tfracmove.cpp` – Fractional pixel movement

**Key Types:**
- `TRop` – Namespace of static raster operation functions
  - `TRop::blur()`, `TRop::erode()`, `TRop::dilate()`, `TRop::convert()`, etc.

---

### 14. **tstream/** – Stream Serialization & I/O
**Responsibility:** Persistent storage, file streaming, endianness handling  
**Key Headers:**
- `tstream.h` – Stream classes (read/write, binary/text)
- `tstreamexception.h` – Stream error handling
- `tpersist.h` – Persistent object base class (save/load)
- `tpersistset.h` – Collection of persistent objects

**Key Source Files:**
- `tstream.cpp` – Stream implementations (file I/O, memory buffers)
- `tstreamexception.cpp` – Exception handling
- `tpersistset.cpp` – Persistent collection implementation

**Key Types:**
- `TOStream` – Binary output stream (file or memory)
- `TIStream` – Binary input stream (file or memory)
- `TXMLStream` – XML-based stream (text format)
- `TPersist` – Base class for saveable objects (abstract)
- `TPersistSet` – Collection of TPersist objects (e.g., palette, level)

---

### 15. **tiio/** – Image Format Codec Support
**Responsibility:** Image format readers/writers (JPEG, BMP, PNG metadata, etc.)  
**Key Headers:**
- `tiio.h` – Codec registry and file I/O coordination
- `tiio_bmp.h` – BMP format support
- `tiio_jpg.h` – JPEG format support
- `tiio_jpg_exif.h` – JPEG EXIF metadata
- `tiio_jpg_util.h` – JPEG utilities
- `tiio_std.h` – Standard format shortcuts

**Key Source Files:**
- `tiio.cpp` – Codec dispatcher
- `tiio_bmp.cpp` – BMP codec
- `tiio_jpg.cpp` – JPEG codec
- `tiio_jpg_util.cpp` – JPEG utilities
- `tiio_jpg_exif.cpp` – EXIF parsing
- `movsettings.cpp` – MOV (QuickTime) settings

**Key Types:**
- `TImageReader` – Abstract reader interface (delegated to format-specific codecs)
- `TImageWriter` – Abstract writer interface
- Format-specific readers: `TBmpReader`, `TJpgReader`, etc.

---

### 16. **tipc/** – Inter-Process Communication
**Responsibility:** Network IPC, 32-bit rendering server wrapper  
**Key Headers:**
- `tipc.h` – IPC server/client API
- `tipcmsg.h` – Message protocol definitions
- `tipcsrv.h` – IPC server implementation
- `tipcsrvP.h` – IPC server private details
- `t32bitsrv_wrap.h` – 32-bit server wrapper (Windows)

**Key Source Files:**
- `tipc.cpp` – IPC client/server
- `tipcmsg.cpp` – Message serialization
- `tipcsrv.cpp` – Server implementation
- `t32bitsrv_wrap.cpp` – 32-bit server wrapper

**Key Types:**
- `TIpcServer` – Local server (TCP/IP, socket communication)
- `TIpcClient` – Remote client connection
- `TIpcMessage` – Message payload

---

### 17. **psdlib/** – Photoshop Format Support
**Responsibility:** PSD file format parsing and layer extraction  
**Key Headers:**
- `psd.h` – PSD file format API

**Key Source Files:**
- `psd.cpp` – PSD parser implementation
- `psdutils.cpp` – PSD utilities (layer extraction, blending)

**Key Types:**
- `PSD` – PSD file reader (layer hierarchy, raster data)

---

### 18. **Specialized Modules (tmeshimage, tmetaimage, tapptools, etc.)**

**tmeshimage/** – Mesh image data (for mesh deformation)
- `tmeshimage.h` – Mesh container with vertex/face data
- `tmeshimage.cpp` – Implementation

**tmetaimage/** – Meta-image (ancillary data)
- `tmetaimage.h` – Meta-data container
- `tmetaimage.cpp` – Implementation

**tapptools/** – Application utilities
- `tenv.cpp` – Environment variable management

**tunit/** – Unit system (measurements)
- Unit definitions for distance, time, frequency

**ttest/** – Testing utilities
- Test harness and assertion macros

**ttoonzimage/** – Toonz-specific raster format
- `ttoonzimage.h` – TZP/TZL format (Toonz proprietary)
- `ttonzimage.cpp` – TZP/TZL codec

**tcache/** – Cache management
- `timagecache.cpp` – Level/image caching

---

## Core Type Hierarchy

### Inheritance Tree (Simplified)

```
TSmartObject (base for reference counting)
├── TRaster (bitmap storage)
│   ├── TRaster32 (32-bit RGBA)
│   ├── TRaster64 (64-bit RGBA)
│   ├── TRasterCM8 (8-bit indexed)
│   ├── TRasterGR8 (8-bit grayscale)
│   └── ... (other pixel formats)
├── TImage (single frame)
├── TLevel (multi-frame sequence)
├── TVectorImage (vector graphic)
└── TPersist (serializable objects)
    ├── TLevel
    ├── TVectorImage
    ├── TLevel (again, supports persistence)
    └── ... (all saveable objects)

TAffineT<T> (template, no inheritance)
├── TAffine (2D int affine)
└── TAffineD (2D double affine)

TPointT<T> (template)
├── TPoint (2D int point)
└── TPointD (2D double point)

TRectT<T> (template)
├── TRect (2D int rect)
└── TRectD (2D double rect)

TCubicBezierT<T> (template)
├── TCubicBezier (int-based)
└── TCubicBezierD (double-based)

TPixel types (no inheritance, value semantics)
├── TPixelRGBM32
├── TPixelRGBM64
├── TPixelD (double-precision)
├── TPixelGR8
├── TPixelGR16
└── ... (various packed formats)
```

---

## Data Flow Summary

### Typical Raster Processing Pipeline
1. **Load** → `TImageReader` → raster data file
2. **Decode** → `tiio_jpg`, `tiio_png`, etc. → `TRaster` object
3. **Wrap** → `TImage` (with metadata)
4. **Process** → `TRop::blur()`, `TRop::erode()`, etc. → modified raster
5. **Render** → `TGL::drawPixels()` or `TOGLRenderData` → display
6. **Save** → `TPersist::save()` or `TImageWriter` → disk

### Typical Vector Processing Pipeline
1. **Load** → `TVectorImageP::load()` → `TVectorImage` object
2. **Edit** → `TStroke` manipulation, region painting
3. **Render** → `TVectorImage::render()` → offline GL rasterization → `TRaster`
4. **Composite** → merge with raster layers
5. **Save** → `TPersist::save()` → disk

---

## Key Statistics

| Aspect | Value |
|--------|-------|
| **Total Source Directories** | 18+ |
| **Total Header Files** | 150+ |
| **Total Source Files** | 120+ |
| **Primary Pixel Formats Supported** | 8+ (32-bit, 64-bit, grayscale, color-mapped) |
| **Geometry Template Specializations** | 2 (int, double) for points, rects, affines |
| **Raster Operations** | 20+ (blur, morphology, convert, composite, etc.) |
| **Image Formats in tnzcore** | JPEG, BMP, PNG, TIFF (delegated to image/ library) |
| **Core Threading Model** | Qt-based (QThread, QMutex, signals/slots) |
| **Audio Formats** | WAV, AIFF, raw (FFmpeg delegated to sound/ library) |

---

## Exports & Public API Surface

All modules export their types via `add_definitions()` in CMakeLists.txt:
- `-DTNZCORE_EXPORTS` → tnzcore module visibility
- `-DTCOLOR_EXPORTS` → color/pixel types
- `-DTGEOMETRY_EXPORTS` → geometry types
- `-DTRASTER_EXPORTS` → raster types
- `-DTIMAGE_EXPORTS` → image types
- `-DTSYSTEM_EXPORTS` → system utilities
- `-DTVECTORIMAGE_EXPORTS` → vector types
- `-DTGL_EXPORTS` → OpenGL wrappers
- `-DTSOUND_EXPORTS` → audio types
- `-DTIMAGE_IO_EXPORTS` → image I/O
- `-DTRASTERIMAGE_EXPORTS` → raster image wrappers
- `-DTMETAIMAGE_EXPORTS` → meta-image types
- `-DTVRENDER_EXPORTS` → vector rendering
- `-DTFLASH_EXPORTS` → Flash format (legacy)
- `-DTROP_EXPORTS` → raster operations
- `-DTSTREAM_EXPORTS` → stream serialization
- `-DTSTRINGID_EXPORTS` → string ID pool
- `-DTVARIANT_EXPORTS` → variant type

---

## Platform-Specific Variations

**Sound I/O (platform-dependent):**
- Windows → `tsound_nt.cpp` (DirectSound via Qt)
- macOS/Linux → `tsound_qt.cpp` (Qt multimedia)

**System Functions (platform-dependent):**
- `tsystempd.cpp` – Platform-specific system info
- `uncpath.cpp` – Windows UNC path support
- `cpuextensions.cpp` – CPU feature detection (SSE, AVX)

**Offline GL:**
- `qtofflinegl.cpp` – Qt-based CPU-side GL rendering

---

## Dependencies Summary

### External Dependencies (from CMakeLists.txt)
- **Qt5** → Core, OpenGL, Network, Multimedia, Gui
- **OpenGL** → GL, GLU, GLUT
- **Graphics** → JPEG (libjpeg), Z (zlib), LZ4
- **System** → AudioUnit, AudioToolbox (macOS); DbgHelp (Windows); GLEW

### Internal Dependencies (within tnzcore)
- All modules linked into single `libtnzcore.so/.dll`
- No inter-module dependencies (monolithic library)

### Downstream Dependencies (libraries that depend on tnzcore)
- **tnzbase** → FX framework, parameter system
- **tnzext** → Deformations, linear algebra
- **toonzlib** → Scene management, xsheet, rendering
- **image** → Format-specific codecs
- **sound** → Audio file formats
- **colorfx** → Color effects
- **All higher layers**

---

## File Organization Summary

```
toonz/sources/
├── include/                     # Public headers (150+ files)
│   ├── tpixel.h, tpixelgr.h    # Pixel types
│   ├── traster.h               # Raster types
│   ├── timage.h, tlevel.h      # Image/level types
│   ├── tgeometry.h, tcurves.h  # Geometry types
│   ├── tsystem.h, tfilepath.h  # System utilities
│   ├── tvectorimage.h          # Vector types
│   ├── tgl.h                   # OpenGL wrapper
│   ├── tsound.h                # Audio types
│   ├── tstream.h, tpersist.h   # Serialization
│   ├── tiio.h                  # Image I/O
│   ├── trop.h                  # Raster operations
│   ├── tpalette.h              # Color styles
│   └── ...
├── common/                      # Implementation (source files)
│   ├── tcore/                  # Core types, threading, undo
│   ├── tcolor/                 # Pixel types, color math
│   ├── tgeometry/              # Geometry implementations
│   ├── traster/                # Raster implementation
│   ├── timage/                 # Image container
│   ├── tsystem/                # System utilities
│   ├── tvectorimage/           # Vector graphics
│   ├── tgl/                    # OpenGL wrappers
│   ├── tsound/                 # Audio I/O
│   ├── timage_io/              # High-level I/O
│   ├── trasterimage/           # Raster image wrapper
│   ├── tvrender/               # Vector rendering
│   ├── trop/                   # Raster operations
│   ├── tstream/                # Serialization
│   ├── tiio/                   # Format codecs
│   ├── tipc/                   # IPC
│   ├── psdlib/                 # PSD format
│   ├── tmeshimage/             # Mesh data
│   ├── tmetaimage/             # Meta-image data
│   └── ...
└── tnzcore/                    # Empty (CMakeLists.txt only)
    └── CMakeLists.txt          # Build configuration
```

---

## Conclusion

The tnzcore type system is a comprehensive, well-organized foundation for raster/vector graphics processing in OpenToonz. It provides:

1. **Core data types** – Pixels, rasters, images, vectors, geometry (150+ public types)
2. **Low-level I/O** – File paths, streams, serialization
3. **Graphics infrastructure** – OpenGL wrappers, offline rendering, tessellation
4. **Audio support** – Cross-platform sound I/O
5. **Concurrency** – Threading, IPC, async messaging
6. **Platform abstraction** – Windows/macOS/Linux portability

All source files are contained in `toonz/sources/common/` across 18+ focused modules, with a unified library export via `tnzcore/CMakeLists.txt`.
