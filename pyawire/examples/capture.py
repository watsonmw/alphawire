import os
from datetime import datetime

import awire
from awire import AwPropertyCode, AwFocusMode, AwDialMode


# Capture workflow script
# - Find a device
# - Adjust some settings
# - Shoot Image and download
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

            # Override the dial mode while connected
            dial_mode = control.get_property_by_code(AwPropertyCode.DIAL_MODE)
            if dial_mode is not None:
                dial_mode.set_value(AwDialMode.REMOTE)

            # Change the exposure program to manual so we can change capture settings
            exp_program = control.get_property_by_code(AwPropertyCode.EXPOSURE_PROGRAM_MODE)
            if exp_program is not None:
                exp_program.set_value(AwFocusMode.MANUAL)

            # Adjust iso, shutter speed and f-stop
            iso = control.get_property_by_code(AwPropertyCode.ISO)
            if iso is not None:
                iso.set_value(100)
            shutter_speed = control.get_property_by_code(AwPropertyCode.SHUTTER_SPEED)
            if shutter_speed is not None:
                shutter_speed.set_value("1/10")
            f_number = control.get_property_by_code(AwPropertyCode.F_NUMBER)
            if f_number is not None:
                f_number.set_value("4.0")

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
