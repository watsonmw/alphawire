import os
import sys
import typing

sys.path.append(os.path.dirname(os.getcwd()) + "/build")

from _alphawire_cffi import ffi, lib as awire_lib


def _convert_c_str(c_str) -> str:
    return ffi.string(c_str).decode('utf-8')


def _USB_BcdVersionAsString(usbVersion) -> str:
    return f"{(usbVersion >> 8) & 0xFF}.{usbVersion & 0xFF:02d}"


class PTPDevice:
    def __init__(self, awire_lib, device):
        self.awire_lib = awire_lib
        self.device = device
        self.allocator = ffi.new("MAllocator[1]")
        self.awire_lib.PTP_InitDefaultAllocator(self.allocator)
        self.control = None

    def open_control(self):
        if self.control is None:
            self.control = PTPControl(self.awire_lib, self.device, self.allocator)
        return self.control


class PTPControl:
    def __init__(self, awire_lib, device, allocator):
        self.awire_lib = awire_lib
        self.control = ffi.new("PTPControl[1]")
        self.device = device
        self.awire_lib.PTPControl_Init(self.control, device, allocator)
        self.allocator = allocator
        self.live_view_image = None  # Need way to free this

    def connect(self, sonyProtocolVersion=300):
        self.awire_lib.PTPControl_Connect(self.control, sonyProtocolVersion)

    def cleanup(self):
        self.awire_lib.PTPControl_Cleanup(self.control)

    def get_num_properties(self) -> int:
        return self.awire_lib.PTPControl_NumProperties(self.control)

    def get_property_at_index(self, index: int):
        return self.awire_lib.PTPControl_GetPropertyAtIndex(self.control, index)

    def get_property_by_code(self, code: int):
        return self.awire_lib.PTPControl_GetPropertyByCode(self.control, code)

    def get_property_value_as_str(self, code: int) -> typing.Optional[str]:
        mstr = ffi.new("MStr[1]")
        r = self.awire_lib.PTPControl_GetPropertyAsStr(self.control, code, mstr)
        if r:
            return _convert_c_str(mstr[0].str)
        else:
            return None

    def get_property_name(self, code: int) -> typing.Optional[str]:
        cstr = self.awire_lib.PTP_GetPropertyStr(code)
        if cstr == ffi.NULL:
            return None
        else:
            return _convert_c_str(cstr)

    def get_num_controls(self) -> int:
        return self.awire_lib.PTPControl_NumControls(self.control)

    def get_control_at_index(self, index: int):
        return self.awire_lib.PTPControl_GetControlAtIndex(self.control, index)

    def get_live_view_image(self) -> typing.Optional[memoryview]:
        if self.live_view_image is None:
            mem_io = ffi.new("MMemIO[1]")
            self.live_view_image = mem_io[0]
            self.live_view_image.allocator = self.allocator
        p_mem_io = ffi.addressof(self.live_view_image)
        live_view_frames = ffi.new("LiveViewFrames[1]")
        p_live_view_frames = ffi.addressof(live_view_frames[0])

        if self.awire_lib.PTPControl_GetLiveViewImage(self.control, p_mem_io, p_live_view_frames):
            self.awire_lib.PTP_FreeLiveViewFrames(self.allocator, live_view_frames)
            buffer_obj = ffi.buffer(self.live_view_image.mem, self.live_view_image.size)
            return memoryview(buffer_obj)

        return None

    def __del__(self):
        self.cleanup()
        if self.live_view_image is not None:
            self.awire_lib.PTP_MemIOFree(ffi.addressof(self.live_view_image))
            self.live_view_image = None


class PTPDeviceInfo:
    def __init__(self, dev: ffi.CData):
        self.manufacturer = _convert_c_str(dev.manufacturer.str)
        self.product = _convert_c_str(dev.product.str)
        self.serial = _convert_c_str(dev.serial.str)
        self.usb_vid = dev.usbVID
        self.usb_pid =  dev.usbPID
        self.usb_version = _USB_BcdVersionAsString(dev.usbVersion)
        self._ffi_device = dev


class PTPDeviceList:
    def __init__(self, awire_lib):
        self.awire_lib = awire_lib
        self.devlist = ffi.new("PTPDeviceList[1]")
        self.allocator = ffi.new("MAllocator[1]")
        self.awire_lib.PTP_InitDefaultAllocator(self.allocator)
        self.isOpen = False
        self.current_index = 0

    def open(self) -> bool:
        if self.isOpen:
            self.close()
        ok = self.awire_lib.PTPDeviceList_Open(self.devlist, self.allocator)
        if ok:
            self.isOpen = True
        return ok

    def close(self) -> bool:
        if not self.isOpen:
            return False
        ok = self.awire_lib.PTPDeviceList_Close(self.devlist)
        if ok:
            self.isOpen = False
        return ok

    def refresh(self) -> bool:
        if not self.isOpen:
            return False
        ok = self.awire_lib.PTPDeviceList_RefreshList(self.devlist)
        if ok:
            self.current_index = 0
        return ok

    def open_device(self, device_info: PTPDeviceInfo) -> typing.Optional[PTPDevice]:
        if not self.isOpen:
            return None
        device_out = ffi.new("PTPDevice**")
        di = ffi.addressof(device_info._ffi_device)
        ok = self.awire_lib.PTPDeviceList_OpenDevice(self.devlist, di, device_out)
        if ok:
            return PTPDevice(self.awire_lib, device_out[0])
        else:
            return None

    def __len__(self):
        if not self.isOpen:
            return 0
        return self.awire_lib.PTPDeviceList_NumDevices(self.devlist)

    def __getitem__(self, index: int) -> typing.Optional[PTPDeviceInfo]:
        if not self.isOpen:
            return None
        if index >= len(self):
            raise IndexError("Device index out of range")
        dev = self.devlist[0].devices[index]
        return PTPDeviceInfo(dev)

    def __iter__(self):
        self.current_index = 0
        return self

    def __next__(self):
        if self.current_index >= len(self):
            raise StopIteration
        device = self[self.current_index]
        self.current_index += 1
        return device


def _run_tests(awire_lib):
    deviceList = PTPDeviceList(awire_lib)
    ok = deviceList.open()
    if ok:
        deviceList.refresh()
        for device_info in deviceList:
            print(f"    {device_info.manufacturer} - {device_info.product} (S/N: {device_info.serial})")
            device = deviceList.open_device(device_info)
            control = device.open_control()
            control.connect()

            # Print Properties
            num_properties = control.get_num_properties()
            print(f"        Properties: {num_properties}")
            for i in range(num_properties):
                property = control.get_property_at_index(i)
                property_name = control.get_property_name(property.propCode)
                if property_name is not None:
                    property_value = control.get_property_value_as_str(property.propCode)
                    print(f"            '{property_name}': {property_value}")

            # Print Controls
            num_controls = control.get_num_controls()
            print(f"        Controls: {num_controls}")
            for i in range(num_controls):
                ctrl = control.get_control_at_index(i)
                control_name = _convert_c_str(ctrl.name)
                if control_name is not None:
                    print(f"            '{control_name}' [0x{ctrl.controlCode:04x}]")

            # Save Live View image
            live_view_image = control.get_live_view_image()
            if live_view_image is not None:
                filename = "live_view.jpg"
                print(f"Saving live view image to {filename}...")
                with open(filename, "wb") as f:
                    f.write(live_view_image)

            # Capture Image & Download

            control.cleanup()
        deviceList.close()


if __name__ == "__main__":
    _run_tests(awire_lib)
