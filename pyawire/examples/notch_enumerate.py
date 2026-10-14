import time

import awire
from awire import (AwIso, AwResult, AwResultCode, AwNotchDirection, AwSonyProtocolVersion)

# For pre-2020 cameras some properties can only be set with a up/down notch control
# These properties are some of the most important:
#    f-number, shutter speed and iso
# The cameras often don't provide an enumeration for us to know which values are available
# This script manually enumerates these properties by changing them one value at a time on the camera


def enum_list_pos(enum_list, value):
    for index, (enum_value, enum_str) in enumerate(enum_list):
        if enum_value == value:
            return index
    return -1


def build_enum_list(control, prop, enum_list):
    enums = prop.get_enums()
    if len(enums) > 0:
        awire.log_info(f"Enum list available for property #enums: {len(enums)}")
        for e in enums:
            enum_list.append((e[0], e[1]))
        return enum_list

    raw_value = prop.get_raw_value()
    value_str = prop.get_value_as_str()
    enum_list.append((raw_value, value_str))

    direction = AwNotchDirection.UP
    position = 0

    max_attempts = 200
    attempts = max_attempts
    while attempts >= 0:
        attempts -= 1
        res = prop.notch(direction)
        if res.code != AwResultCode.OK:
            awire.log_warn(f"notch failed for property")
            return res

        # Sleep & refresh properties - if no change wait another bit
        # Retry fresh properties until you see a change in current value
        # Each notch applies to camera setting immediately - but changes take a second or two
        # to be reflected in PTP properties returned by the camera
        last_val = raw_value
        read_attempt = 30
        while raw_value == last_val and read_attempt >= 0:
            time.sleep(0.1)
            control.refresh_properties()
            read_attempt -= 1
            raw_value = int(prop.get_raw_value())

        if read_attempt < 0:
            if direction == AwNotchDirection.UP:
                # assume end of enumeration
                awire.log_info(f"value didn't change, switching direction...")
                direction = AwNotchDirection.DOWN
            else:
                awire.log_info(f"value didn't change (previously checked other direction) done...")
                break

        if direction == AwNotchDirection.UP:
            position += 1
        else:
            position -= 1

        value_str = prop.get_value_as_str()
        awire.log_info(f"   '{value_str}': {raw_value}")
        pos = enum_list_pos(enum_list, raw_value)
        if pos < 0:
            if position < 0:
                enum_list.insert(0, (raw_value, value_str))
            else:
                if position > len(enum_list) - 1:
                    enum_list.append((raw_value, value_str))
                else:
                    enum_list[position] = (raw_value, value_str)
        elif direction == AwNotchDirection.DOWN:
            position = pos
        else:
            awire.log_info(f"Detected looping., done...")
            break

    return AwResult(AwResultCode.OK)



def main():
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
            control: AwControl = device.open_control()
            control.connect()

            notch_settings = []
            settings_enum = {}

            if control.get_protocol_version() != AwSonyProtocolVersion.V2:
                awire.log_info(f"Not connected with protocol 2 - notch not needed")
                continue

            for prop_index in range(control.get_num_properties()):
                prop = control.get_property_at_index(prop_index)
                if prop is None:
                    continue
                if prop.is_notch():
                    awire.log_info(f"Found notch setting \'{prop.get_id()}\'")
                    notch_settings.append(prop.get_id())

            # Set the settings
            for setting_name in notch_settings:
                enum_list = []
                prop = control.get_property(setting_name)
                if prop is None:
                    awire.log_warn(f"Could not find \'{setting_name}\'")
                else:
                    settings_enum[setting_name] = enum_list
                    awire.log_info(f"checking \'{setting_name}\':")
                    build_enum_list(control, prop, enum_list)

            for setting_name, enum_list in settings_enum.items():
                awire.log_info(f"\'{setting_name}\':")
                if setting_name == 'iso':
                    # ISO loops so won't be sorted
                    enum_list.sort(key=lambda x: x[0])
                    if enum_list[-1][0] == AwIso.AUTO:
                        enum_list.insert(0, enum_list.pop())
                for enum_item in enum_list:
                    awire.log_info(f"  {enum_item[0]} \'{enum_item[1]}\'")

            device.close()

    device_list.close()


if __name__ == '__main__':
    awire.log_set_level(awire.AwLogLevel.INFO)
    main()
