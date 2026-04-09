UI
===

- Settings apply
  - Str setter (match based on enums)
  - int setter (try to find matching enum)
  - bool setter1945
- Build list of property editors / widgets?
  - Debug Set widgets
  - Enum List Read only support (except in debug)
- Remove sorting for other tabs
- Timing / Position
- Connect multiple cameras
- Simple bool setter for properties that have an on/off enabled/disabled state
- Detect transport errors and disconnect
- Button list and control 
  - Add buttons UI
- Add all new controls

Backends
===

- IP: Don't copy packet buffer so much
- IP: Connect via IP only?
- IP: Connect with password (TLS)

Build
===

- Meson build system

Python
===

- Python bindings are broken 
  - consider switching to building alphawirelib statically
  - consider alphawirelib as static only

Documentation
===

- Settings needed to connect to camera.
- PTPIP setup (pairing and disabling encryption)