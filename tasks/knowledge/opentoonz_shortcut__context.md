# OpenToonz Shortcut System Context

## How Shortcuts Work

### Command Registration Chain

1. **Command IDs** are `#define` macros in `menubarcommandids.h`:
   ```cpp
   #define MI_CommandName "MI_CommandName"
   ```

2. **Registration** happens in `MainWindow::defineActions()` (`mainwindow.cpp`), using typed helper methods:
   ```cpp
   createMenuXsheetAction(MI_CommandName, QT_TR_NOOP("Display Name"), "DefaultShortcut", "icon_svg_name");
   ```
   Each `createMenu*Action()` calls `createAction()` → `CommandManager::define(id, type, shortcut, action, icon)`.

3. **Handler binding** uses `MenuItemHandler` (from `menubarcommand.h`):
   ```cpp
   class MyCommand : public MenuItemHandler {
   public:
     MyCommand() : MenuItemHandler(MI_CommandName) {}
     void execute() override { /* implementation */ }
   } myCommandInstance;  // static instantiation auto-registers
   ```
   The `MenuItemHandler` constructor calls `CommandManager::setHandler(id, handler)`.

4. **ShortcutPopup** reads `CommandManager::getActions(type)` to populate its tree, grouped by `CommandType` enum.

### CommandType Categories (mapped to Shortcut Popup folders)

| CommandType                    | Shortcut Popup Category |
|-------------------------------|------------------------|
| `MenuFileCommandType`         | File Menu              |
| `MenuEditCommandType`         | Edit Menu              |
| `MenuXsheetCommandType`       | Xsheet Menu            |
| `MenuCellsCommandType`        | Cells Menu             |
| `MenuPlayCommandType`         | Play Menu              |
| `MenuRenderCommandType`       | Render Menu            |
| `MenuViewCommandType`         | View Menu              |
| `RightClickMenuCommandType`   | Right-click Menu       |
| `ToolCommandType`             | Tools                  |
| `ToolModifierCommandType`     | Tool Modifiers         |
| `MiscCommandType`             | Misc                   |

### Context Menu Pattern

Commands already registered with CommandManager can be added to context menus via:
```cpp
CommandManager *cmdManager = CommandManager::instance();
menu->addAction(cmdManager->getAction(MI_CommandName));
```
This automatically shows the assigned shortcut label next to the menu item.

### Key Files

| File | Role |
|------|------|
| `toonz/sources/toonz/menubarcommandids.h` | All command ID macros |
| `toonz/sources/include/toonzqt/menubarcommand.h` | CommandManager, MenuItemHandler, CommandType |
| `toonz/sources/toonz/mainwindow.cpp` | Registration site (defineActions) |
| `toonz/sources/toonz/shortcutpopup.h/cpp` | Configure Shortcuts UI |
| `toonz/sources/toonz/menubar.cpp` | Menu bar structure |

### Pattern for Adding a New Shortcuttable Command

1. Add `#define MI_YourCommand "MI_YourCommand"` to `menubarcommandids.h`
2. In `mainwindow.cpp` → `defineActions()`, call `createMenu*Action(MI_YourCommand, ...)`
3. Create a `MenuItemHandler` subclass with `execute()` override (typically in the relevant .cpp file)
4. If it should appear in a context menu, use `cmdManager->getAction(MI_YourCommand)`
