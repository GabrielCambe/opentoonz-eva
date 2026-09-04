# Infinite Background Widget: Decision Flow and Architecture

## Guidance Resolution

### 1. Nature of the "Infinite" Canvas
- **Recommendation:** Implement a dockable panel with an arbitrarily large, fixed-size `QImage` (e.g. 10000x10000 pixels) acting as the raster canvas.
- **Why:** Creating a truly "infinite" raster requires complex tile management, and a vector approach might not suit users who want to paint raster backgrounds. A massive fixed-size `QImage` is simple to implement for a V1 MVP, handles panning/zooming via a `QGraphicsView` or custom paint event, and can be easily cropped or saved to disk.
- **Confidence:** medium
- **Assumptions:** A 10000x10000 image uses ~400MB of RAM, which is acceptable for a modern desktop application for a dedicated background painting feature.
- **Alternatives considered:** Vector canvas (complex to implement full painting tools), dynamically growing tiled raster (too complex for MVP).
- **Next step:** Implement the drawing widget backed by a large `QImage`.

### 2. UI Location
- **Recommendation:** Implement it as a standard OpenToonz Dock Panel (`TPanel`) and register a command in the Windows menu.
- **Why:** This matches the existing UI paradigms (e.g., Palette Viewer, Scene Viewer) and allows the user to dock the background viewer side-by-side with the main scene.
- **Confidence:** high
- **Assumptions:** Users want it to stay open alongside their timeline/xsheet.
- **Alternatives considered:** A modal popup (blocks workflow).
- **Next step:** Subclass `TPanel`, register `MI_InfiniteBackground` in `menubarcommandids.h`, and add it to `mainwindow.cpp` panel factory.

## Architecture Plan
1. **InfiniteBackgroundWidget**: A subclass of `QWidget` (or `QGraphicsView`) that handles mouse events to paint onto the internal `QImage` canvas. It will overlay the camera bounding box using `ViewerDraw::drawCameraMask()`.
2. **InfiniteBackgroundPanel**: A subclass of `TPanel` that wraps the widget.
3. **Export/Import**: A button on the panel that triggers a file save dialog for the `QImage`, saves it as `.png` or `.tif`, and then uses `IoCmd::loadResources` to import it into the scene.
