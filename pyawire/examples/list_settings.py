import awire


def list_settings():
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
            num_properties = control.get_num_properties()
            awire.log_info(f"        Properties: {num_properties}")
            for i in range(num_properties):
                property = control.get_property_at_index(i)
                prop_label = property.get_label()
                if prop_label is not None:
                    property_value = property.get_value()
                    awire.log_info(f"            '{prop_label}': {property_value}")

                    enums = property.get_enums()
                    if enums:
                        for val, label in enums:
                            awire.log_info(f"                {label}")

            # Print Controls
            num_controls = control.get_num_controls()
            awire.log_info(f"        Controls: {num_controls}")
            for i in range(num_controls):
                ctrl = control.get_control_at_index(i)
                if ctrl.label is not None:
                    awire.log_info(f"            '{ctrl.label}' [0x{ctrl.code:04x}]")

            device.close()

    print("device_list.close")
    device_list.close()


if __name__ == '__main__':
    awire.log_set_level(awire.AwLogLevel.TRACE)
    list_settings()
