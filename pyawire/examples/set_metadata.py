import datetime
import time
import argparse
import awire
from awire.main import AwPropertyCode


# On cameras that support it, set the following:
# - Sync current time from PC
# - Photographer
# - Copyright

def set_metadata(photographer_str: str, copyright_str: str):
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

            photographer = control.get_property(AwPropertyCode.PHOTOGRAPHER)
            if photographer is not None:
                awire.log_info(f"Setting photographer to '{photographer_str}'")
                photographer.set_value(photographer_str)

            copyright = control.get_property(AwPropertyCode.COPYRIGHT)
            if copyright is not None:
                awire.log_info(f"Setting copyright to '{copyright_str}'")
                copyright.set_value(copyright_str)

            date_time_set = control.get_property(AwPropertyCode.DATE_TIME_SET)
            if date_time_set is not None:
                now = datetime.datetime.now()
                
                # Get timezone offset in minutes
                is_dst = time.localtime().tm_isdst > 0
                offset_seconds = -time.altzone if is_dst else -time.timezone
                offset_minutes = offset_seconds // 60
                
                offset_hours = int(offset_minutes / 60)
                offset_mins = abs(int(offset_minutes % 60))
                
                # deciSec (1/10th of a second)
                deci_sec = int(now.microsecond / 100000)
                
                # Format: YYYYmmddTHHMMSS.m+HHMM
                time_str = now.strftime("%Y%m%dT%H%M%S") + f".{deci_sec}{offset_hours:+03d}{offset_mins:02d}"

                awire.log_info(f"Setting time to '{time_str}'")
                date_time_set.set_value(time_str)

            if date_time_set is None and copyright is None and copyright is None:
                awire.log_warn(f"No metadata settable on this camera model")

            device.close()

    device_list.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Set camera metadata")
    parser.add_argument("--photographer", type=str, default="awire", help="Photographer name")
    parser.add_argument("--copyright", type=str, default="awire", help="Copyright information")
    args = parser.parse_args()

    awire.log_set_level(awire.AwLogLevel.INFO)
    
    set_metadata(args.photographer, args.copyright)
