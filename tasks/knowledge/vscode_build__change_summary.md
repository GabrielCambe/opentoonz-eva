# Change summary: VS Code Insiders build setup

**Goal:** build & run OpenToonz from VS Code Insiders instead of the Visual Studio IDE.

**Why it's easy:** the project is CMake-based; VS only consumed CMake-generated projects. CMake Tools
in VS Code drives the identical build with the same MSVC + Qt 5.15.2 toolchain.

**Files added/changed (`.vscode/`):**
- `settings.json` — reuse existing `toonz/build` cache; generator `Visual Studio 18 2026`; `QT_PATH`.
- `tasks.json` — `cmake --build` tasks (OpenToonz target + ALL).
- `launch.json` — F5 debug via `cppvsdbg`, Qt bin on PATH, builds first.
- `extensions.json` — recommends `ms-vscode.cmake-tools`, `ms-vscode.cpptools`.

**Verified facts (on disk):**
- Canonical build dir: `toonz/build` (most recent cache, Jun 4; has deployed `Qt5*d.dll` next to exe).
- Exe: `toonz/build/Debug/OpenToonz.exe` (exists).
- Generator/Qt confirmed from `toonz/build/CMakeCache.txt`.
- All four JSON configs parse cleanly.

**Scope:** trigger-only change. No source, toolchain, or CMake changes. `.sln` in Visual Studio still works.

**Plan:** `tasks/vscode_insiders_build.xml` (standard-xml, autonomous, low risk).
