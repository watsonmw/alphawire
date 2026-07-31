#
# Alphawire Python API
#
# Supported Python versions: 3.7+ (CPython)
#
import enum
import time
import typing

from ._binding import ffi, lib

# Here are some practical ways to extend and improve so it exposes more of the C API in cleanly and safely. `main.py``aw-control.h`
# ## 1) Add thin wrappers for the missing C functions
# Right now only exposes a subset of `AwControl_*`. The header suggests several useful areas to add next: `main.py`
# - **Support queries**
#     - `supports_event(eventCode)`
#     - `supports_control(controlCode)`
#     - `supports_property(propCode)`
#     - `property_enabled(property)` / `property_enabled_by_code(propCode)`
#
# - **Property helpers**
#     - `get_property_value_as_bool(property)`
#     - `is_property_writable(property)`
#     - `is_property_notch(property)`
#     - `get_property_id(property)`
#
# - **Control helpers**
#     - `set_control_value(controlCode, value)`
#     - `get_enums_for_control(controlCode)`
#     - `get_enums_for_property(property)`
#
# - **Media / device features**
#     - `get_osd_image()`
#     - `get_camera_settings_file()`
#     - `put_camera_settings_file()`
#     - `read_events(timeout_ms)`
#     - `get_magnifier()`
#     - `set_magnifier(...)`
#     - `remote_button_enable()`
#     - `remote_button_press(button, pressed)`
#
# This makes the Python layer much more complete and avoids forcing users to call raw `lib.*` functions directly.
# ## 2) Normalize return-value handling
# Some methods currently return raw C results, while others convert to . For a nicer Python API: `bool`
# - Return for “did it work?” `bool`
# - Return when a value is unavailable `None`
# - Return Python objects instead of C structs where possible
# - Raise exceptions only for truly exceptional failures, if you want a more Pythonic style
#
# For example, functions like `connect()`, , and `set_control_toggle()` could consistently return or an `AwResult` wrapper. `cleanup()``bool`
# ## 3) Wrap C structs in Python-friendly classes
# You already have , , and . Consider adding small wrapper classes for: `AwDeviceInfo``AwDevice``AwControl`
# - `AwPtpProperty`
# - `AwPtpControl`
# - `AwPtpEvent`
# - `AwMagnifier`
# - enum/value lists returned by and `AwControl_GetEnumsForProperty``AwControl_GetEnumsForControl`
#
# That gives you a place to add:
# - readable `__repr__`
# - Python properties
# - conversions from raw C fields into strings / ints / enums
#
# ## 4) Add resource-safety improvements
# A few patterns will make the binding more robust:
# - Implement context managers:
#     - as `with AwDeviceList() as dl:` `AwDeviceList`
#     - as `with device.open_control() as ctrl:` `AwControl`
#
# - Make idempotent and safe after partial initialization `cleanup()`
# - Avoid relying only on , since Python finalization is not deterministic `__del__`
#
# This is especially useful for camera sessions and allocated buffers.
# ## 5) Improve memory ownership clarity
# Functions like live view image retrieval and captured image download manage native buffers. It would help to standardize ownership rules in Python:
# - Document whether the returned remains valid until the next call `memoryview`
# - Convert to when a stable copy is safer `bytes`
# - Centralize freeing of , `MStr`, and enum buffers into helper methods `MMemIO`
#
# For example, the Python wrapper can hide all / details. `Aw_MemIOFree``AwControl_FreeLiveViewFrames`
# ## 6) Add explicit type annotations
# would benefit from richer typing, especially for public methods: `main.py`
# - `-> bool`
# - `-> typing.Optional[str]`
# - `-> typing.Optional[bytes]`
# - `-> list[...]`
# - `-> AwResult` if you expose the raw result type
#
# This improves editor completion and makes the API easier to use.
# ## 7) Add higher-level convenience methods
# Instead of only exposing low-level getters/setters, add user-friendly helpers such as:
# - `list_properties()`
# - `list_controls()`
# - `find_property_by_id(...)`
# - `find_property_by_code(...)`
# - `read_all_events(timeout_ms=0)`
# - `capture_image_bytes()`
# - `live_view_frame()`
#
# These can be built on top of the raw `AwControl_*` functions and make the Python API feel much more complete.
# ## 8) Use Python enums for codes
# The C API uses numeric codes for:
# - properties
# - controls
# - events
# - protocols
# - result codes
#
# Wrapping these in `Enum` or `IntEnum` classes will make much easier to use and harder to misuse. `main.py`
# For example:
# - `AwSonyProtocolVersion`
# - `AwControlCode`
# - `AwPropertyCode`
# - `AwEventCode`
#
# ## 9) Add error conversion helpers
# If `AwResult` is common, create a helper that:
# - extracts the result code
# - converts it to a readable message
# - optionally raises a Python exception
#
# That way users get meaningful feedback instead of only numeric codes.
# ## 10) Make the API more discoverable
# A good Python wrapper should be self-explanatory. Consider adding:
# - docstrings for every public method
# - short usage examples in the module
# - `__repr__` / `__str__` for key objects
# - maybe `dir()`-friendly names for properties and controls
#
# ## Suggested next additions from `aw-control.h`
# If you want the biggest value with the least effort, I’d prioritize these first:
# 1. `AwControl_SupportsEvent`
# 2. `AwControl_SupportsControl`
# 3. `AwControl_SupportsProperty`
# 4. `AwControl_GetPropertyValueAsBool`
# 5. `AwControl_IsPropertyWritable`
# 6. `AwControl_GetEnumsForProperty`
# 7. `AwControl_GetEnumsForControl`
# 8. `AwControl_ReadEvents`
# 9. / `AwControl_GetMagnifier``AwControl_SetMagnifier`
# 10. `AwControl_RemoteButtonPress`
#
# ## If you want, I can also help with:
# - a **clean Python API design** for `main.py`
# - a **minimal patch plan** for exposing the next C functions
# - or a **concrete wrapper implementation** for one section, like properties, controls, or live view

def _convert_c_str(c_str) -> typing.Optional[str]:
    if c_str == ffi.NULL:
        return None
    return ffi.string(c_str).decode("utf-8")


def _convert_m_str(m_str) -> str:
    if m_str.str == ffi.NULL or m_str.size == 0:
        return ""
    return ffi.string(m_str.str, m_str.size).decode("utf-8")


def _usb_bcd_version_as_string(usb_version) -> str:
    return f"{(usb_version >> 8) & 0xFF}.{usb_version & 0xFF:02d}"


class PtpDataType(enum.IntEnum):
    UNDEF = 0x0000
    INT8 = 0x0001
    UINT8 = 0x0002
    INT16 = 0x0003
    UINT16 = 0x0004
    INT32 = 0x0005
    UINT32 = 0x0006
    INT64 = 0x0007
    UINT64 = 0x0008
    INT128 = 0x0009
    UINT128 = 0x000A
    AINT8 = 0x4001
    AUINT8 = 0x4002
    AINT16 = 0x4003
    AUINT16 = 0x4004
    AINT32 = 0x4005
    AUINT32 = 0x4006
    AINT64 = 0x4007
    AUINT64 = 0x4008
    AINT128 = 0x4009
    AUINT128 = 0x400A
    STR = 0xFFFF


class AwDeviceInfo:
    def __init__(self, ffi_device):
        self.manufacturer = _convert_m_str(ffi_device.manufacturer)
        self.product = _convert_m_str(ffi_device.product)
        self.serial = _convert_m_str(ffi_device.serial)
        self.ip_address = _convert_m_str(ffi_device.ipAddress)
        self.usb_vid = ffi_device.usbVID
        self.usb_pid = ffi_device.usbPID
        self.usb_version = _usb_bcd_version_as_string(ffi_device.usbVersion)
        self._ffi_device = ffi_device

class AwPtpProperty:
    def __init__(self, control, ffi_control, allocator, ffi_property):
        self._control = control
        self._allocator = allocator
        self._ffi_control = ffi_control
        self._ffi_property = ffi_property
        self.code = ffi_property.propCode

    def get_value_as_str(self) -> typing.Optional[str]:
        out_str = ffi.new("MStr[1]")
        ok = lib.AwControl_GetPropertyValueAsStr(self._ffi_control,  self._allocator, self._ffi_property, out_str)
        if not ok:
            return None
        result = _convert_m_str(out_str[0])
        lib.Aw_StrFree(self._allocator, out_str)
        return result

    def get_value(self):
        out_str = ffi.new("MStr[1]")
        ok = lib.AwControl_GetPropertyValueAsKnownStr(self._ffi_control,  self._allocator, self._ffi_property, out_str)
        if ok:
            result = _convert_m_str(out_str[0])
            lib.Aw_StrFree(self._allocator, out_str)
            return result

        # Return integer if string conversion not available
        dt = self._ffi_property.dataType
        v = self._ffi_property.value
        if dt == PtpDataType.INT8: return v.i8
        if dt == PtpDataType.UINT8: return v.u8
        if dt == PtpDataType.INT16: return v.i16
        if dt == PtpDataType.UINT16: return v.u16
        if dt == PtpDataType.INT32: return v.i32
        if dt == PtpDataType.UINT32: return v.u32
        if dt == PtpDataType.INT64: return v.i64
        if dt == PtpDataType.UINT64: return v.u64
        return None

    def get_label(self) -> str:
        return _convert_c_str(lib.AwGetPropertyLabel(self.code))


class AwPtpControl:
    def __init__(self, control, ffi_control, allocator, ffi_ptp_control):
        self._control = control
        self._allocator = allocator
        self._ffi_control = ffi_control
        self._ffi_property = ffi_ptp_control
        self.code = ffi_ptp_control.controlCode
        self.label = _convert_c_str(ffi_ptp_control.label)


class AwSonyProtocolVersion(enum.Enum):
    V2 = 200
    V3 = 300


class AwControl:
    def __init__(self, ffi_device, allocator):
        self._ffi = ffi.new("AwControl[1]")
        self._allocator = allocator
        self._live_view_mem = None
        lib.AwControl_Init(self._ffi, ffi_device, allocator)

    def connect(self, sony_protocol_version: AwSonyProtocolVersion=AwSonyProtocolVersion.V3):
        return lib.AwControl_Connect(self._ffi, sony_protocol_version.value)

    def cleanup(self):
        if self._ffi is not None:
            lib.AwControl_Cleanup(self._ffi)
            self._ffi = None
        if self._live_view_mem is not None:
            lib.Aw_MemIOFree(self._live_view_mem)
            self._live_view_mem = None

    def update_properties(self, full_refresh: bool = True) -> bool:
        result = lib.AwControl_UpdateProperties(self._ffi, full_refresh)
        return result.code == lib.AW_RESULT_OK

    def get_num_properties(self) -> int:
        return lib.AwControl_NumProperties(self._ffi)

    def get_property_at_index(self, index: int) -> typing.Optional[AwPtpProperty]:
        prop = lib.AwControl_GetPropertyByIndex(self._ffi, index)
        if prop is None:
            return None
        return AwPtpProperty(self, self._ffi, self._allocator, prop)

    def get_property_by_code(self, code: int) -> typing.Optional[AwPtpProperty]:
        prop = lib.AwControl_GetPropertyByCode(self._ffi, code)
        if prop is None:
            return None
        return AwPtpProperty(self, self._ffi, self._allocator, prop)

    def get_property_by_id(self, prop_id: str) -> typing.Optional[AwPtpProperty]:
        c_id = ffi.new("char[]", prop_id.encode("utf-8"))
        prop = lib.AwControl_GetPropertyById(self._ffi, c_id)
        if prop is None:
            return None
        return AwPtpProperty(self, self._ffi, self._allocator, prop)

    def get_num_controls(self) -> int:
        return lib.AwControl_NumControls(self._ffi)

    def get_control_at_index(self, index: int) -> typing.Optional[AwPtpControl]:
        control = lib.AwControl_GetControlByIndex(self._ffi, index)
        if control is None:
            return None
        return AwPtpControl(self, self._ffi, self._allocator, control)

    def get_control_by_code(self, control_code: int) -> typing.Optional[AwPtpControl]:
        control = lib.AwControl_GetControlByCode(self._ffi, control_code)
        if control is None:
            return None
        return AwPtpControl(self, self._ffi, self._allocator, control)

    def set_control_toggle(self, control_code: int, pressed: bool) -> typing.Optional[AwPtpControl]:
        return lib.AwControl_SetControlToggle(self._ffi, control_code, pressed)

    def set_property_str(self, ffi_property, value: str):
        c_val = ffi.new("char[]", value.encode("utf-8"))
        m_str = ffi.new("MStr[1]")
        m_str[0].str = c_val
        m_str[0].size = len(value)
        m_str[0].capacity = 0
        return lib.AwControl_SetPropertyStr(self._ffi, ffi_property, m_str[0])

    def get_live_view_image(self) -> typing.Optional[memoryview]:
        if self._live_view_mem is None:
            self._live_view_mem = ffi.new("MMemIO[1]")
            self._live_view_mem[0].allocator = self._allocator
        live_view_frames = ffi.new("AwLiveViewFrames[1]")
        result = lib.AwControl_GetLiveViewImage(self._ffi, self._live_view_mem, live_view_frames)
        if self._live_view_mem[0].size:
            lib.AwControl_FreeLiveViewFrames(self._ffi, live_view_frames)
            buf = ffi.buffer(self._live_view_mem[0].mem, self._live_view_mem[0].size)
            return memoryview(buf)
        return None

    def get_pending_files(self) -> int:
        return lib.AwControl_GetPendingFiles(self._ffi)

    def get_captured_image(self) -> typing.Optional[bytes]:
        mem_io = ffi.new("MMemIO[1]")
        mem_io[0].allocator = self._allocator
        cii = ffi.new("AwPtpCapturedImageInfo[1]")
        result = lib.AwControl_GetCapturedImage(self._ffi, mem_io, cii)
        if result.code == lib.AW_RESULT_OK and mem_io[0].size:
            data = bytes(ffi.buffer(mem_io[0].mem, mem_io[0].size))
            lib.Aw_MemIOFree(mem_io)
            return data
        lib.Aw_MemIOFree(mem_io)
        return None

    def __del__(self):
        self.cleanup()


class AwDevice:
    def __init__(self, ffi_device):
        self._ffi_device = ffi_device
        self._allocator = ffi.new("MAllocator[1]")
        lib.Aw_InitDefaultAllocator(self._allocator)
        self._control = None

    def open_control(self) -> AwControl:
        if self._control is None:
            self._control = AwControl(self._ffi_device, self._allocator)
        return self._control


class AwDeviceList:
    def __init__(self):
        self._devlist = ffi.new("AwDeviceList[1]")
        self._allocator = ffi.new("MAllocator[1]")
        lib.Aw_InitDefaultAllocator(self._allocator)
        self._is_open = False
        self._iter_index = 0

    def open(self) -> bool:
        if self._is_open:
            self.close()
        ok = lib.AwDeviceList_Open(self._devlist, self._allocator)
        if ok:
            self._is_open = True
        return bool(ok)

    def close(self) -> bool:
        if not self._is_open:
            return False
        ok = lib.AwDeviceList_Close(self._devlist)
        if ok:
            self._is_open = False
        return bool(ok)

    def refresh(self) -> bool:
        if not self._is_open:
            return False
        return bool(lib.AwDeviceList_RefreshList(self._devlist))

    def needs_refresh(self) -> bool:
        if not self._is_open:
            return False
        return bool(lib.AwDeviceList_NeedsRefresh(self._devlist))

    def is_refreshing(self) -> bool:
        if not self._is_open:
            return False
        return bool(lib.AwDeviceList_IsRefreshingList(self._devlist))

    def poll_updates(self) -> bool:
        if not self._is_open:
            return False
        return bool(lib.AwDeviceList_PollUpdates(self._devlist))

    def open_device(self, device_info: AwDeviceInfo) -> typing.Optional[AwDevice]:
        if not self._is_open:
            return None
        device_out = ffi.new("AwDevice**")
        ffi_device_info = ffi.addressof(device_info._ffi_device)
        result = lib.AwDeviceList_OpenDevice(self._devlist, ffi_device_info, device_out)
        if result.code == lib.AW_RESULT_OK:
            return AwDevice(device_out[0])
        return None

    def close_device(self, device: AwDevice):
        lib.AwDeviceList_CloseDevice(self._devlist, device._ffi_device)

    def __len__(self) -> int:
        if not self._is_open:
            return 0
        return lib.AwDeviceList_NumDevices(self._devlist)

    def __getitem__(self, index: int) -> AwDeviceInfo:
        if not self._is_open:
            raise IndexError("Device list is not open")
        if index < 0 or index >= len(self):
            raise IndexError("Device index out of range")
        return AwDeviceInfo(self._devlist[0].devices.data[index])

    def __iter__(self):
        self._iter_index = 0
        return self

    def __next__(self) -> AwDeviceInfo:
        if self._iter_index >= len(self):
            raise StopIteration
        device = self[self._iter_index]
        self._iter_index += 1
        return device

    def __del__(self):
        if self._is_open:
            self.close()
