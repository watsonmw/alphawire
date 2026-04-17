UI
===

- Connect multiple cameras
- Detect transport errors and disconnect
- Button list and control 
  - Add buttons UI

API
===

- Settings apply from JSON file
  - Save settings to JSON file
  - Load/Apply settings from JSON file
  - API updates
    - Str setter
      - Str: set directly
    - int setter
      - Enum: Set based on index or value?
      - Int Range: set directly if in range


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