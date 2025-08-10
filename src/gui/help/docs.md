---
**General**

Surkl always starts in Read-Only mode.  Use the lock button at the bottom-left corner to toggle Read-Only mode.  When Read-Only mode is on, writing to the file system is disabled.

---
**Window**

The window can be split vertically or horizontally using the split button of the title bar.  Left-click \+ Drag then drop the title bar to swap windows.

---
**Scenegraph**

| Shortcut | Description |
| :---- | :---- |
| Left click item | Select node or edge |
| Ctrl \+ Left click item | Select multiple items |
| Ctrl \+ Left click \+ drag | Pan the view |
| Alt \+ Left click \+ drag | Zoom in/out |
| B | scene bookmark; left click to position, Delete to delete. |

---
**Scene Bookmark**

A scene bookmark is selected by Left clicking a bookmark item in the scene.  With a selected scene bookmark:

| Shortcut | Description |
| :---- | :---- |
| 1 | Pan to first quadrant |
| 2 | Pan to second quadrant |
| 3 | Pan to third quadrant |
| 4 | Pan to fourth quadrant |
| 5 | Center on the bookmark |

---
**Folders**

A folder can be in one of three states: open, closed, or half-closed.  Closing a parent folder with at least one open or half-closed sub-folder will half-close the parent folder, while holding the Shift button will close the entire sub-tree.

| Action | Sub-Folder | Current State | Next State |
| :---- | :---: | :---: | :---: |
| double-click | N/A | Closed | Open |
| double-click | Closed | Open | Closed |
| double-click | Open | Open | Half-Closed |
| Shift \+ double-click | Open | Open/Half-Closed | Closed |

| Shortcut | Description |
| :---- | :---- |
| D | single-step clockwise rotation (internal) |
| A | single-step counter-clockwise rotation (internal) |
| Shift \+ D | full clockwise rotation (internal) |
| Shift \+ A | Full counter-clockwise rotation (internal) |
| Plus | Grows child nodes (closed folders or files), or self. |
| Shift \+ Plus | Higher rate of growth. |
| Minus | Shrinks child nodes (closed folders or files), or self. |
| Shift \+ Minus | Higher rate of shrinkage. |
| Delete | Press and Hold for 3 seconds to delete a node. |

---
**Move/Copy/Link**

Left click drag an edge item to destination to Move.

Ctrl \+ Left click drag an edge item to destination to Copy.

Ctrl \+ Shift \+ Left click drag an edge item to destination to link.
