import argparse
import sys
import time
import typing

import awire
from awire import AwPropertyCode, AwFocusMode, AwDialMode, AwExposureProgramMode, AwSonyProtocolVersion, AwIso, \
    AwControl


def _apply_settings(control: AwControl, settings: typing.Dict[str, typing.Any], already_set: typing.Set[str]):
    # Set the settings
    props_to_check = []
    for setting_name, setting_value in settings.items():
        if setting_name in already_set:
            continue
        if setting_name == 'exposure_program':
            continue
        prop = control.get_property(setting_name)
        if prop is None:
            awire.log_warn(f"Could not find \'{setting_name}\'")
        else:
            props_to_check.append((prop.code, setting_name, setting_value))
            value = prop.get_value()
            if isinstance(setting_value, str):
                value = prop.get_value_as_str()
            if value != setting_value:
                if not prop.is_writable():
                    awire.log_info(f"\'{setting_name}\' is read-only - will try to set anyway...")
                awire.log_info(f"\'{setting_name}\' setting to \'{setting_value}\' was \'{value}\'")
            else:
                awire.log_info(f"\'{setting_name}\' already set to \'{setting_value}\'")
            prop.set_value(setting_value)

    # Wait for camera to update settings
    time.sleep(0.5 if control.get_protocol_version() == AwSonyProtocolVersion.V3 else 1.0)
    control.refresh_properties()

    # Check if any settings didn't apply
    didnt_update_settings = []
    for (prop_code, setting_name, setting_value) in props_to_check:
        prop = control.get_property(prop_code)
        if prop is not None:
            value = prop.get_value()
            # Make sure we convert to string if we set with a string (e.g. for iso you can set with int or str)
            if isinstance(setting_value, str) and not isinstance(value, str):
                value = prop.get_value_as_str()
            if value != setting_value:
                didnt_update_settings.append(
                    (setting_name, f"   \'{setting_name}\' set to: \'{value}\' expected: \'{setting_value}\'"))
            else:
                already_set.add(setting_name)

    return didnt_update_settings


def _is_target_value_missing_from_enums(control, prop_not_set, target_value):
    prop = control.get_property(prop_not_set)
    enums = prop.get_enums()
    if len(enums) == 0:
        return False
    if isinstance(target_value, str):
        for enum_value, enum_str in enums:
            if enum_str == target_value:
                return False
    else:
        for enum_value, enum_str in prop.get_enums():
            if enum_value == target_value:
                return False
    return True


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

        device, result = device_list.open_device(device_info)
        if device is not None:
            control = device.open_control()
            control.connect()

            control.wait_for_writable()

            # Override the dial mode while connected
            dial_mode = control.get_property_by_code(AwPropertyCode.DIAL_MODE)
            if dial_mode is not None:
                awire.log_info(f"\'dial_mode\' setting to \'{AwDialMode.REMOTE}\'")
                dial_mode.set_value(AwDialMode.REMOTE)

            # Set the exposure mode first
            if 'exposure_program' in settings:
                exp_program = control.get_property_by_code(AwPropertyCode.EXPOSURE_PROGRAM_MODE)
                if exp_program is not None:
                    exp_program_value = settings['exposure_program']
                    awire.log_info(f"\'exposure_program\' setting to \'{exp_program_value}\'")
                    exp_program.set_value(exp_program_value)

            already_set = set()
            props_not_set: list[tuple[str, str]] = []
            retry = 0
            while retry < 4:
                props_not_set = _apply_settings(control, settings, already_set)
                if len(props_not_set) == 0:
                    break
                elif retry < 3:
                    # wait, refresh and retry
                    for prop_not_set, _ in props_not_set:
                        target_value = settings[prop_not_set]
                        if _is_target_value_missing_from_enums(control, prop_not_set, target_value):
                            awire.log_warn(f"\'{prop_not_set}\' doesn't have enum: \'{target_value}\'")
                    if len(props_not_set):
                        awire.log_warn("Settings not updated: " +
                                       ", ".join([setting_name for setting_name, didnt_update_log in props_not_set]) +
                                       ".  Retrying...")
                    time.sleep(1.)
                    control.refresh_properties()
                retry += 1
            if len(props_not_set) > 0:
                awire.log_warn("Settings not updated:")
                for setting_name, didnt_update_log in props_not_set:
                    awire.log_warn(didnt_update_log)

            device.close()

    device_list.close()


SETTINGS_MAP = {
    "1": {
        "exposure_program": AwExposureProgramMode.MANUAL,
        "iso": "100",
        "shutter_speed": "1/10",
        "f_number": "4.0",
        "focus_mode": AwFocusMode.MANUAL,
    },
    "2": {
        "exposure_program": AwExposureProgramMode.MANUAL,
        "iso": "400",
        "shutter_speed": "1/60",
        # "f_number": "10",
        "focus_mode": AwFocusMode.AF_AUTO,
    },
    "3": {
        "exposure_program": AwExposureProgramMode.MANUAL,
        "iso": "400",
        "shutter_speed": "1/60",
        "focus_mode": AwFocusMode.AF_S,
    },
}


def main(selected_profile=None):
    if selected_profile:
        if selected_profile in SETTINGS_MAP:
            set_settings(SETTINGS_MAP[selected_profile])
        else:
            print(f"Error: Profile '{selected_profile}' not found.")


if __name__ == '__main__':
    awire.log_set_level(awire.AwLogLevel.INFO)
    parser = argparse.ArgumentParser(description='Set camera settings')
    parser.add_argument(
        '--set',
        choices=SETTINGS_MAP.keys(),
        help='Select which settings list to apply (e.g., 1, 2 or 3)'
    )
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(0)
    args = parser.parse_args()
    main(selected_profile=args.set)
