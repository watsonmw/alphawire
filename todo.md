UI
===

- Build list of property editors / widgets?
  - Debug Set widgets
  - Enum List Read only support (except in debug)
- Remove sorting for other tabs
- Timing / Position
- Connect multiple cameras
- Live View
  - Click to focus
  - Click to zoom
- Settings apply
- Focus Magnifier needs its own API - two sets of properties control the same thing - which one to use depends on
  on the camera.  Maybe good idea to create some virtual properties (id only) for this - disable id for
  magnifier properties.
- Detect transport errors and disconnect

Backends
===

- IP: Don't copy packet buffer so much
- IP: Connect via IP only?
- IP: Connect with password (TLS)
- Split PTPResult and AW results
- Events for libusb and IOKit

Commands
===

Shutdown - 0x9280


