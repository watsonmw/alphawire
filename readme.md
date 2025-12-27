# AlphaWire

A C library for controlling Sony Alpha cameras via USB connection, with a focus on minimal dependencies and broad camera
support.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview

AlphaWire provides both low-level and high-level APIs for controlling Sony Alpha cameras.

It has some advantages over the official SDK:
- Minimal dependencies
- Fast
- Supports older cameras not covered by official Sony Camera Remote SDK
- Easy debugging of camera remote features

Downsides to alphawire:
- Missing backends TCP (on the roadmap)
- Possibly missing metadata for latest cameras (will add as needed)
- No image processing (out of scope for this project)

> **Note**: This project is not affiliated with or endorsed by Sony. 'Sony' and 'Alpha' are trademarks or registered
> trademarks of Sony Corporation.

## Current Status
- **Platform Support**: Currently Windows (Visual Studio 2018 or MinGW), OSX (clang) and Linux
- **Build System**: CMake
- **Architecture**: Designed for easy cross-platform expansion

## Library Features

### Core Capabilities
- Comprehensive camera control via low-level and high-level APIs
- Full PTP device property control
- Real-time Live View streaming
- Tethered capture and file download
- Camera settings management (save/load)
- Compatible with both pre-2020 and post-2020 Alpha cameras

### Windows Backend Support
- WIA (Windows Image Acquisition)
- libusbk

### OSX Backend Support
- IOKit

### Linux Backend Support
- libusb

## Demo Application
A GUI application is included for camera control and PTP protocol debugging.

### Features
- PTP property/control inspection and modification
- Live view monitoring
- Focus control and image capture
- Camera settings management
- Protocol debugging tools
- Fast download of images

## Roadmap
- [ ] Build System Improvements
   - Packaged releases for UI application
- [ ] Platform Extensions
   - TCP backend implementation
- [ ] API Development
   - Threading - support separate thread per device
   - Expand Python bindings
   - Enhanced high-level API
