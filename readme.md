# AlphaWire

A C library for controlling Sony Alpha cameras via USB connection, with a focus on minimal dependencies and broad camera
support.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview

AlphaWire provides both low-level and high-level APIs for controlling Sony Alpha cameras.

It has some advantages over the official SDK:
- Minimal dependencies
- Fast image downloads
- Supports older cameras not covered by official Sony Camera Remote SDK
- Easy debugging of camera remote features

Platform Support:
- Windows (Visual Studio 2018 or MinGW)
- OSX (clang)
- Linux (GCC)

Build System:
- CMake

> **Note**: This project is not affiliated with or endorsed by Sony. 'Sony' and 'Alpha' are trademarks or registered
> trademarks of Sony Corporation.


## Library Features

### Core Capabilities
- Comprehensive camera control via low-level and high-level APIs
- Full PTP device property control
- Real-time Live View streaming
- Tethered capture and file download
- Camera settings management (save/load)
- Compatible with both pre-2020 and post-2020 Alpha cameras
- IMGUI application for camera control and debugging

### Windows Backend Support
- WIA (Windows Image Acquisition)
- libusbk
- TCP/IP

Use [Zadig](https://zadig.akeo.ie/) to switch drivers on Windows.

If your device is not showing up, try updating the libusbk driver with Zadig.

### OSX Backend Support
- IOKit
- TCP/IP

May need to close the 'Preview' application before using the camera, as if open it acquires exclusive access to the
camera.

### Linux Backend Support
- libusb
- TCP/IP

## Demo Application
A GUI application is included for camera control and PTP protocol debugging.

### Features
- PTP property/control inspection and modification
- Live view monitoring
- Focus control and image capture
- Camera settings management
- Protocol debugging tools
- Fast image download

### Limitations

- PTPIP backend doesnt support SSL
- Many properties not implemented (missing naming / description - but can still be controlled/inspected)
- No ARW / image processing (outside of scope)

## Roadmap
- [ ] Build System Improvements
   - Packaged releases for UI application
- [ ] API Development
   - Python bindings
   - Descriptions for each property
   - Enhanced high-level API
