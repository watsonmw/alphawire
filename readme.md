# AlphaWire

A C library for controlling Sony Alpha cameras via USB connection, with a focus on minimal dependencies and broad camera support.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview

AlphaWire provides both low-level and high-level APIs for controlling Sony Alpha cameras. It's particularly useful for:
- Controlling Alpha cameras with minimal dependencies
- Supporting cameras not covered by official Sony Camera Remote SDK
- Debugging PTP (Picture Transfer Protocol) implementation

> **Note**: This project is not affiliated with or endorsed by Sony. 'Sony' and 'Alpha' are trademarks of Sony Corporation.

## Current Status

- **Platform Support**: Currently Windows-only (MinGW compiler required)
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
   - CMake/Makefile targets for library
   - Packaged releases for UI application
- [ ] Platform Extensions
   - Linux support
   - macOS support
   - TCP backend implementation
- [ ] API Development
   - Python bindings (cffi/ctypes)
   - Enhanced high-level API
