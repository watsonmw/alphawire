import time

import awire


# - Find a device
# - List its settings
# - Adjust some settings
# - Get Live View Image
# - Shoot Image and download
def basic_example():
    print(f"Searching for Alpha cameras...")

    device_list = awire.AwDeviceList()
    ok = device_list.open()
    if not ok:
        print("Failed to open device list")
        return

    device_list.refresh()

    while device_list.is_refreshing():
        device_list.poll_updates()
        if len(device_list) > 0:
            print(f"Found {len(device_list)} devices")
            break

    for device_info in device_list:
        print(f"    {device_info.manufacturer} - {device_info.product} (S/N: {device_info.serial})")

        device = device_list.open_device(device_info)
        control = device.open_control()
        control.connect()

        # Print Properties
        num_properties = control.get_num_properties()
        print(f"        Properties: {num_properties}")
        for i in range(num_properties):
            property = control.get_property_at_index(i)
            prop_label = property.get_label()
            if prop_label is not None:
                #property_value = property.get_value_as_str()
                property_value = property.get_value()
                print(f"            '{prop_label}': {property_value}")

        # Print Controls
        num_controls = control.get_num_controls()
        print(f"        Controls: {num_controls}")
        for i in range(num_controls):
            ctrl = control.get_control_at_index(i)
            if ctrl.label is not None:
                print(f"            '{ctrl.label}' [0x{ctrl.code:04x}]")

        # Save Live View image
        live_view_image = None
        tries = 0
        while live_view_image is None:
            live_view_image = control.get_live_view_image()
            if live_view_image is not None:
                filename = "live_view.jpg"
                print(f"Saving live view image to {filename}...")
                with open(filename, "wb") as f:
                    f.write(live_view_image)
                break
            time.sleep(.5)
            tries += 1
            if tries >= 10:
                break

        # TODO: Set some settings

        # TODO: Capture Image & Download

        # Clean up
        control.cleanup()



if __name__ == '__main__':
    basic_example()
    print("Done.")



