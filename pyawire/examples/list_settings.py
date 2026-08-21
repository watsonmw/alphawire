import awire
import argparse


# List all exposed settings for all connected cameras
# Lists current value for each setting, and list any potential values

def list_settings(show_all=False):
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
        awire.log_info(f"    {device_info.manufacturer} - {device_info.product} (S/N: {device_info.serial})")

        device = device_list.open_device(device_info)
        if device is not None:
            control = device.open_control()
            control.connect()

            # Print all properties
            num_props = control.get_num_properties()
            awire.log_info(f"        Properties: {num_props}")
            for i in range(num_props):
                property = control.get_property_at_index(i)
                prop_label = property.get_label()

                if prop_label is None:
                    if not show_all:
                        continue
                    prop_label = f'0x{property.code:04x}'
                else:
                    prop_label = f'\'{prop_label}\''
                prop_val = property.get_value()
                prop_val_as_str = property.get_value_as_str()
                if prop_val_as_str is None:
                    awire.log_info(f"            {prop_label}: {prop_val}")
                else:
                    awire.log_info(f"            {prop_label}: {prop_val} \"{prop_val_as_str}\"")

                enums = property.get_enums()
                if enums:
                    for enum_val, enum_val_as_str in enums:
                        if enum_val_as_str is None:
                            awire.log_info(f"                {enum_val}")
                        else:
                            awire.log_info(f"                {enum_val} \"{enum_val_as_str}\"")

            # Print Controls
            num_controls = control.get_num_controls()
            awire.log_info(f"        Controls: {num_controls}")
            for i in range(num_controls):
                ctrl = control.get_control_at_index(i)
                if ctrl.label is not None:
                    awire.log_info(f"            '{ctrl.label}' [0x{ctrl.code:04x}]")
                else:
                    awire.log_info(f"            0x{ctrl.code:04x}")

            device.close()

    device_list.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='List Alpha camera settings')
    parser.add_argument('--all', action='store_true', help='Show all properties, including those without labels')
    args = parser.parse_args()

    awire.log_set_level(awire.AwLogLevel.TRACE)
    list_settings(show_all=args.all)
