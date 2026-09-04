# Infinite Background Widget: Change Summary

## What Changed
- **New Widget:** Created `InfiniteBackgroundWidget` in `infinitebackgroundpanel.h` and `infinitebackgroundpanel.cpp`. This widget provides a 10000x10000 pixel raster canvas that supports drawing strokes, zooming, and panning.
- **Camera Mask:** The widget implements a camera overlay by drawing a red bounding box in the center, sized according to the current OpenToonz camera (`TApp::instance()->getCurrentScene()->getScene()->getCurrentCamera()->getRes()`).
- **Export/Import Integration:** Added an "Export and Auto-Import to Scene" button. When clicked, it asks for a file location, saves the background canvas as an image (e.g., PNG), and automatically imports it into the current OpenToonz scene using `IoCmd::loadResources()`.
- **Dockable Panel:** Wrapped the widget in a `TPanel` via `InfiniteBackgroundPanelFactory` so it can be docked anywhere in the OpenToonz workspace.
- **Menus:** Registered `MI_OpenInfiniteBackgroundPanel` in `menubarcommandids.h` and added the "Infinite Background" menu action to `mainwindow.cpp` and `menubar.cpp` (under the "Windows" -> "Other Windows" or similar menus, exactly matching `HistoryPanel` placement).
- **Build System:** Updated `toonz/sources/toonz/CMakeLists.txt` to compile the new `.h` and `.cpp` files.

## How to Understand the Result
Users can now open the "Infinite Background" panel from the `Windows` menu. They can freely pan (middle-click), zoom (scroll wheel), and draw (left-click). The red rectangle visually represents the camera bounds. Once the drawing is complete, they can export it and it will immediately appear in the xsheet for use in their animation cuts.
