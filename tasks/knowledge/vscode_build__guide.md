# Building & Running OpenToonz in VS Code Insiders

## The key idea
Your project is **CMake-based**. Visual Studio never compiled it "natively" — CMake generated the
`.sln`/`.vcxproj` files and VS just consumed them. VS Code Insiders can drive the *exact same* CMake
build through the **CMake Tools** extension, using the same MSVC compiler (installed with Visual
Studio) and the same Qt 5.15.2. Nothing about the toolchain changes; only the editor that triggers
it changes.

## One-time setup (already done for you)
The `.vscode/` folder now contains:

- **settings.json** – points CMake Tools at your source (`toonz/sources`) and **reuses your existing
  build folder** `toonz/build`, so it won't reconfigure from scratch. Sets the generator
  (`Visual Studio 18 2026`) and `QT_PATH`.
- **tasks.json** – build tasks (`cmake --build`) for the OpenToonz target and for everything.
- **launch.json** – an F5 debug config using the MSVC debugger (`cppvsdbg`).
- **extensions.json** – recommends the two extensions you need.

## First run
1. Install the recommended extensions when VS Code Insiders prompts you (or from the Extensions
   panel): **CMake Tools** (`ms-vscode.cmake-tools`) and **C/C++** (`ms-vscode.cpptools`).
2. Open the repo folder (`opentoonz-eva`) in VS Code Insiders.
3. If prompted to select a kit, pick your **Visual Studio 2026 – amd64** kit. (It usually just reuses
   the existing cache and skips this.)

## Build
- **Whole solution:** Terminal → Run Build Task (`Ctrl+Shift+B`) → "CMake: build ALL".
- **Just the app:** that's the default build task → "CMake: build OpenToonz".
- Or use the CMake Tools status bar at the bottom: the **Build** button.

Output lands in `toonz/build/Debug/` (same as before).

## Run / Debug
- Press **F5** → "Debug OpenToonz (MSVC)". It builds first, then launches
  `toonz/build/Debug/OpenToonz.exe` under the debugger.
- To run **without** debugging: `Ctrl+F5`, or click ▶ in the CMake Tools status bar.

## Switching Debug/Release
Use the **variant** selector in the CMake Tools status bar (Debug ⇄ Release). The launch config
follows your selected target automatically — no path edits needed.

## If something errors
- *"No kit selected"* → run `CMake: Select a Kit` and choose your Visual Studio amd64 kit.
- *Qt DLL not found at launch* → the launch config already adds `C:/Qt/5.15.2/msvc2019_64/bin` to
  PATH; confirm that path matches your Qt install.
- *Wants to reconfigure* → that's fine; it reuses the cache. To force a clean one,
  `CMake: Delete Cache and Reconfigure`.

## Scope note (kept deliberately tight)
This only changes how you *trigger* the build (VS Code instead of the VS IDE). It does **not** touch
the CMake setup, the toolchain, or any source. You can still open the `.sln` in Visual Studio anytime
— both point at the same `toonz/build`.
