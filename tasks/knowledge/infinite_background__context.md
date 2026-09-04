# Infinite Background Widget Context

This document captures the findings regarding how OpenToonz draws cameras and imports files, which will be necessary to build the new "infinite" background widget.

## Camera Drawing Logic
In OpenToonz, the camera overlay is drawn using static methods in `ViewerDraw` (found in `viewerdraw.h` / `viewerdraw.cpp`). Specifically:
- `ViewerDraw::drawCamera`
- `ViewerDraw::drawCameraMask`

The `SceneViewer` sets up the matrix `m_drawCameraAff` using the `TAffine` transformation from the current camera in the `TStageObjectTree`. 
```cpp
  TStageObjectId cameraId = xsh->getStageObjectTree()->getCurrentCameraId();
  TStageObject *camera    = xsh->getStageObject(cameraId);
  TAffine cameraPlacement = camera->getPlacement(frame);
  double cameraZ          = camera->getZ(frame);
  m_drawCameraAff = getViewMatrix() * cameraPlacement * TScale((1000 + cameraZ) / 1000);
```
To show the camera inside the new widget, we will need to retrieve the current camera affine in a similar way and apply `ViewerDraw::drawCameraMask`.

## Import / Export Logic
To export the drawn background and import it into the scene:
1. **Export:** We will generate an image or a set of strokes from the new widget, then save it to disk using `TImageWriter` or `ToonzImageUtils::saveLevel`.
2. **Import:** `IoCmd` provides a standard way to load external files into the xsheet. We can construct a `IoCmd::LoadResourceArguments` with the newly saved file path and call `IoCmd::loadResources(args)`. This will automatically handle exposing it in the current xsheet if `args.expose` is true.

## Canvas bounds
OpenToonz normally uses strict bounding boxes defined by `LevelOptions` or camera size. An "infinite" canvas needs to operate dynamically, likely by managing an arbitrarily large internal `TRaster` or managing a `TVectorImage` that doesn't limit stroke bounds, which we then crop during export or auto-resize as the user draws.
