import time

import awire


def list_cameras():
    awire.log_info(f"Searching for Alpha cameras...")

    device_list = awire.AwDeviceList()
    ok = device_list.open()
    if not ok:
        awire.log_info("Failed to open device list")
        return

    device_list.refresh()

    printed_devices = {}
    while device_list.is_refreshing():
        device_list.poll_updates()
        if len(device_list) > 0:
            for device_info in device_list:
                if device_info.serial not in printed_devices:
                    printed_devices[device_info.serial] = True
                    if device_info.usb_vid is not None:
                        extra = f"{device_info.usb_pid}:{device_info.usb_vid} | usb:{device_info.usb_version}"
                    else:
                        extra = f"{device_info.ip_address}"
                    awire.log_info(
                        f"    | {device_info.product} | {device_info.manufacturer} | {device_info.serial} | {extra} |")
        time.sleep(0.1)

    if len(device_list) == 0:
        awire.log_info("No devices found!")
        return

    device_list.close()


if __name__ == '__main__':
    awire.log_set_level(awire.AwLogLevel.INFO)
    list_cameras()
