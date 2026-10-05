
# awire

Python bindings for [AlphaWire](https://github.com/watsonmw/alphawire) - a C library for Sony Alpha camera tethered control.

## Overview

`awire` provides a convenient Python interface to control Sony Alpha cameras via USB or IP connections. It is based on the AlphaWire C library, which focuses on minimal dependencies and broad camera support.

### Features
- **Fast and lightweight**: Minimal overhead for camera control.
- **Broad Compatibility**: Supports both pre-2020 and post-2020 Sony Alpha cameras.
- **Full Control**: Access to PTP device properties and camera settings.
- **Tethered Capture**: Trigger captures and download images directly.
- **Live View**: Real-time streaming support.

## Installation

```bash
pip install awire
```

## Quick Start

### List Connected Cameras

```python
import awire
import time

def main():
    # Set logging level (optional)
    awire.log_set_level(awire.AwLogLevel.INFO)

    # Initialize device list
    device_list = awire.AwDeviceList()
    if not device_list.open():
        return

    # Start searching for cameras
    device_list.refresh()

    # Wait for discovery (or poll in a loop)
    while device_list.is_refreshing():
        device_list.poll_updates()
        time.sleep(0.1)

    # List found devices
    for device in device_list:
        print(f"Found: {device.manufacturer} {device.product} (S/N: {device.serial})")

    device_list.close()

if __name__ == "__main__":
    main()
```

## Development

To build and install the package in development mode:

1. **Install Build Dependencies**:
   ```bash
   pip install --upgrade build twine cffi
   ```

2. **Build the C extension**:
   ```bash
   python build_extension.py --debug
   ```

3. **Install in editable mode**:
   ```bash
   pip install -e .
   ```

### Building Wheels

```bash
python -m build
```

## Compatibility

The package is built using the Python Stable ABI (Limited API), meaning a single wheel should work on all CPython versions from 3.9 onwards for a given platform.

## License

MIT License. See the main [LICENSE](../LICENSE) file for details.

> **Note**: This project is not affiliated with or endorsed by Sony. 'Sony' and 'Alpha' are trademarks or registered trademarks of Sony Corporation.
