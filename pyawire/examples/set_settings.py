import awire
from awire import AwPropertyCode, AwFocusMode, AwDialMode

# Set camera settings given a dictionary of settings to set
def set_settings(settings):
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

            # Set the exposure mode firstm
            if 'exposure_program' in settings:
                exp_program = control.get_property_by_code(AwPropertyCode.EXPOSURE_PROGRAM_MODE)
                if exp_program is not None:
                    exp_program.set_value(settings['exposure_program'])

            for setting_name, setting_value in settings.items():
                if setting_name == 'exposure_program':
                    continue
                prop = control.get_property_by_id(setting_name)
                if prop is None:
                    try:
                        prop_code = AwPropertyCode[setting_name.upper()]
                        prop = control.get_property_by_code(prop_code)
                    except KeyError:
                        pass
                if prop is None:
                    awire.log_warn(f"Could not find property \'{setting_name}\'")
                else:
                    awire.log_info(f"Setting property \'{setting_name}\' to \'{setting_value}\'")
                    prop.set_value(setting_value)

            device.close()

    device_list.close()

def main():
    settings = {
        "exposure_program": AwFocusMode.MANUAL,
        "iso": 100,
        "shutter_speed": "1/10",
        "f_number": "4.0"
    }
    set_settings(settings)

if __name__ == '__main__':
    awire.log_set_level(awire.AwLogLevel.INFO)
    main()
