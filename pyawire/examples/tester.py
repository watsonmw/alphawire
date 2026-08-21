import os
import time
from datetime import datetime

import awire


# Longer test script
def camera_tester():
    awire.log_info(f"Searching for Alpha cameras...")

    device_list = awire.AwDeviceList()
    ok = device_list.open()
    if not ok:
        print("Failed to open device list")
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
        awire.log_info(f"    {device_info.manufacturer} - {device_info.product} (S/N: {device_info.serial})")
        awire.log_info(f"    IP: {device_info.ip_address}, USB VID: 0x{device_info.usb_vid:04x}, PID: 0x{device_info.usb_pid:04x}, Version: {device_info.usb_version}")

        device = device_list.open_device(device_info)
        if device is not None:
            control = device.open_control()
            control.connect()

            num_properties = control.get_num_properties()
            awire.log_info(f"Found {num_properties} properties")
            
            # Read a few specific properties if they exist
            iso_prop = control.get_property_by_code(awire.AwPropertyCode.ISO)
            if iso_prop:
                awire.log_info(f"ISO Property: {iso_prop.get_label()} = {iso_prop.get_value()} (Raw: {iso_prop.get_value_as_str()})")
                enums = iso_prop.get_enums()
                if enums:
                    awire.log_info(f"  Available ISO values: {[e[1] for e in enums[:5]]}...")

            f_number = control.get_property_by_id("f-number")
            if f_number:
                awire.log_info(f"F-Number: {f_number.get_value()}")

            prop_doesnt_exist = control.get_property_by_id("doesnt-exist")
            if prop_doesnt_exist is not None:
                awire.log_error(f"Expected no property called 'doesnt-exist")

            num_controls = control.get_num_controls()
            awire.log_info(f"Found {num_controls} controls")
            for i in range(min(num_controls, 5)):
                ctrl = control.get_control_at_index(i)
                if ctrl:
                    awire.log_info(f"  Control {i}: {ctrl.label} (Code: 0x{ctrl.code:04x})")

            # Check shutter control exists
            shutter_ctrl = control.get_control_by_code(awire.AwControlCode.SHUTTER)
            if shutter_ctrl:
                awire.log_info(f"Found Shutter control: {shutter_ctrl.label}")

            # Test property setting (non-destructive if possible, or just re-setting current value)
            if iso_prop:
                current_iso = iso_prop.get_value()
                awire.log_info(f"Attempting to re-set ISO to current value: {current_iso}")
                # Note: AwIso.AUTO is a special value, but set_value(int) should handle it if passed as int
                val_to_set = current_iso
                if isinstance(current_iso, awire.AwIso):
                    val_to_set = int(current_iso)

                if iso_prop.set_value(val_to_set):
                    awire.log_info("Successfully set ISO")
                else:
                    awire.log_warn("Failed to set ISO")

            # Test Update Properties
            awire.log_info("Testing RefreshProperties(full_refresh=False)")
            control.refresh_properties(full_refresh=False)

            # Test Pending Files
            pending = control.get_pending_files()
            awire.log_info(f"Pending files on camera: {pending}")

            # Save Live View image
            live_view_image = None
            tries = 0
            while live_view_image is None:
                live_view_image = control.get_live_view_image()
                if live_view_image is not None:
                    filename = "live_view.jpg"
                    awire.log_info(f"Saving live view image to {filename}...")
                    with open(filename, "wb") as f:
                        f.write(live_view_image)
                    break
                time.sleep(.1)
                tries += 1
                if tries >= 10:
                    break

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
    camera_tester()
    awire.log_info("Done.")
