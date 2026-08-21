import os
from datetime import datetime

import awire


# Capture workflow script
# - Search devices
# - For each device, shoot image and download
def capture_workflow():
    awire.log_info(f"Searching for Alpha cameras...")

    device_list = awire.AwDeviceList()
    ok = device_list.open()
    if not ok:
        awire.log_info("Failed to open device list")
        return

    device_list.refresh()

    while device_list.is_refreshing():
        device_list.poll_updates()
        if len(device_list) > 0:
            awire.log_info(f"Found {len(device_list)} devices")
            break

    if len(device_list) == 0:
        awire.log_info("No devices found!")
        return

    for device_info in device_list:
        awire.log_info(f"{device_info.manufacturer} - {device_info.product} (S/N: {device_info.serial})")

        device = device_list.open_device(device_info)
        if device is not None:
            control = device.open_control()
            control.connect()

            # Capture and download image
            image = control.capture_and_download()
            if image:
                ext = os.path.splitext(image.filename)[1] or '.jpg'
                filename = datetime.now().strftime(f"%Y%m%d_%H%M%S{ext}")
                with open(filename, "wb") as f:
                    f.write(image.data)
                awire.log_info(f"Image captured and downloaded to {filename}")

            device.close()

    device_list.close()


if __name__ == '__main__':
    awire.log_set_level(awire.AwLogLevel.INFO)
    capture_workflow()
    awire.log_info("Done.")
