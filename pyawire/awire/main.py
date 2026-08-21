#
# Alphawire Python API
#
# Supported Python versions: 3.7+ (CPython)
#
"""
Alphawire Python API

This module provides a Python wrapper for the Alphawire C library, allowing control of
Sony cameras via PTP over USB.
"""
import enum
import sys
import time
import typing
import weakref

from ._binding import ffi, lib


def _convert_c_str(c_str: typing.Any) -> typing.Optional[str]:
    if c_str == ffi.NULL:
        return None
    return ffi.string(c_str).decode("utf-8", errors="surrogateescape")


def _convert_m_str(m_str: typing.Any) -> str:
    if m_str.str == ffi.NULL or m_str.size == 0:
        return ""
    return ffi.string(m_str.str, m_str.size).decode("utf-8", errors="surrogateescape")


def _usb_bcd_version_as_string(usb_version: int) -> str:
    return f"{(usb_version >> 8) & 0xFF}.{usb_version & 0xFF:02d}"


class AwIntEnum(enum.IntEnum):
    def __str__(self) -> str:
        return self.name
        

class PtpDataType(AwIntEnum):
    """PTP Data Types."""
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

class AwSonyProtocolVersion(AwIntEnum):
    """Sony PTP Protocol Versions."""
    V2 = 200
    V3 = 300


class AwDialMode(AwIntEnum):
    """Camera Dial Mode."""
    CAMERA = 0x00
    REMOTE = 0x01


class AwExposureProgramModeU16(AwIntEnum):
    """Exposure Program Modes."""
    MANUAL = 0x0001
    AUTOMATIC = 0x0002
    APERTURE_PRIORITY = 0x0003
    SHUTTER_PRIORITY = 0x0004
    PROGRAM_CREATIVE = 0x0005
    PROGRAM_ACTION = 0x0006
    PORTRAIT = 0x0007
    AUTO = 0x8000
    AUTO_PLUS = 0x8001
    P_A = 0x8008
    P_S = 0x8009
    SPORTS_ACTION = 0x8011
    SUNSET = 0x8012
    NIGHT_SCENE = 0x8013
    LANDSCAPE = 0x8014
    MACRO = 0x8015
    HAND_HELD_TWILIGHT = 0x8016
    NIGHT_PORTRAIT = 0x8017
    ANTI_MOTION_BLUR = 0x8018
    PET = 0x8019
    GOURMET = 0x801A
    FIREWORKS = 0x801B
    HIGH_SENSITIVITY = 0x801C
    MEMORY_RECALL = 0x8020
    MOVIE_P = 0x8050
    MOVIE_A = 0x8051
    MOVIE_S = 0x8052
    MOVIE_M = 0x8053
    MOVIE_AUTO = 0x8054


class AwExposureProgramMode(AwIntEnum):
    """Exposure Program Modes."""
    MANUAL = 0x00000001
    AUTOMATIC = 0x00010002
    APERTURE_PRIORITY = 0x00020003
    SHUTTER_PRIORITY = 0x00030004
    PROGRAM_CREATIVE = 0x00000005
    PROGRAM_ACTION = 0x00000006
    PORTRAIT = 0x00000007
    AUTO = 0x00048000
    AUTO_PLUS = 0x00048001
    P_A = 0x00008008
    P_S = 0x00008009
    SPORTS_ACTION = 0x00058011
    SUNSET = 0x00058012
    NIGHT_SCENE = 0x00058013
    LANDSCAPE = 0x00058014
    MACRO = 0x00058015
    HAND_HELD_TWILIGHT = 0x00058016
    NIGHT_PORTRAIT = 0x00058017
    ANTI_MOTION_BLUR = 0x00058018
    PET = 0x00058019
    GOURMET = 0x0005801A
    FIREWORKS = 0x0005801B
    HIGH_SENSITIVITY = 0x0005801C
    MEMORY_RECALL = 0x00008020
    CONTINUOUS_PRIORITY_AE = 0x00008030
    TELE_ZOOM_CONTINUOUS_PRIORITY_AE_8PICS = 0x00008031
    TELE_ZOOM_CONTINUOUS_PRIORITY_AE_10PICS = 0x00008032
    CONTINUOUS_PRIORITY_AE_12PICS = 0x00008033
    PANORAMA_3D = 0x00068040
    PANORAMA = 0x00068041
    MOVIE_P = 0x00078050
    MOVIE_A = 0x00078051
    MOVIE_S = 0x00078052
    MOVIE_M = 0x00078053
    MOVIE_AUTO = 0x00078054
    MOVIE_SQ_P = 0x00098059
    MOVIE_SQ_A = 0x0009805A
    MOVIE_SQ_S = 0x0009805B
    MOVIE_SQ_M = 0x0009805C
    MOVIE_SQ_AUTO = 0x0009805D
    FLASH_OFF = 0x00008060
    PICTURE_EFFECT = 0x00008070
    HFR_P = 0x00088080
    HFR_A = 0x00088081
    HFR_S = 0x00088082
    HFR_M = 0x00088083
    SQ_P = 0x00008084
    SQ_A = 0x00008085
    SQ_S = 0x00008086
    SQ_M = 0x00008087
    MOVIE = 0x000A8088
    STILL = 0x000A8089
    F_MOVIE_OR_SQ = 0x000B808A
    MOVIE_F_MODE = 0x00078090
    SQ_F_MODE = 0x00098091
    INTERVAL_REC_MOVIE_F_MODE = 0x000C8092
    INTERVAL_REC_MOVIE_P = 0x000C8093
    INTERVAL_REC_MOVIE_A = 0x000C8094
    INTERVAL_REC_MOVIE_S = 0x000C8095
    INTERVAL_REC_MOVIE_M = 0x000C8096
    INTERVAL_REC_MOVIE_AUTO = 0x000C8097


class AwCaptureMode(AwIntEnum):
    """Capture (Drive) Modes."""
    NORMAL = 0x00000001
    CONTINUOUS_HI = 0x00010002
    TIMELAPSE = 0x00020003
    SELF_TIMER_5S = 0x00038003
    SELF_TIMER_10S = 0x00038004
    SELF_TIMER_2S = 0x00038005
    SELF_PORTRAIT_1_PERSON = 0x00078006
    SELF_PORTRAIT_2_PEOPLE = 0x00078007
    CONTINUOUS_SELF_TIMER_3_IMG = 0x00088008
    CONTINUOUS_SELF_TIMER_5_IMG = 0x00088009
    REMOTE_COMMANDER = 0x0007800A
    MIRROR_UP = 0x0007800B
    CONTINUOUS_SELF_TIMER_3_IMG_5S = 0x0008800C
    CONTINUOUS_SELF_TIMER_5_IMG_5S = 0x0008800D
    CONTINUOUS_SELF_TIMER_3_IMG_2S = 0x0008800E
    CONTINUOUS_SELF_TIMER_5_IMG_2S = 0x0008800F
    CONTINUOUS_HI_PLUS = 0x00018010
    CONTINUOUS_HI_LIVE = 0x00018011
    CONTINUOUS_LO = 0x00018012
    CONTINUOUS_SHOOTING = 0x00018013
    CONTINUOUS_SHOOTING_SPEED_PRIORITY = 0x00018014
    CONTINUOUS_MID = 0x00018015
    CONTINUOUS_MID_LIVE = 0x00018016
    CONTINUOUS_LO_LIVE = 0x00018017
    WHITE_BALANCE_BRACKET_LO = 0x00068018
    DRO_BRACKET_LO = 0x00078019
    LPF_BRACKET = 0x0007801A
    WHITE_BALANCE_BRACKET_HI = 0x00068028
    DRO_BRACKET_HI = 0x00078029
    SPOT_BURST_SHOOTING_LO = 0x00098030
    SPOT_BURST_SHOOTING_MID = 0x00098031
    SPOT_BURST_SHOOTING_HI = 0x00098032
    FOCUS_BRACKET = 0x000A8040
    CONTINUOUS_BRACKET_0_3_EV_3_IMG = 0x00048337
    CONTINUOUS_BRACKET_0_5_EV_3_IMG = 0x00048357
    CONTINUOUS_BRACKET_0_7_EV_3_IMG = 0x00048377
    CONTINUOUS_BRACKET_1_0_EV_3_IMG = 0x00048311
    CONTINUOUS_BRACKET_1_3_EV_3_IMG = 0x00048341
    CONTINUOUS_BRACKET_1_5_EV_3_IMG = 0x00048361
    CONTINUOUS_BRACKET_1_7_EV_3_IMG = 0x00048381
    CONTINUOUS_BRACKET_2_0_EV_3_IMG = 0x00048321
    CONTINUOUS_BRACKET_2_3_EV_3_IMG = 0x00048351
    CONTINUOUS_BRACKET_2_5_EV_3_IMG = 0x00048371
    CONTINUOUS_BRACKET_2_7_EV_3_IMG = 0x00048391
    CONTINUOUS_BRACKET_3_0_EV_3_IMG = 0x00048331
    CONTINUOUS_BRACKET_0_3_EV_5_IMG = 0x00048537
    CONTINUOUS_BRACKET_0_5_EV_5_IMG = 0x00048557
    CONTINUOUS_BRACKET_0_7_EV_5_IMG = 0x00048577
    CONTINUOUS_BRACKET_1_0_EV_5_IMG = 0x00048511
    CONTINUOUS_BRACKET_1_3_EV_5_IMG = 0x00048541
    CONTINUOUS_BRACKET_1_5_EV_5_IMG = 0x00048561
    CONTINUOUS_BRACKET_1_7_EV_5_IMG = 0x00048581
    CONTINUOUS_BRACKET_2_0_EV_5_IMG = 0x00048521
    CONTINUOUS_BRACKET_2_3_EV_5_IMG = 0x00048551
    CONTINUOUS_BRACKET_2_5_EV_5_IMG = 0x00048571
    CONTINUOUS_BRACKET_2_7_EV_5_IMG = 0x00048591
    CONTINUOUS_BRACKET_3_0_EV_5_IMG = 0x00048531
    CONTINUOUS_BRACKET_0_3_EV_7_IMG = 0x00048737
    CONTINUOUS_BRACKET_0_5_EV_7_IMG = 0x00048757
    CONTINUOUS_BRACKET_0_7_EV_7_IMG = 0x00048777
    CONTINUOUS_BRACKET_1_0_EV_7_IMG = 0x00048711
    CONTINUOUS_BRACKET_1_3_EV_7_IMG = 0x00048741
    CONTINUOUS_BRACKET_1_5_EV_7_IMG = 0x00048761
    CONTINUOUS_BRACKET_1_7_EV_7_IMG = 0x00048781
    CONTINUOUS_BRACKET_2_0_EV_7_IMG = 0x00048721
    CONTINUOUS_BRACKET_0_3_EV_9_IMG = 0x00048937
    CONTINUOUS_BRACKET_0_5_EV_9_IMG = 0x00048957
    CONTINUOUS_BRACKET_0_7_EV_9_IMG = 0x00048977
    CONTINUOUS_BRACKET_1_0_EV_9_IMG = 0x00048911
    CONTINUOUS_BRACKET_0_3_EV_2_IMG_PLUS = 0x0004C237
    CONTINUOUS_BRACKET_0_5_EV_2_IMG_PLUS = 0x0004C257
    CONTINUOUS_BRACKET_0_7_EV_2_IMG_PLUS = 0x0004C277
    CONTINUOUS_BRACKET_1_0_EV_2_IMG_PLUS = 0x0004C211
    CONTINUOUS_BRACKET_1_3_EV_2_IMG_PLUS = 0x0004C241
    CONTINUOUS_BRACKET_1_5_EV_2_IMG_PLUS = 0x0004C261
    CONTINUOUS_BRACKET_1_7_EV_2_IMG_PLUS = 0x0004C281
    CONTINUOUS_BRACKET_2_0_EV_2_IMG_PLUS = 0x0004C221
    CONTINUOUS_BRACKET_2_3_EV_2_IMG_PLUS = 0x0004C251
    CONTINUOUS_BRACKET_2_5_EV_2_IMG_PLUS = 0x0004C271
    CONTINUOUS_BRACKET_2_7_EV_2_IMG_PLUS = 0x0004C291
    CONTINUOUS_BRACKET_3_0_EV_2_IMG_PLUS = 0x0004C231
    CONTINUOUS_BRACKET_0_3_EV_2_IMG_MINUS = 0x0004C23F
    CONTINUOUS_BRACKET_0_5_EV_2_IMG_MINUS = 0x0004C25F
    CONTINUOUS_BRACKET_0_7_EV_2_IMG_MINUS = 0x0004C27F
    CONTINUOUS_BRACKET_1_0_EV_2_IMG_MINUS = 0x0004C219
    CONTINUOUS_BRACKET_1_3_EV_2_IMG_MINUS = 0x0004C249
    CONTINUOUS_BRACKET_1_5_EV_2_IMG_MINUS = 0x0004C269
    CONTINUOUS_BRACKET_1_7_EV_2_IMG_MINUS = 0x0004C289
    CONTINUOUS_BRACKET_2_0_EV_2_IMG_MINUS = 0x0004C229
    CONTINUOUS_BRACKET_2_3_EV_2_IMG_MINUS = 0x0004C259
    CONTINUOUS_BRACKET_2_5_EV_2_IMG_MINUS = 0x0004C279
    CONTINUOUS_BRACKET_2_7_EV_2_IMG_MINUS = 0x0004C299
    CONTINUOUS_BRACKET_3_0_EV_2_IMG_MINUS = 0x0004C239
    SINGLE_BRACKET_0_3_EV_3_IMG = 0x00058336
    SINGLE_BRACKET_0_5_EV_3_IMG = 0x00058356
    SINGLE_BRACKET_0_7_EV_3_IMG = 0x00058376
    SINGLE_BRACKET_1_0_EV_3_IMG = 0x00058310
    SINGLE_BRACKET_1_3_EV_3_IMG = 0x00058340
    SINGLE_BRACKET_1_5_EV_3_IMG = 0x00058360
    SINGLE_BRACKET_1_7_EV_3_IMG = 0x00058380
    SINGLE_BRACKET_2_0_EV_3_IMG = 0x00058320
    SINGLE_BRACKET_2_3_EV_3_IMG = 0x00058350
    SINGLE_BRACKET_2_5_EV_3_IMG = 0x00058370
    SINGLE_BRACKET_2_7_EV_3_IMG = 0x00058390
    SINGLE_BRACKET_3_0_EV_3_IMG = 0x00058330
    SINGLE_BRACKET_0_3_EV_5_IMG = 0x00058536
    SINGLE_BRACKET_0_5_EV_5_IMG = 0x00058556
    SINGLE_BRACKET_0_7_EV_5_IMG = 0x00058576
    SINGLE_BRACKET_1_0_EV_5_IMG = 0x00058510
    SINGLE_BRACKET_1_3_EV_5_IMG = 0x00058540
    SINGLE_BRACKET_1_5_EV_5_IMG = 0x00058560
    SINGLE_BRACKET_1_7_EV_5_IMG = 0x00058580
    SINGLE_BRACKET_2_0_EV_5_IMG = 0x00058520
    SINGLE_BRACKET_2_3_EV_5_IMG = 0x00058550
    SINGLE_BRACKET_2_5_EV_5_IMG = 0x00058570
    SINGLE_BRACKET_2_7_EV_5_IMG = 0x00058590
    SINGLE_BRACKET_3_0_EV_5_IMG = 0x00058530
    SINGLE_BRACKET_0_3_EV_7_IMG = 0x00058736
    SINGLE_BRACKET_0_5_EV_7_IMG = 0x00058756
    SINGLE_BRACKET_0_7_EV_7_IMG = 0x00058776
    SINGLE_BRACKET_1_0_EV_7_IMG = 0x00058710
    SINGLE_BRACKET_1_3_EV_7_IMG = 0x00058740
    SINGLE_BRACKET_1_5_EV_7_IMG = 0x00058760
    SINGLE_BRACKET_1_7_EV_7_IMG = 0x00058780
    SINGLE_BRACKET_2_0_EV_7_IMG = 0x00058720
    SINGLE_BRACKET_0_3_EV_9_IMG = 0x00058936
    SINGLE_BRACKET_0_5_EV_9_IMG = 0x00058956
    SINGLE_BRACKET_0_7_EV_9_IMG = 0x00058976
    SINGLE_BRACKET_1_0_EV_9_IMG = 0x00058910
    SINGLE_BRACKET_0_3_EV_2_IMG_PLUS = 0x0005C236
    SINGLE_BRACKET_0_5_EV_2_IMG_PLUS = 0x0005C256
    SINGLE_BRACKET_0_7_EV_2_IMG_PLUS = 0x0005C276
    SINGLE_BRACKET_1_0_EV_2_IMG_PLUS = 0x0005C210
    SINGLE_BRACKET_1_3_EV_2_IMG_PLUS = 0x0005C240
    SINGLE_BRACKET_1_5_EV_2_IMG_PLUS = 0x0005C260
    SINGLE_BRACKET_1_7_EV_2_IMG_PLUS = 0x0005C280
    SINGLE_BRACKET_2_0_EV_2_IMG_PLUS = 0x0005C220
    SINGLE_BRACKET_2_3_EV_2_IMG_PLUS = 0x0005C250
    SINGLE_BRACKET_2_5_EV_2_IMG_PLUS = 0x0005C270
    SINGLE_BRACKET_2_7_EV_2_IMG_PLUS = 0x0005C290
    SINGLE_BRACKET_3_0_EV_2_IMG_PLUS = 0x0005C230
    SINGLE_BRACKET_0_3_EV_2_IMG_MINUS = 0x0005C23E
    SINGLE_BRACKET_0_5_EV_2_IMG_MINUS = 0x0005C25E
    SINGLE_BRACKET_0_7_EV_2_IMG_MINUS = 0x0005C27E
    SINGLE_BRACKET_1_0_EV_2_IMG_MINUS = 0x0005C218
    SINGLE_BRACKET_1_3_EV_2_IMG_MINUS = 0x0005C248
    SINGLE_BRACKET_1_5_EV_2_IMG_MINUS = 0x0005C268
    SINGLE_BRACKET_1_7_EV_2_IMG_MINUS = 0x0005C288
    SINGLE_BRACKET_2_0_EV_2_IMG_MINUS = 0x0005C228
    SINGLE_BRACKET_2_3_EV_2_IMG_MINUS = 0x0005C258
    SINGLE_BRACKET_2_5_EV_2_IMG_MINUS = 0x0005C278
    SINGLE_BRACKET_2_7_EV_2_IMG_MINUS = 0x0005C298
    SINGLE_BRACKET_3_0_EV_2_IMG_MINUS = 0x0005C238


class AwCaptureModeU16(AwIntEnum):
    """Capture (Drive) Modes."""
    NORMAL = 0x0001
    CONTINUOUS_HI = 0x0002
    TIMELAPSE = 0x0003
    SELF_TIMER_5S = 0x8003
    SELF_TIMER_10S = 0x8004
    SELF_TIMER_2S = 0x8005
    SELF_PORTRAIT_1_PERSON = 0x8006
    SELF_PORTRAIT_2_PEOPLE = 0x8007
    CONTINUOUS_SELF_TIMER_3_IMG = 0x8008
    CONTINUOUS_SELF_TIMER_5_IMG = 0x8009
    REMOTE_COMMANDER = 0x800A
    MIRROR_UP = 0x800B
    CONTINUOUS_SELF_TIMER_3_IMG_5S = 0x800C
    CONTINUOUS_SELF_TIMER_5_IMG_5S = 0x800D
    CONTINUOUS_SELF_TIMER_3_IMG_2S = 0x800E
    CONTINUOUS_SELF_TIMER_5_IMG_2S = 0x800F
    CONTINUOUS_HI_PLUS = 0x8010
    CONTINUOUS_HI_LIVE = 0x8011
    CONTINUOUS_LO = 0x8012
    CONTINUOUS_SHOOTING = 0x8013
    CONTINUOUS_SHOOTING_SPEED_PRIORITY = 0x8014
    CONTINUOUS_MID = 0x8015
    CONTINUOUS_MID_LIVE = 0x8016
    CONTINUOUS_LO_LIVE = 0x8017
    WHITE_BALANCE_BRACKET_LO = 0x8018
    DRO_BRACKET_LO = 0x8019
    LPF_BRACKET = 0x801A
    WHITE_BALANCE_BRACKET_HI = 0x8028
    DRO_BRACKET_HI = 0x8029
    SPOT_BURST_SHOOTING_LO = 0x8030
    SPOT_BURST_SHOOTING_MID = 0x8031
    SPOT_BURST_SHOOTING_HI = 0x8032
    FOCUS_BRACKET = 0x8040
    CONTINUOUS_BRACKET_0_3_EV_3_IMG = 0x8337
    CONTINUOUS_BRACKET_0_5_EV_3_IMG = 0x8357
    CONTINUOUS_BRACKET_0_7_EV_3_IMG = 0x8377
    CONTINUOUS_BRACKET_1_0_EV_3_IMG = 0x8311
    CONTINUOUS_BRACKET_1_3_EV_3_IMG = 0x8341
    CONTINUOUS_BRACKET_1_5_EV_3_IMG = 0x8361
    CONTINUOUS_BRACKET_1_7_EV_3_IMG = 0x8381
    CONTINUOUS_BRACKET_2_0_EV_3_IMG = 0x8321
    CONTINUOUS_BRACKET_2_3_EV_3_IMG = 0x8351
    CONTINUOUS_BRACKET_2_5_EV_3_IMG = 0x8371
    CONTINUOUS_BRACKET_2_7_EV_3_IMG = 0x8391
    CONTINUOUS_BRACKET_3_0_EV_3_IMG = 0x8331
    CONTINUOUS_BRACKET_0_3_EV_5_IMG = 0x8537
    CONTINUOUS_BRACKET_0_5_EV_5_IMG = 0x8557
    CONTINUOUS_BRACKET_0_7_EV_5_IMG = 0x8577
    CONTINUOUS_BRACKET_1_0_EV_5_IMG = 0x8511
    CONTINUOUS_BRACKET_1_3_EV_5_IMG = 0x8541
    CONTINUOUS_BRACKET_1_5_EV_5_IMG = 0x8561
    CONTINUOUS_BRACKET_1_7_EV_5_IMG = 0x8581
    CONTINUOUS_BRACKET_2_0_EV_5_IMG = 0x8521
    CONTINUOUS_BRACKET_2_3_EV_5_IMG = 0x8551
    CONTINUOUS_BRACKET_2_5_EV_5_IMG = 0x8571
    CONTINUOUS_BRACKET_2_7_EV_5_IMG = 0x8591
    CONTINUOUS_BRACKET_3_0_EV_5_IMG = 0x8531
    CONTINUOUS_BRACKET_0_3_EV_7_IMG = 0x8737
    CONTINUOUS_BRACKET_0_5_EV_7_IMG = 0x8757
    CONTINUOUS_BRACKET_0_7_EV_7_IMG = 0x8777
    CONTINUOUS_BRACKET_1_0_EV_7_IMG = 0x8711
    CONTINUOUS_BRACKET_1_3_EV_7_IMG = 0x8741
    CONTINUOUS_BRACKET_1_5_EV_7_IMG = 0x8761
    CONTINUOUS_BRACKET_1_7_EV_7_IMG = 0x8781
    CONTINUOUS_BRACKET_2_0_EV_7_IMG = 0x8721
    CONTINUOUS_BRACKET_0_3_EV_9_IMG = 0x8937
    CONTINUOUS_BRACKET_0_5_EV_9_IMG = 0x8957
    CONTINUOUS_BRACKET_0_7_EV_9_IMG = 0x8977
    CONTINUOUS_BRACKET_1_0_EV_9_IMG = 0x8911
    CONTINUOUS_BRACKET_0_3_EV_2_IMG_PLUS = 0xC237
    CONTINUOUS_BRACKET_0_5_EV_2_IMG_PLUS = 0xC257
    CONTINUOUS_BRACKET_0_7_EV_2_IMG_PLUS = 0xC277
    CONTINUOUS_BRACKET_1_0_EV_2_IMG_PLUS = 0xC211
    CONTINUOUS_BRACKET_1_3_EV_2_IMG_PLUS = 0xC241
    CONTINUOUS_BRACKET_1_5_EV_2_IMG_PLUS = 0xC261
    CONTINUOUS_BRACKET_1_7_EV_2_IMG_PLUS = 0xC281
    CONTINUOUS_BRACKET_2_0_EV_2_IMG_PLUS = 0xC221
    CONTINUOUS_BRACKET_2_3_EV_2_IMG_PLUS = 0xC251
    CONTINUOUS_BRACKET_2_5_EV_2_IMG_PLUS = 0xC271
    CONTINUOUS_BRACKET_2_7_EV_2_IMG_PLUS = 0xC291
    CONTINUOUS_BRACKET_3_0_EV_2_IMG_PLUS = 0xC231
    CONTINUOUS_BRACKET_0_3_EV_2_IMG_MINUS = 0xC23F
    CONTINUOUS_BRACKET_0_5_EV_2_IMG_MINUS = 0xC25F
    CONTINUOUS_BRACKET_0_7_EV_2_IMG_MINUS = 0xC27F
    CONTINUOUS_BRACKET_1_0_EV_2_IMG_MINUS = 0xC219
    CONTINUOUS_BRACKET_1_3_EV_2_IMG_MINUS = 0xC249
    CONTINUOUS_BRACKET_1_5_EV_2_IMG_MINUS = 0xC269
    CONTINUOUS_BRACKET_1_7_EV_2_IMG_MINUS = 0xC289
    CONTINUOUS_BRACKET_2_0_EV_2_IMG_MINUS = 0xC229
    CONTINUOUS_BRACKET_2_3_EV_2_IMG_MINUS = 0xC259
    CONTINUOUS_BRACKET_2_5_EV_2_IMG_MINUS = 0xC279
    CONTINUOUS_BRACKET_2_7_EV_2_IMG_MINUS = 0xC299
    CONTINUOUS_BRACKET_3_0_EV_2_IMG_MINUS = 0xC239
    SINGLE_BRACKET_0_3_EV_3_IMG = 0x8336
    SINGLE_BRACKET_0_5_EV_3_IMG = 0x8356
    SINGLE_BRACKET_0_7_EV_3_IMG = 0x8376
    SINGLE_BRACKET_1_0_EV_3_IMG = 0x8310
    SINGLE_BRACKET_1_3_EV_3_IMG = 0x8340
    SINGLE_BRACKET_1_5_EV_3_IMG = 0x8360
    SINGLE_BRACKET_1_7_EV_3_IMG = 0x8380
    SINGLE_BRACKET_2_0_EV_3_IMG = 0x8320
    SINGLE_BRACKET_2_3_EV_3_IMG = 0x8350
    SINGLE_BRACKET_2_5_EV_3_IMG = 0x8370
    SINGLE_BRACKET_2_7_EV_3_IMG = 0x8390
    SINGLE_BRACKET_3_0_EV_3_IMG = 0x8330
    SINGLE_BRACKET_0_3_EV_5_IMG = 0x8536
    SINGLE_BRACKET_0_5_EV_5_IMG = 0x8556
    SINGLE_BRACKET_0_7_EV_5_IMG = 0x8576
    SINGLE_BRACKET_1_0_EV_5_IMG = 0x8510
    SINGLE_BRACKET_1_3_EV_5_IMG = 0x8540
    SINGLE_BRACKET_1_5_EV_5_IMG = 0x8560
    SINGLE_BRACKET_1_7_EV_5_IMG = 0x8580
    SINGLE_BRACKET_2_0_EV_5_IMG = 0x8520
    SINGLE_BRACKET_2_3_EV_5_IMG = 0x8550
    SINGLE_BRACKET_2_5_EV_5_IMG = 0x8570
    SINGLE_BRACKET_2_7_EV_5_IMG = 0x8590
    SINGLE_BRACKET_3_0_EV_5_IMG = 0x8530
    SINGLE_BRACKET_0_3_EV_7_IMG = 0x8736
    SINGLE_BRACKET_0_5_EV_7_IMG = 0x8756
    SINGLE_BRACKET_0_7_EV_7_IMG = 0x8776
    SINGLE_BRACKET_1_0_EV_7_IMG = 0x8710
    SINGLE_BRACKET_1_3_EV_7_IMG = 0x8740
    SINGLE_BRACKET_1_5_EV_7_IMG = 0x8760
    SINGLE_BRACKET_1_7_EV_7_IMG = 0x8780
    SINGLE_BRACKET_2_0_EV_7_IMG = 0x8720
    SINGLE_BRACKET_0_3_EV_9_IMG = 0x8936
    SINGLE_BRACKET_0_5_EV_9_IMG = 0x8956
    SINGLE_BRACKET_0_7_EV_9_IMG = 0x8976
    SINGLE_BRACKET_1_0_EV_9_IMG = 0x8910
    SINGLE_BRACKET_0_3_EV_2_IMG_PLUS = 0xC236
    SINGLE_BRACKET_0_5_EV_2_IMG_PLUS = 0xC256
    SINGLE_BRACKET_0_7_EV_2_IMG_PLUS = 0xC276
    SINGLE_BRACKET_1_0_EV_2_IMG_PLUS = 0xC210
    SINGLE_BRACKET_1_3_EV_2_IMG_PLUS = 0xC240
    SINGLE_BRACKET_1_5_EV_2_IMG_PLUS = 0xC260
    SINGLE_BRACKET_1_7_EV_2_IMG_PLUS = 0xC280
    SINGLE_BRACKET_2_0_EV_2_IMG_PLUS = 0xC220
    SINGLE_BRACKET_2_3_EV_2_IMG_PLUS = 0xC250
    SINGLE_BRACKET_2_5_EV_2_IMG_PLUS = 0xC270
    SINGLE_BRACKET_2_7_EV_2_IMG_PLUS = 0xC290
    SINGLE_BRACKET_3_0_EV_2_IMG_PLUS = 0xC230
    SINGLE_BRACKET_0_3_EV_2_IMG_MINUS = 0xC23E
    SINGLE_BRACKET_0_5_EV_2_IMG_MINUS = 0xC25E
    SINGLE_BRACKET_0_7_EV_2_IMG_MINUS = 0xC27E
    SINGLE_BRACKET_1_0_EV_2_IMG_MINUS = 0xC218
    SINGLE_BRACKET_1_3_EV_2_IMG_MINUS = 0xC248
    SINGLE_BRACKET_1_5_EV_2_IMG_MINUS = 0xC268
    SINGLE_BRACKET_1_7_EV_2_IMG_MINUS = 0xC288
    SINGLE_BRACKET_2_0_EV_2_IMG_MINUS = 0xC228
    SINGLE_BRACKET_2_3_EV_2_IMG_MINUS = 0xC258
    SINGLE_BRACKET_2_5_EV_2_IMG_MINUS = 0xC278
    SINGLE_BRACKET_2_7_EV_2_IMG_MINUS = 0xC298
    SINGLE_BRACKET_3_0_EV_2_IMG_MINUS = 0xC238


class AwFlashMode(AwIntEnum):
    """Flash Mode Settings."""
    AUTO = 0x0001
    OFF = 0x0002
    FILL = 0x0003
    RED_EYE_AUTO = 0x0004
    RED_EYE_FILL = 0x0005
    EXTERNAL_SYNC = 0x0006
    SLOW_SYNC = 0x8001
    REAR_SYNC = 0x8003
    WIRELESS = 0x8004
    HSS_AUTO = 0x8021
    HSS_FILL = 0x8022
    HSS_WL = 0x8024
    SLOW_SYNC_RED_EYE_ON = 0x8031
    SLOW_SYNC_RED_EYE_OFF = 0x8032
    SLOW_SYNC_WIRELESS = 0x8041
    REAR_SYNC_WIRELESS = 0x8042


class AwWhiteBalance(AwIntEnum):
    """White Balance Settings."""
    MANUAL = 0x0001
    AWB = 0x0002
    ONE_PUSH_AUTO = 0x0003
    DAYLIGHT = 0x0004
    FLUORESCENT = 0x0005
    TUNGSTEN = 0x0006
    FLASH = 0x0007
    FLUORESCENT_WARM_WHITE = 0x8001
    FLUORESCENT_COOL_WHITE = 0x8002
    FLUORESCENT_DAY_WHITE = 0x8003
    FLUORESCENT_DAYLIGHT = 0x8004
    CLOUDY = 0x8010
    SHADE = 0x8011
    CUSTOM_TEMP = 0x8012
    CUSTOM_1 = 0x8020
    CUSTOM_2 = 0x8021
    CUSTOM_3 = 0x8022
    CUSTOM = 0x8023
    UNDERWATER_AUTO = 0x8030


class AwFocusMode(AwIntEnum):
    """Focus Modes."""
    MANUAL = 0x0001
    AF_S = 0x0002
    AF_C = 0x8004
    AF_AUTO = 0x8005
    DMF = 0x8006


class AwFocusArea(AwIntEnum):
    """Focus Area Settings."""
    WIDE = 0x0001
    ZONE = 0x0002
    CENTER = 0x0003
    FLEXIBLE_SPOT_S = 0x0101
    FLEXIBLE_SPOT_M = 0x0102
    FLEXIBLE_SPOT_L = 0x0103
    EXPAND_FLEXIBLE_SPOT = 0x0104
    FLEXIBLE_SPOT = 0x0105
    FLEXIBLE_SPOT_XS = 0x0106
    FLEXIBLE_SPOT_XL = 0x0107
    FLEXIBLE_SPOT_FREE_1 = 0x1101
    FLEXIBLE_SPOT_FREE_2 = 0x1102
    FLEXIBLE_SPOT_FREE_3 = 0x1103
    TRACKING_WIDE = 0x0201
    TRACKING_ZONE = 0x0202
    TRACKING_CENTER = 0x0203
    TRACKING_FLEXIBLE_SPOT_S = 0x0204
    TRACKING_FLEXIBLE_SPOT_M = 0x0205
    TRACKING_FLEXIBLE_SPOT_L = 0x0206
    TRACKING_EXPAND_FLEXIBLE_SPOT = 0x0207
    TRACKING_FLEXIBLE_SPOT = 0x0208
    TRACKING_FLEXIBLE_SPOT_XS = 0x0209
    TRACKING_FLEXIBLE_SPOT_XL = 0x020A
    TRACKING_FLEXIBLE_SPOT_FREE_1 = 0x1201
    TRACKING_FLEXIBLE_SPOT_FREE_2 = 0x1202
    TRACKING_FLEXIBLE_SPOT_FREE_3 = 0x1203


class AwAutoFocusStatus(AwIntEnum):
    """Auto Focus Status."""
    UNLOCK = 0x01
    AFS_LOCKED = 0x02
    AFS_FAILED = 0x03
    AFC_TRACKING = 0x05
    AFC_FOCUSED = 0x06
    AFC_FAILED = 0x07


class AwAspectRatio(AwIntEnum):
    """Image Aspect Ratio."""
    RATIO_3_2 = 0x01
    RATIO_16_9 = 0x02
    RATIO_4_3 = 0x03
    RATIO_1_1 = 0x04


class AwShutterType(AwIntEnum):
    """Shutter Type (Mechanical/Electronic)."""
    AUTO = 0x01
    MECHANICAL = 0x02
    ELECTRONIC = 0x03


class AwImageStabilization(AwIntEnum):
    """Image Stabilization (Steady Shot)."""
    OFF = 0x01
    ON = 0x02


class AwSilentMode(AwIntEnum):
    """Silent Mode Setting."""
    OFF = 0x01
    ON = 0x02


class AwImageFileFormat(AwIntEnum):
    """Image File Format."""
    NOT_APPLICABLE = 0x00
    RAW = 0x01
    RAW_AND_JPEG = 0x02
    JPEG = 0x03
    RAW_AND_HEIF = 0x04
    HEIF = 0x05


class AwRawFileType(AwIntEnum):
    """RAW File Type."""
    COMPRESSED = 0x01
    LOSSLESS_L = 0x02
    LOSSLESS_M = 0x03
    LOSSLESS_S = 0x04
    UNCOMPRESSED = 0x05
    LOSSLESS = 0x06
    COMPRESSED_HQ = 0x07


class AwImageQuality(AwIntEnum):
    """Image Compression Quality."""
    EXTRA_FINE = 0x01
    FINE = 0x02
    STANDARD = 0x03
    LIGHT = 0x04


class AwCompressionSetting(AwIntEnum):
    """Image Compression Settings."""
    ECO_LIGHT = 0x01
    STD = 0x02
    FINE = 0x03
    XFINE = 0x04
    RAW = 0x10
    RAW_AND_JPG_LIGHT = 0x11
    RAW_AND_JPG_STD = 0x12
    RAW_AND_JPG_FINE = 0x13
    RAW_AND_JPG_XFINE = 0x14
    RAW_COMPRESSED = 0x20
    RAW_COMPRESSED_AND_JPG_FINE = 0x23
    HEIF_ECO_LIGHT = 0x31
    HEIF_STD = 0x32
    HEIF_FINE = 0x33
    HEIF_XFINE = 0x34
    RAW_AND_HEIF_LIGHT = 0x41
    RAW_AND_HEIF_STD = 0x42
    RAW_AND_HEIF_FINE = 0x43
    RAW_AND_HEIF_XFINE = 0x44


class AwImageCompressedFileType(AwIntEnum):
    """Compressed Image File Type."""
    JPEG = 0x01
    HEIF_422 = 0x02
    HEIF_420 = 0x03


class AwDroMode(AwIntEnum):
    """DRO / HDR Mode."""
    OFF = 0x01
    DRO = 0x02
    DRO_PLUS = 0x10
    DRO_LVL1 = 0x11
    DRO_LVL2 = 0x12
    DRO_LVL3 = 0x13
    DRO_LVL4 = 0x14
    DRO_LVL5 = 0x15
    DRO_AUTO = 0x1F
    HDR_AUTO = 0x20
    HDR_1_0EV = 0x21
    HDR_2_0EV = 0x22
    HDR_3_0EV = 0x23
    HDR_4_0EV = 0x24
    HDR_5_0EV = 0x25
    HDR_6_0EV = 0x26


class AwIso(AwIntEnum):
    """ISO Settings."""
    AUTO = 0x00ffffff


class AwPcRemoteSaveDest(AwIntEnum):
    """PC Remote Save Destination."""
    PC = 0x0001
    CARD = 0x0010
    PC_AND_CARD = 0x0011


class AwLiveViewSettingEffect(AwIntEnum):
    """Live View Setting Effect."""
    NA = 0x00
    ON = 0x01
    OFF = 0x02


class AwPcSaveImage(AwIntEnum):
    """PC Save Image Settings."""
    RAW_AND_JPEG = 0x01
    JPEG_ONLY = 0x02
    RAW_ONLY = 0x03
    RAW_AND_HEIF = 0x04
    HEIF_ONLY = 0x05


class AwLockState(AwIntEnum):
    """Lock  State (e.g. for AE/AF Lock)."""
    UNLOCKED = 0x01
    LOCKED = 0x02


###################################
# Logging
###################################

class AwLogLevel(AwIntEnum):
    """Logging Levels."""
    TRACE = 4
    DEBUG = 3
    INFO = 2
    WARNING = 1
    ERROR = 0

@ffi.callback("void(struct AwLog*, AwLogLevel, const char*)")
def _aw_log_callback(logger: typing.Any, level: int, message: typing.Any) -> None:
    py_message = ffi.string(message).decode("utf-8", errors="surrogateescape")
    # Route to the Python _log function, but we need to convert level
    # since we updated AwLogLevel to match C, it should be direct
    log(AwLogLevel(level), py_message)

def _aw_log_func_default(level: AwLogLevel, message: str):
    level_name = level.name
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    try:
        print(f"[{timestamp}] [{level_name}] {message}")
    except UnicodeEncodeError:
        # Fallback for environments with restricted encoding (like Windows cmd with cp1252)
        safe_message = message.encode(sys.stdout.encoding or 'ascii', errors='replace').decode(sys.stdout.encoding or 'ascii')
        print(f"[{timestamp}] [{level_name}] {safe_message}")

_aw_log_level: AwLogLevel = AwLogLevel.WARNING
_aw_log_func: typing.Callable[[AwLogLevel, str], None] = _aw_log_func_default
_aw_log_instances: typing.List[weakref.ReferenceType] = []

def _aw_update_all_loggers() -> None:
    global _aw_log_instances
    new_instances = []
    for ref in _aw_log_instances:
        obj = ref()
        if obj is not None:
            if hasattr(obj, "_devlist"):
                obj._devlist[0].logger.level = _aw_log_level.value
            elif hasattr(obj, "_ffi"):
                obj._ffi[0].logger.level = _aw_log_level.value
            new_instances.append(ref)
    _aw_log_instances = new_instances

def log(level: AwLogLevel, message: str) -> None:
    """
    Log a message with the specified level.

    Args:
        level: The log level.
        message: The message to log.
    """
    if level <= _aw_log_level:
        _aw_log_func(level, message)

def log_debug(message: str) -> None:
    """Log a debug message."""
    log(AwLogLevel.DEBUG, message)

def log_info(message: str) -> None:
    """Log an info message."""
    log(AwLogLevel.INFO, message)

def log_warn(message: str) -> None:
    """Log a warning message."""
    log(AwLogLevel.WARNING, message)

def log_error(message: str) -> None:
    """Log an error message."""
    log(AwLogLevel.ERROR, message)

def log_set_level(level: AwLogLevel) -> None:
    """
    Set the global log level.

    Args:
        level: The new log level.
    """
    global _aw_log_level
    _aw_log_level = level
    _aw_update_all_loggers()

def log_set_func(log_func: typing.Callable[[AwLogLevel, str], None]):
    """
    Set a custom logging function.

    Args:
        log_func: A function that takes a log level and a message string.
    """
    global _aw_log_func
    _aw_log_func = log_func


###################################
# Controls
###################################

class AwControlCode(AwIntEnum):
    """Camera Control Codes (OpCodes)."""
    SHUTTER_HALF_PRESS = 0xD2C1
    SHUTTER = 0xD2C2
    AE_LOCK = 0xD2C3
    AFL_BUTTON = 0xD2C4
    SHUTTER_ONE_RESET = 0xD2C5
    SHUTTER_ONE = 0xD2C7
    MOVIE_RECORD = 0xD2C8
    FEL_BUTTON = 0xD2C9
    MEDIA_FORMAT = 0xD2CA
    FOCUS_MAGNIFIER = 0XD2CB
    FOCUS_MAGNIFIER_CANCEL = 0XD2CC
    REMOTE_KEY_UP = 0XD2CD
    REMOTE_KEY_DOWN = 0XD2CE
    REMOTE_KEY_LEFT = 0XD2CF
    REMOTE_KEY_RIGHT =  0XD2D0
    REMOTE_KEY_MENU =  0xD2FF
    MANUAL_FOCUS_ADJUST = 0xD2D1
    AUTO_FOCUS_HOLD = 0xD2D2
    PIXEL_SHIFT_SHOOT_CANCEL = 0xD2D3
    PIXEL_SHIFT_SHOOT = 0xD2D4
    HFR_STANDBY = 0xD2D5
    HFR_RECORD_CANCEL = 0xD2D6
    FOCUS_STEP_NEAR = 0xD2D7
    FOCUS_STEP_FAR = 0xD2D8
    AWB_LOCK = 0xD2D9
    FOCUS_AREA_X_Y = 0xD2DC
    ZOOM = 0xD2DD
    CUSTOM_WB_CAPTURE_STANDBY = 0xD2DF
    CUSTOM_WB_CAPTURE_STANDBY_CANCEL = 0xD2E0
    CUSTOM_WB_CAPTURE = 0xD2E1
    FORMAT_MEDIA = 0xD2E2
    REMOTE_TOUCH_XY = 0xD2E4
    REMOTE_TOUCH_CANCEL = 0xD2E5
    SHUTTER_BOTH = 0xD2E6
    FORMAT_MEDIA_CANCEL = 0xD2E7
    SAVE_ZOOM_AND_FOCUS_POSITION = 0xD2E9
    LOAD_ZOOM_AND_FOCUS_POSITION = 0xD2EA
    APS_C_FULL_TOGGLE = 0xD2EB
    COLOR_TEMPERATURE_STEP = 0xD2EC
    WHITE_BALANCE_TINT_STEP = 0xD2ED
    FOCUS_OPERATION = 0xD2EF
    FLICKER_SCAN = 0xD2F1
    SETTINGS_RESET = 0xD2F3
    PIXEL_MAPPING = 0xD300
    POWER_OFF = 0xD301
    TIME_CODE_PRESET_RESET = 0xD302
    USER_BIT_PRESET_RESET = 0xD303
    SENSOR_CLEANING = 0xD304
    RESET_PICTURE_PROFILE = 0xD305
    RESET_CREATIVE_LOOK = 0xD306
    REMOTE_BUTTON = 0xD309
    REMOTE_BUTTON_MULTI = 0xD30A
    REMOTE_DIAL_ADJUST = 0xD30B
    REMOTE_LEVER_ADJUST = 0xD30C
    SHUTTER_ECS_NUMBER_STEP = 0xF000
    MOVIE_RECORD_TOGGLE = 0xF001
    FOCUS_POSITION_CANCEL = 0xF002

class AwPropertyCode(AwIntEnum):
    """Camera Property Codes (Device Property Codes)."""
    COMPRESSION_SETTING = 0x5004
    WHITE_BALANCE = 0x5005
    F_NUMBER = 0x5007
    FOCUS_MODE = 0x500A
    EXPOSURE_METERING_MODE = 0x500B
    FLASH_MODE = 0x500C
    EXPOSURE_PROGRAM_MODE = 0x500E
    EXPOSURE_COMPENSATION = 0x5010
    CAPTURE_MODE = 0x5013
    IRIS_MODE = 0xD001
    IRIS_CLOSE_MODE = 0xD002
    IRIS_DISPLAY_UNIT = 0xD003
    FOCAL_DISTANCE_METER = 0xD004
    FOCAL_DISTANCE_FEET = 0xD005
    FOCAL_DISTANCE_UNIT = 0xD006
    FOCUS_MODE_SETTING = 0xD007
    FOCUS_SPEED_RANGE = 0xD008
    DIGITAL_ZOOM_SCALE = 0xD00A
    ZOOM_DISTANCE = 0xD00B
    WHITE_BALANCE_MODE = 0xD00C
    SHUTTER_SETTING = 0xD00F
    SHUTTER_MODE = 0xD010
    SHUTTER_MODE_STATUS = 0xD011
    SHUTTER_ELECTRONIC_MODE = 0xD012
    SHUTTER_MODE_SETTING = 0xD013
    SHUTTER_SLOW = 0xD014
    SHUTTER_SLOW_FRAMES = 0xD015
    SHUTTER_SPEED_VALUE = 0xD016
    SHUTTER_SPEED_CURRENT = 0xD017
    ND_FILTER = 0xD018
    ND_FILTER_MODE = 0xD019
    ND_FILTER_MODE_SETTING = 0xD01A
    ND_FILTER_VALUE = 0xD01B
    GAIN_CONTROL = 0xD01C
    GAIN_UNIT = 0xD01D
    GAIN_DB_VALUE = 0xD01E
    EXPOSURE_INDEX = 0xD022
    ISO_CURRENT = 0xD023
    MOVIE_FILE_FORMAT_PROXY = 0xD027
    PLAYBACK_MEDIA = 0xD042
    TOUCH_OPERATION = 0xD047
    TIME_CODE_FORMAT = 0xD0D5
    IMAGE_STABILIZATION = 0xD0D9
    SILENT_MODE = 0xD0DB
    SILENT_MODE_APERTURE_DRIVE_AF = 0xD0DC
    SILENT_MODE_POWER_OFF = 0xD0DD
    SILENT_MODE_AUTO_PIXEL_MAPPING = 0xD0DE
    SHUTTER_TYPE = 0xD0DF
    CREATIVE_LOOK = 0xD0FA
    FILE_NAME_PREFIX = 0xD1CA
    SHUTTER_RELEASE_TIMING = 0xD156
    PHOTOGRAPHER = 0xD1CE
    COPYRIGHT = 0xD1CF
    FLASH_COMPENSATION = 0xD200
    DRO_HDR_MODE = 0xD201
    IMAGE_SIZE = 0xD203
    OSD_IMAGE_MODE = 0xD207
    BUTTON_LIST = 0xD208
    BUTTON_LIST_MULTI = 0xD209
    DIAL_LIST = 0xD20A
    LEVER_LIST = 0xD20B
    SHUTTER_SPEED = 0xD20D
    BATTERY_LEVEL = 0xD20E
    CUSTOM_COLOR_TEMP = 0xD20F
    WHITE_BALANCE_GM = 0xD210
    ASPECT_RATIO = 0xD211
    AUTO_FOCUS_STATUS = 0xD213
    PREDICTED_MAX_FILE_SIZE = 0xD214
    PENDING_FILES = 0xD215
    AE_LOCK_STATUS = 0xD217
    BATTERY_REMAINING = 0xD218
    PICTURE_EFFECT = 0xD21B
    WHITE_BALANCE_AB = 0xD21C
    MOVIE_REC_STATE = 0xD21D
    ISO = 0xD21E
    FEL_LOCK_STATUS = 0xD21F
    LIVE_VIEW_STATUS = 0xD221
    IMAGE_SAVE_DESTINATION = 0xD222
    DATE_TIME_SET = 0xD223
    FOCUS_AREA = 0xD22C
    FOCUS_MAGNIFY_SCALE = 0xD22f
    FOCUS_MAGNIFY_POS = 0xD230
    LIVE_VIEW_SETTING_EFFECT = 0xD231
    FOCUS_AREA_POS_OLD = 0xD232
    MANUAL_FOCUS_ADJUST_ENABLED = 0xD235
    PIXEL_SHIFT_SHOOTING_MODE = 0xD239
    PIXEL_SHIFT_SHOOTING_NUMBER = 0xD23A
    PIXEL_SHIFT_SHOOTING_INTERVAL = 0xD23B
    PIXEL_SHIFT_SHOOTING_STATUS = 0xD23C
    PIXEL_SHIFT_SHOOTING_PROGRESS = 0xD23D
    PICTURE_PROFILE = 0xD23F
    CREATIVE_STYLE = 0xD240
    MOVIE_FILE_FORMAT = 0xD241
    MOVIE_QUALITY = 0xD242
    MEDIA_SLOT1_STATUS = 0xD248
    FOCUS_POSITION = 0xD24C
    AWB_LOCK_STATUS = 0xD24E
    INTERVAL_RECORD_MODE = 0xD24F
    INTERVAL_RECORD_STATUS = 0xD250
    DEVICE_OVERHEATING_STATE = 0xD251
    IMAGE_QUALITY = 0xD252
    IMAGE_FILE_FORMAT = 0xD253
    FOCUS_MAGNIFY = 0xD254
    AF_TRACKING_SENS = 0xD255
    MEDIA_SLOT2_STATUS = 0xD256
    DIAL_MODE = 0xD25A
    ZOOM_OPERATION_ENABLED = 0xD25B
    ZOOM_SCALE = 0xD25C
    ZOOM_BAR_INFO = 0xD25D
    ZOOM_SETTING = 0xD25F
    ZOOM_TYPE_STATUS = 0xD260
    WIRELESS_FLASH = 0xD262
    RED_EYE_REDUCTION = 0xD263
    REMOTE_RESTRICT_STATUS = 0xD264
    LIVE_VIEW_AREA = 0xD267
    IMAGE_TRANSFER_SIZE = 0xD268
    PC_SAVE_IMAGE = 0xD269
    LIVE_VIEW_QUALITY = 0xD26A
    CAMERA_SETTING_SAVE_ENABLED = 0xD271
    CAMERA_SETTING_READ_ENABLED = 0xD272
    CAMERA_SETTING_SAVE_READ_STATE = 0xD273
    FORMAT_MEDIA_SLOT1_ENABLED = 0xD279
    FORMAT_MEDIA_SLOT2_ENABLED = 0xD27A
    FORMAT_MEDIA_PROGRESS = 0xD27B
    TOUCH_FOCUS_OPERATION = 0xD283
    REMOTE_TOUCH_ENABLED = 0xD284
    REMOTE_TOUCH_CANCEL_ENABLED = 0xD285
    MOVIE_FRAME_RATE = 0xD286
    IMAGE_COMPRESSED_FILE_TYPE = 0xD287
    RAW_FILE_TYPE = 0xD288
    FORMAT_MEDIA_QUICK_SLOT1_ENABLED = 0xD292
    FORMAT_MEDIA_QUICK_SLOT2_ENABLED = 0xD293
    FORMAT_MEDIA_CANCEL_ENABLED = 0xD294
    CONTENTS_TRANSFER_ENABLED = 0xD295
    FOCUS_POSITION_ABS_SET = 0xE042
    FOCUS_POSITION_ABS = 0xE043
    LENS_INFORMATION_ENABLED = 0xE086

class AwDeviceInfo:
    """Information about a discovered camera device."""
    def __init__(self, ffi_device: typing.Any) -> None:
        self.manufacturer: str = _convert_m_str(ffi_device.manufacturer)
        self.product: str = _convert_m_str(ffi_device.product)
        self.serial: str = _convert_m_str(ffi_device.serial)
        self.ip_address: str = _convert_m_str(ffi_device.ipAddress)
        self.usb_vid: int = ffi_device.usbVID
        self.usb_pid: int = ffi_device.usbPID
        self.usb_version: str = _usb_bcd_version_as_string(ffi_device.usbVersion)
        self._ffi_device = ffi_device

class AwPtpProperty:
    """
    Represents a PTP (Picture Transfer Protocol) property of a camera.

    Properties correspond to camera settings such as ISO, shutter speed, or white
    balance. Each property has a code, a data type, and a current value.
    """
    def __init__(self, control: 'AwControl', ffi_control: typing.Any, allocator: typing.Any, ffi_property: typing.Any) -> None:
        self._control: 'AwControl' = control
        self._allocator = allocator
        self._ffi_control = ffi_control
        self._ffi_property = ffi_property
        self.code: int = ffi_property.propCode
        self._enum_type = self._get_enum_type()

    def _get_enum_type(self) -> typing.Optional[typing.Type[AwIntEnum]]:
        dt = self._ffi_property.dataType
        if self.code == AwPropertyCode.DIAL_MODE:
            return AwDialMode
        if self.code == AwPropertyCode.EXPOSURE_PROGRAM_MODE:
            return AwExposureProgramModeU16 if dt == PtpDataType.UINT16 else AwExposureProgramMode
        if self.code == AwPropertyCode.CAPTURE_MODE:
            return AwCaptureModeU16 if dt == PtpDataType.UINT16 else AwCaptureMode
        if self.code == AwPropertyCode.WHITE_BALANCE:
            return AwWhiteBalance
        if self.code == AwPropertyCode.FLASH_MODE:
            return AwFlashMode
        if self.code == AwPropertyCode.FOCUS_MODE:
            return AwFocusMode
        if self.code == AwPropertyCode.FOCUS_AREA:
            return AwFocusArea
        if self.code == AwPropertyCode.AUTO_FOCUS_STATUS:
            return AwAutoFocusStatus
        if self.code == AwPropertyCode.ASPECT_RATIO:
            return AwAspectRatio
        if self.code == AwPropertyCode.SHUTTER_TYPE:
            return AwShutterType
        if self.code == AwPropertyCode.IMAGE_STABILIZATION:
            return AwImageStabilization
        if self.code == AwPropertyCode.SILENT_MODE:
            return AwSilentMode
        if self.code == AwPropertyCode.IMAGE_FILE_FORMAT:
            return AwImageFileFormat
        if self.code == AwPropertyCode.RAW_FILE_TYPE:
            return AwRawFileType
        if self.code == AwPropertyCode.IMAGE_QUALITY:
            return AwImageQuality
        if self.code == AwPropertyCode.COMPRESSION_SETTING:
            return AwCompressionSetting
        if self.code == AwPropertyCode.IMAGE_COMPRESSED_FILE_TYPE:
            return AwImageCompressedFileType
        if self.code == AwPropertyCode.DRO_HDR_MODE:
            return AwDroMode
        if self.code == AwPropertyCode.ISO:
            return AwIso
        if self.code == AwPropertyCode.IMAGE_SAVE_DESTINATION:
            return AwPcRemoteSaveDest
        if self.code == AwPropertyCode.LIVE_VIEW_SETTING_EFFECT:
            return AwLiveViewSettingEffect
        if self.code == AwPropertyCode.PC_SAVE_IMAGE:
            return AwPcSaveImage
        if self.code in (AwPropertyCode.AE_LOCK_STATUS, AwPropertyCode.AWB_LOCK_STATUS, AwPropertyCode.FEL_LOCK_STATUS):
            return AwLockState
        return None

    def get_value_as_str(self) -> typing.Optional[str]:
        """
        Get the current value of the property as a human-readable string.

        For enums, this looks up the known string representation. For string types,
        it returns the string directly. For some numeric properties, it returns a
        formatted string (e.g., '0.3m' for focal distance).

        Returns:
            The property value as a string, or None if it could not be retrieved.
        """
        out_str = ffi.new("MStr[1]")
        ok = lib.AwControl_GetPropertyValueAsKnownStr(self._ffi_control,  self._allocator, self._ffi_property, out_str)
        if not ok:
            return None
        result = _convert_m_str(out_str[0])
        lib.Aw_StrFree(self._allocator, out_str)
        return result

    def _convert_value(self, dt, v):
        raw_val = None
        if dt == PtpDataType.INT8:
            raw_val = v.i8
        elif dt == PtpDataType.UINT8:
            raw_val = v.u8
        elif dt == PtpDataType.INT16:
            raw_val = v.i16
        elif dt == PtpDataType.UINT16:
            raw_val = v.u16
        elif dt == PtpDataType.INT32:
            raw_val = v.i32
        elif dt == PtpDataType.UINT32:
            raw_val = v.u32
        elif dt == PtpDataType.INT64:
            raw_val = v.i64
        elif dt == PtpDataType.UINT64:
            raw_val = v.u64
        elif dt == PtpDataType.STR:
            raw_val = _convert_m_str(v.str)

        if self._enum_type is not None:
            try:
                return self._enum_type(raw_val)
            except ValueError:
                return raw_val

        return raw_val

    def get_value(self) -> typing.Any:
        """
        Get the current value of the property, automatically converted to a Python type.

        Returns:
            The property value.
        """
        # Check if we have a known enum for this property
        dt = self._ffi_property.dataType
        v = self._ffi_property.value
        raw_val = self._convert_value(dt, v)
        if raw_val is None:
            out_str = ffi.new("MStr[1]")
            ok = lib.AwControl_GetPropertyValueAsKnownStr(self._ffi_control,  self._allocator, self._ffi_property, out_str)
            if ok:
                result = _convert_m_str(out_str[0])
                lib.Aw_StrFree(self._allocator, out_str)
                return result

        return raw_val

    def get_label(self) -> typing.Optional[str]:
        """
        Get the human-readable label for this property.

        Returns:
            The property label.
        """
        return _convert_c_str(lib.AwGetPropertyLabel(self.code))

    def get_id(self) -> typing.Optional[str]:
        out_str = ffi.new("MStr[1]")
        ok = lib.AwControl_GetPropertyId(self._ffi_control, self._ffi_property, out_str)
        if ok:
            result = _convert_m_str(out_str[0])
            lib.Aw_StrFree(self._allocator, out_str)
            return result
        return None

    def get_enums(self) -> typing.List[typing.Tuple[typing.Any, typing.Optional[str]]]:
        """
        Get the list of allowed values (enums) for this property.

        Returns:
            A list of tuples (value, label).
        """
        enums = ffi.new("AwPtpPropValueEnums*")
        ok = lib.AwControl_GetEnumsForProperty(self._ffi_control, self._allocator, self._ffi_property, 0, enums)
        if not ok:
            return []

        result = []
        dt = self._ffi_property.dataType
        for i in range(enums.values.size):
            e = enums.values.data[i]
            val = self._convert_value(dt, e.propValue)
            if e.str.size > 0:
                val_as_str = _convert_m_str(e.str)
                result.append((val, val_as_str))
            else:
                result.append((val, None))

        lib.AwControl_FreePropValueEnums(self._ffi_control, enums)
        return result

    def set_value(self, value: typing.Union[str, int, AwIntEnum]) -> bool:
        """
        Set the value of the property.

        Args:
            value: The new value (either an integer, a string, or an AwIntEnum subclass specific to the property)).

        Returns:
            True if the property was set successfully, False otherwise.
        """
        if isinstance(value, str):
            # If we have an enum type, try to see if the string matches any enum name or value
            if self._enum_type is not None:
                try:
                    # Try to match by name
                    value = self._enum_type[value]
                except KeyError:
                    # If not a name, maybe it's a value represented as string? 
                    # set_property_str below will handle it if it's a known string value for the property.
                    pass

            if isinstance(value, str):
                res = self._control.set_property_str(self._ffi_property, value)
                return res.code == lib.AW_RESULT_OK

        if isinstance(value, AwIntEnum) and self._enum_type is not None:
            # Validate that the enum value is of the correct type for this property
            if type(value) != self._enum_type:
                return False

        if isinstance(value, (int, AwIntEnum)):
            int_value = int(value)
            
            prop_value = ffi.new("AwPtpPropValue*")
            dt = self._ffi_property.dataType
            if dt == PtpDataType.INT8: prop_value.i8 = int_value
            elif dt == PtpDataType.UINT8: prop_value.u8 = int_value
            elif dt == PtpDataType.INT16: prop_value.i16 = int_value
            elif dt == PtpDataType.UINT16: prop_value.u16 = int_value
            elif dt == PtpDataType.INT32: prop_value.i32 = int_value
            elif dt == PtpDataType.UINT32: prop_value.u32 = int_value
            elif dt == PtpDataType.INT64: prop_value.i64 = int_value
            elif dt == PtpDataType.UINT64: prop_value.u64 = int_value
            else:
                return False
            res = lib.AwControl_SetPropertyValue(self._ffi_control, self._ffi_property, prop_value[0])
            return res.code == lib.AW_RESULT_OK
        return False


class AwPtpControl:
    """
    Represents a PTP control (command) of a camera.

    Controls correspond to actions such as triggering the shutter, starting movie
    recording, or adjusting focus step.
    """
    def __init__(self, control: 'AwControl', ffi_control: typing.Any,
                 allocator: typing.Any, ffi_ptp_control: typing.Any) -> None:
        self._control: 'AwControl' = control
        self._allocator = allocator
        self._ffi_control = ffi_control
        self._ffi_property = ffi_ptp_control
        self.code: int = ffi_ptp_control.controlCode
        self.label: typing.Optional[str] = _convert_c_str(ffi_ptp_control.label)


class AwCapturedFile:
    """Represents a file captured by the camera."""
    def __init__(self, filename: str, data: bytes) -> None:
        self.filename = filename
        self.data = data


class AwCaptureStage(enum.Enum):
    """Stages of the image capture workflow."""
    WAIT_FOCUS = 0
    TRIGGER = 1
    WAIT_FILE = 2
    WAIT_DOWNLOAD = 3


class AwImageCaptureWorkflow:
    """Configuration for the image capture workflow."""
    def __init__(self) -> None:
        self.loop_wait_time: float = 0.1
        self.wait_focus: float = 3.0
        self.wait_file: float = 3.0
        self.wait_download: float = 3.0

    def get_wait_time(self, stage: AwCaptureStage) -> typing.Tuple[float, float]:
        if stage == AwCaptureStage.WAIT_FOCUS:
            return self.wait_focus, self.loop_wait_time
        elif stage == AwCaptureStage.WAIT_FILE:
            return self.wait_file, self.loop_wait_time
        elif stage == AwCaptureStage.WAIT_DOWNLOAD:
            return self.wait_download, self.loop_wait_time
        else:
            raise ValueError("Invalid capture stage")

    def wait(self, seconds: float, stage: AwCaptureStage) -> None:
        time.sleep(seconds)


class AwControl:
    """
    Main interface for controlling a Sony PTP (Picture Transfer Protocol) camera device.

    This class encapsulates the state and resources needed to interact with a camera,
    managing the connection, session, transactions, and data exchange.
    """
    def __init__(self, ffi_device: typing.Any, allocator: typing.Any,
                 device: typing.Optional['AwDevice'] = None) -> None:
        self._ffi = ffi.new("AwControl[1]")
        self._allocator = allocator
        self._live_view_mem = None
        self._device: typing.Optional['AwDevice'] = device
        lib.AwControl_Init(self._ffi, ffi_device, allocator)
        self._ffi[0].logger.logFunc = _aw_log_callback
        self._ffi[0].logger.level = _aw_log_level.value
        _aw_log_instances.append(weakref.ref(self))

    def connect(self, sony_protocol_version: AwSonyProtocolVersion = AwSonyProtocolVersion.V3) -> typing.Any:
        """
        Connect to the Sony camera over PTP.

        Opens a new session, performs authentication, retrieves device and property
        information, and prepares the device for control operations.

        Args:
            sony_protocol_version: The Sony PTP protocol version to use.
                V3 (default) is supported by 2020+ cameras and offers more properties
                and API improvements over V2.

        Returns:
            An AwResult indicating success or failure.
        """
        log_info(f"Connecting to device with protocol version {sony_protocol_version.name}")
        return lib.AwControl_Connect(self._ffi, sony_protocol_version.value)

    def close(self) -> None:
        """
        Close the camera connection and release resources.

        Cleans up the control structure and closes the PTP session. The underlying
        device transport should be closed after this.
        """
        if self._ffi is not None:
            log_info("Cleaning up camera control")
            lib.AwControl_Cleanup(self._ffi)
            self._ffi = None
        if self._live_view_mem is not None:
            lib.Aw_MemIOFree(self._live_view_mem)
            self._live_view_mem = None

    def update_properties(self, full_refresh: bool = True) -> bool:
        """
        Update the local cache of camera properties by pulling latest values from the device.

        Args:
            full_refresh: If True, performs a full refresh of all properties.
                If False, only changed properties are refreshed. Note that some
                properties like pending file count may require a full refresh to update
                correctly on certain cameras.

        Returns:
            True if successful, False otherwise.
        """
        log(AwLogLevel.DEBUG, f"Updating properties (full_refresh={full_refresh})")
        result = lib.AwControl_UpdateProperties(self._ffi, full_refresh)
        return result.code == lib.AW_RESULT_OK

    def get_num_properties(self) -> int:
        """
        Get the number of available camera properties.

        Returns:
            The number of properties.
        """
        return lib.AwControl_NumProperties(self._ffi)

    def get_property_at_index(self, index: int) -> typing.Optional[AwPtpProperty]:
        """
        Get a property by its index.

        Use with `get_num_properties()` to list all available properties. The order
        is stable between refreshes.

        Args:
            index: The property index.

        Returns:
            The AwPtpProperty object, or None if the index is out of range.
        """
        prop = lib.AwControl_GetPropertyByIndex(self._ffi, index)
        if prop == ffi.NULL:
            return None
        return AwPtpProperty(self, self._ffi, self._allocator, prop)

    def get_property(self, locater: typing.Union[str, AwPropertyCode, int]) -> typing.Optional[AwPtpProperty]:
        if isinstance(locater, str):
            prop = self.get_property_by_id(locater)
            if prop is not None:
                return prop
            try:
                prop_code = AwPropertyCode[locater.upper()]
                return self.get_property_by_code(prop_code)
            except KeyError:
                return None
        if isinstance(locater, (AwPropertyCode, int)):
            return self.get_property_by_code(locater)
        return None

    def get_property_by_code(self, code: int) -> typing.Optional[AwPtpProperty]:
        """
        Get a property by its PTP code.

        Args:
            code: The PTP property code.

        Returns:
            The AwPtpProperty object, or None if not found.
        """
        prop = lib.AwControl_GetPropertyByCode(self._ffi, code)
        if prop == ffi.NULL:
            return None
        return AwPtpProperty(self, self._ffi, self._allocator, prop)

    def get_property_by_id(self, prop_id: str) -> typing.Optional[AwPtpProperty]:
        """
        Get a property by its string ID (e.g., 'shutter-speed', 'iso').

        Property IDs are specific to AlphaWire and provide a stable way to refer to
        settings across different camera models that might use different PTP codes.

        Args:
            prop_id: The property ID string.

        Returns:
            The AwPtpProperty object, or None if not found.
        """
        c_id = ffi.new("char[]", prop_id.encode("utf-8"))
        prop = lib.AwControl_GetPropertyById(self._ffi, c_id)
        if prop == ffi.NULL:
            return None
        return AwPtpProperty(self, self._ffi, self._allocator, prop)

    def get_num_controls(self) -> int:
        """
        Get the number of available camera controls.

        Returns:
            The number of controls.
        """
        return lib.AwControl_NumControls(self._ffi)

    def get_control_at_index(self, index: int) -> typing.Optional[AwPtpControl]:
        """
        Get a control by its index.

        Args:
            index: The control index.

        Returns:
            The AwPtpControl object, or None if the index is out of range.
        """
        control = lib.AwControl_GetControlByIndex(self._ffi, index)
        if control == ffi.NULL:
            return None
        return AwPtpControl(self, self._ffi, self._allocator, control)

    def get_control_by_code(self, control_code: int) -> typing.Optional[AwPtpControl]:
        """
        Get a control by its PTP code.

        Args:
            control_code: The PTP control code.

        Returns:
            The AwPtpControl object, or None if not found.
        """
        control = lib.AwControl_GetControlByCode(self._ffi, control_code)
        if control == ffi.NULL:
            return None
        return AwPtpControl(self, self._ffi, self._allocator, control)

    def set_control_toggle(self, control_code: int, pressed: bool) -> typing.Any:
        """
        Toggle a control (press or release).

        Args:
            control_code: The PTP control code.
            pressed: True to press, False to release.

        Returns:
            An AwResult.
        """
        return lib.AwControl_SetControlToggle(self._ffi, control_code, pressed)

    def set_property_str(self, ffi_property: typing.Any, value: str) -> typing.Any:
        c_val = ffi.new("char[]", value.encode("utf-8"))
        m_str = ffi.new("MStr[1]")
        m_str[0].str = c_val
        m_str[0].size = len(value)
        m_str[0].capacity = 0
        return lib.AwControl_SetPropertyStr(self._ffi, ffi_property, m_str[0])

    def get_live_view_image(self) -> typing.Optional[memoryview]:
        """
        Retrieve the current live view frame from the camera.

        Returns:
            A memoryview containing the image data (usually JPEG), or None if unavailable.
            You may need to retry this call immediately after connecting.
        """
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
        """
        Get the number of image files queued for download on the camera.

        Returns:
            The number of pending files.
        """
        lib.AwControl_UpdateProperties(self._ffi, True)
        return lib.AwControl_GetPendingFiles(self._ffi)

    def get_captured_image(self) -> typing.Optional[AwCapturedFile]:
        """
        Download the next captured image from the camera's buffer.

        Note: Downloading when no images are available can cause issues (crashes or
        invalid counts) on older pre-2020 cameras.

        Returns:
            An AwCapturedFile object, or None if no image is available.
        """
        mem_io = ffi.new("MMemIO[1]")
        mem_io[0].allocator = self._allocator
        cii = ffi.new("AwPtpCapturedImageInfo[1]")
        result = lib.AwControl_GetCapturedImage(self._ffi, mem_io, cii)
        if result.code == lib.AW_RESULT_OK and mem_io[0].size:
            filename = _convert_m_str(cii[0].filename)
            data = bytes(ffi.buffer(mem_io[0].mem, mem_io[0].size))
            lib.Aw_MemIOFree(mem_io)
            return AwCapturedFile(filename, data)
        lib.Aw_MemIOFree(mem_io)
        return None

    def update_properties(self, full_refresh: bool = False) -> typing.Any:
        log(AwLogLevel.DEBUG, f"Updating properties (full_refresh={full_refresh})")
        return lib.AwControl_UpdateProperties(self._ffi, full_refresh)

    def _wait_for_condition(self, condition_func: typing.Callable[[], typing.Any], 
                            stage: AwCaptureStage, workflow: AwImageCaptureWorkflow, label: str) -> typing.Any:
        total_wait_time, sleep_time = workflow.get_wait_time(stage)
        log(AwLogLevel.TRACE, f"Starting wait for {label} (total_wait={total_wait_time}s)")
        start_wait = time.time()
        while time.time() - start_wait < total_wait_time:
            result = condition_func()
            if result:
                return result
            log(AwLogLevel.TRACE, f"Waiting for {label}... ({time.time() - start_wait:.2f}s)")
            workflow.wait(sleep_time, stage)

        log(AwLogLevel.WARNING, f"Timeout waiting for {label} after {total_wait_time}s")
        return None

    def capture_image(self, trigger_duration= 0.1,
                      workflow: typing.Optional[AwImageCaptureWorkflow] = None) -> bool:
        """
        Trigger an image capture on the camera.

        Args:
            trigger_duration: How long to hold the shutter button down (seconds).
            workflow: Optional configuration for the capture workflow.

        Returns:
            True if an image was captured and is ready for download, False otherwise.
        """
        if workflow is None:
            workflow = AwImageCaptureWorkflow()

        self.set_control_toggle(AwControlCode.SHUTTER_HALF_PRESS, True)
        
        # wait focus done
        focus_mode = self.get_property_by_id('focus-mode')
        wait_focus = True
        if focus_mode is None:
            wait_focus = False
        else:
            focus_mode_str = focus_mode.get_value()
            wait_focus = (focus_mode_str != "Manual")

        if wait_focus:
            focus_state = self.get_property_by_id('auto-focus-status')
            if focus_state is not None:
                focus_complete_states = {
                    AwAutoFocusStatus.AFC_FOCUSED,
                    AwAutoFocusStatus.AFS_LOCKED,
                    AwAutoFocusStatus.AFS_FAILED,
                    AwAutoFocusStatus.AFC_FAILED
                }

                def check_focus():
                    self.update_properties()
                    val = focus_state.get_value()
                    return val in focus_complete_states

                self._wait_for_condition(check_focus, AwCaptureStage.WAIT_FOCUS, workflow, "focus")

        self.set_control_toggle(AwControlCode.SHUTTER, True)
        workflow.wait(trigger_duration, AwCaptureStage.TRIGGER)
        self.set_control_toggle(AwControlCode.SHUTTER, False)
        self.set_control_toggle(AwControlCode.SHUTTER_HALF_PRESS, False)

        file_ready = self._wait_for_condition(lambda: self.get_pending_files() > 0, 
                                              AwCaptureStage.WAIT_FILE, workflow, "file")

        return bool(file_ready)

    def download_image(self, workflow: typing.Optional[AwImageCaptureWorkflow] = None) -> typing.Optional[AwCapturedFile]:
        """
        Wait for and download a captured image.

        Args:
            workflow: Optional configuration for the capture workflow.

        Returns:
            The downloaded AwCapturedFile, or None on timeout.
        """
        if workflow is None:
            workflow = AwImageCaptureWorkflow()

        return self._wait_for_condition(self.get_captured_image, 
                                        AwCaptureStage.WAIT_DOWNLOAD, workflow, "download")

    def capture_and_download(self, trigger_duration=.1,
                             workflow: typing.Optional[AwImageCaptureWorkflow] = None) -> typing.Optional[AwCapturedFile]:
        """
        Trigger an image capture and download the result.

        Args:
            trigger_duration: How long to hold the shutter button down (seconds).
            workflow: Optional configuration for the capture workflow.

        Returns:
            The downloaded AwCapturedFile, or None on error or timeout.
        """
        if workflow is None:
            workflow = AwImageCaptureWorkflow()

        if not self.capture_image(trigger_duration, workflow):
            return None

        return self.download_image(workflow)

    def __del__(self) -> None:
        self.close()

class AwDevice:
    """Represents a discovered camera device."""
    def __init__(self, ffi_device: typing.Any, device_list: 'AwDeviceList') -> None:
        self._ffi_device = ffi_device
        self._device_list: 'AwDeviceList' = device_list
        self._allocator = ffi.new("MAllocator[1]")
        lib.Aw_InitDefaultAllocator(self._allocator)
        self._control: typing.Optional[AwControl] = None

    def open_control(self) -> AwControl:
        """
        Open the control interface for this device.

        Returns:
            An AwControl object.
        """
        if self._control is None:
            self._control = AwControl(self._ffi_device, self._allocator, self)
        return self._control

    def close(self) -> None:
        """Close the device and release resources."""
        if self._control is not None:
            self._control.close()
            self._control = None
        if self._ffi_device is not None and self._device_list is not None:
            if self._ffi_device is not None:
                lib.AwDeviceList_CloseDevice(self._device_list._devlist, self._ffi_device)
                if self in self._device_list._devices:
                    self._device_list._devices.remove(self)
            self._ffi_device = None
            self._device_list = None

    def __del__(self) -> None:
        self.close()

class AwDeviceList:
    """
    Manager for discovering and listing Sony camera devices.

    This class serves as the central data model for managing connected PTP devices
    across various backends (LibUSBK, LibUSB, WIA, and TCP/IP). It tracks which
    devices are available and which are currently open.
    """
    def __init__(self) -> None:
        self._devlist = ffi.new("AwDeviceList[1]")
        self._allocator = ffi.new("MAllocator[1]")
        lib.Aw_InitDefaultAllocator(self._allocator)
        self._is_open: bool = False
        self._iter_index: int = 0
        self._devices: typing.List[AwDevice] = []
        self._devlist[0].logger.logFunc = _aw_log_callback
        self._devlist[0].logger.level = _aw_log_level.value
        _aw_log_instances.append(weakref.ref(self))

    def open(self) -> bool:
        """
        Open the device list manager and initialize available backends.

        Returns:
            True if at least one backend was initialized, False otherwise.
        """
        if self._is_open:
            self.close()
        ok = lib.AwDeviceList_Open(self._devlist, self._allocator)
        if ok:
            self._is_open = True
        return bool(ok)

    def close(self) -> bool:
        """
        Close the device list manager and release all associated resources.

        This will also close all devices that were opened through this list.

        Returns:
            True if successful, False otherwise.
        """
        if not self._is_open:
            return False
        for device in list(self._devices):
            device.close()
        self._devices.clear()
        ok = lib.AwDeviceList_Close(self._devlist)
        if ok:
            self._is_open = False
        return bool(ok)

    def refresh(self) -> bool:
        """
        Trigger a refresh of the device list by polling all backends.

        This updates the list of available devices. While usually blocking,
        discovery on the IP backend may take time and require subsequent polling.

        Returns:
            True if successful, False otherwise.
        """
        if not self._is_open:
            return False
        return bool(lib.AwDeviceList_RefreshList(self._devlist))

    def needs_refresh(self) -> bool:
        """
        Quick check if the device list needs a refresh without performing one.

        Checks backends for device addition or removal flags.

        Returns:
            True if a refresh is needed, False otherwise.
        """
        if not self._is_open:
            return False
        return bool(lib.AwDeviceList_NeedsRefresh(self._devlist))

    def is_refreshing(self) -> bool:
        """
        Check if the device list is currently being refreshed in the background.

        If True, you should call `poll_updates()` to receive new devices.

        Returns:
            True if refreshing, False otherwise.
        """
        if not self._is_open:
            return False
        return bool(lib.AwDeviceList_IsRefreshingList(self._devlist))

    def poll_updates(self) -> bool:
        """
        Poll for device list updates (non-blocking).

        Should be called if `is_refreshing()` is True.

        Returns:
            True if a device was added, False otherwise.
        """
        if not self._is_open:
            return False
        return bool(lib.AwDeviceList_PollUpdates(self._devlist))

    def open_device(self, device_info: AwDeviceInfo) -> typing.Optional[AwDevice]:
        """
        Open a device from the list.

        Args:
            device_info: The info of the device to open.

        Returns:
            An AwDevice object, or None if it could not be opened.
        """
        if not self._is_open:
            return None
        device_out = ffi.new("AwDevice**")
        ffi_device_info = ffi.addressof(device_info._ffi_device)
        result = lib.AwDeviceList_OpenDevice(self._devlist, ffi_device_info, device_out)
        if result.code == lib.AW_RESULT_OK:
            device = AwDevice(device_out[0], self)
            self._devices.append(device)
            return device
        return None

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

    def __iter__(self) -> 'AwDeviceList':
        self._iter_index = 0
        return self

    def __next__(self) -> AwDeviceInfo:
        if self._iter_index >= len(self):
            raise StopIteration
        device = self[self._iter_index]
        self._iter_index += 1
        return device

    def __del__(self) -> None:
        if self._is_open:
            self.close()
