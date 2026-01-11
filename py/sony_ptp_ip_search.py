import socket
import struct
import urllib.request
import xml.etree.ElementTree as ET
from urllib.parse import urljoin, urlparse


def parse_ssdp_response(response):
    headers = {}
    for line in response.split("\r\n"):
        if ":" in line:
            k, v = line.split(":", 1)
            headers[k.strip().lower()] = v.strip()
    return headers

def fetch_device_description(location):
    try:
        with urllib.request.urlopen(location, timeout=5) as resp:
            return resp.read()
    except Exception as e:
        print(f"Failed to fetch description: {e}")
        return None

def fetch_digital_imaging_desc(location):
    try:
        with urllib.request.urlopen(location, timeout=5) as resp:
            return resp.read()
    except Exception as e:
        print(f"Failed to fetch description: {e}")
        return None

def parse_device_info(xml_bytes):
    ns = {"upnp": "urn:schemas-upnp-org:device-1-0"}
    root = ET.fromstring(xml_bytes)

    device = root.find("upnp:device", ns)
    if device is None:
        return {}

    # Parse all SCPDURL from serviceList
    digitalImagingUrl = None
    service_list = device.find("upnp:serviceList", ns)
    if service_list is not None:
        for service in service_list.findall("upnp:service", ns):
            if service.findtext("upnp:serviceId", default="", namespaces=ns) == 'urn:schemas-sony-com:serviceId:DigitalImaging':
                scpd_url = service.findtext("upnp:SCPDURL", default="", namespaces=ns)
                digitalImagingUrl = scpd_url

    return {
        "friendlyName": device.findtext("upnp:friendlyName", default="", namespaces=ns),
        "manufacturer": device.findtext("upnp:manufacturer", default="", namespaces=ns),
        "modelName": device.findtext("upnp:modelName", default="", namespaces=ns),
        "deviceType": device.findtext("upnp:deviceType", default="", namespaces=ns),
        "digitalImagingUrl": digitalImagingUrl,
        "uuid": device.findtext("upnp:UDN", default="", namespaces=ns),
    }

def parse_digital_imaging_info(xml_bytes):
    ns = {"upnp": "urn:schemas-upnp-org:service-1-0"}
    root = ET.fromstring(xml_bytes)

    device = root.find("upnp:X_DigitalImagingDeviceInfo", ns)
    if device is None:
        return {}

    di = {}
    device_info = device.find("upnp:X_DeviceInfo", ns)
    if device_info is not None:
        di["ModelName"] = device_info.findtext("upnp:X_ModelName", default="", namespaces=ns)
        di["FirmwareVersion"] = device_info.findtext("upnp:X_FirmwareVersion", default="", namespaces=ns)
        di["Category"] = device_info.findtext("upnp:X_Category", default="", namespaces=ns)
        di["MacAddress"] = device_info.findtext("upnp:X_MacAddress", default="", namespaces=ns)

    connection_info = device.find("upnp:X_ConnectionInfo", ns)
    if connection_info is not None:
        di["SSH_Support"] = connection_info.findtext("upnp:X_SSH_Support", default="", namespaces=ns)

    ptp_info = device.find("upnp:X_PTP_Information", ns)
    if ptp_info is not None:
        di["PTP_Versions"] = ptp_info.findtext("upnp:X_PTP_Versions", default="", namespaces=ns)
        di["PTP_PairingNecessity"] = ptp_info.findtext("upnp:X_PTP_PairingNecessity", default="", namespaces=ns)

    return di


def check_tcp_connection(host, port, timeout=2):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        result = sock.connect_ex((host, port))
        sock.close()
        return result == 0
    except Exception as e:
        print(f"TCP connection check failed for {host}:{port} - {e}")
        return False


def send_ptp_ip_init_command(host, port=15740, timeout=20):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        sock.connect((host, port))

        # PTP IP Init Command Request packet
        # Length: 4 bytes, Packet Type: 4 bytes (0x00000001 for Init Command Request)
        # GUID: 16 bytes, Friendly Name: variable length (null-terminated UTF-16LE)
        # Protocol Version: 4 bytes

        guid = b'\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f'
        friendly_name = "AlphaWire".encode('utf-16le') + b'\x00\x00'
        protocol_version = 0x00010000

        packet_type = 0x00000001
        payload = guid + friendly_name + struct.pack('<I', protocol_version)
        packet_length = 8 + len(payload)

        packet = struct.pack('<I', packet_length) + struct.pack('<I', packet_type) + payload

        print(f"Sending PTP IP Init Command Request to {host}:{port}...")
        sock.sendall(packet)

        # Wait for response
        response = sock.recv(4096)
        if len(response) >= 8:
            print("Got response:")
            # Parse response
            resp_packet_type = struct.unpack('<I', response[4:8])[0]
            if resp_packet_type == 0x00000002:  # Init Command Ack
                if len(response) >= 32:
                    resp_length = struct.unpack('<I', response[0:4])[0]
                    connection_id = struct.unpack('<I', response[8:12])[0]
                    camera_guid = response[12:28]

                    # Extract friendly name (UTF-16LE null-terminated)
                    friendly_name_bytes = response[28:-4]
                    friendly_name_end = -1
                    # Finds UTF‑16LE null terminator in friendly name bytes
                    for i in range(0, len(friendly_name_bytes) - 1, 2):
                        if friendly_name_bytes[i] == 0 and friendly_name_bytes[i+1] == 0:
                            friendly_name_end = i
                    if friendly_name_end != -1:
                        friendly_name = friendly_name_bytes[:friendly_name_end].decode('utf-16le', errors='ignore')
                    else:
                        friendly_name = friendly_name_bytes.decode('utf-16le', errors='ignore')

                    protocol_version = struct.unpack('<I', response[-4:])[0]

                    print(f"  Connection ID: 0x{connection_id:08x}")
                    print(f"  Camera GUID: {camera_guid.hex()}")
                    print(f"  Camera Friendly Name: {friendly_name}")
                    print(f"  Protocol Version: 0x{protocol_version:08x}")
                else:
                    print("Response too short to parse Init Command Ack")
            else:
                print(f"Unexpected packet type: 0x{resp_packet_type:08x}")
        else:
            print("Response too short to parse header")

        sock.close()
        return True
    except Exception as e:
        print(f"Failed to send PTP IP Init Command: {e}")
        return False


def discover_sony_cameras_ptp_ip(timeout=10):
    # SSDP M-SEARCH message
    msg = (
        'M-SEARCH * HTTP/1.1\r\n'
        'HOST:239.255.255.250:1900\r\n'
        'MAN:"ssdp:discover"\r\n'
        'ST:ssdp:all\r\n'
        'MX:2\r\n'
        '\r\n'
    )

    # Set up UDP socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
    s.settimeout(timeout)

    # Get all local IPv4 addresses
    try:
        hostname = socket.gethostname()
        local_ips = [addr[4][0] for addr in socket.getaddrinfo(hostname, None, socket.AF_INET)]
    except Exception as e:
        print(f"Could not determine local IPs: {e}")
        local_ips = ["0.0.0.0"]

    # Send M-SEARCH on all local interfaces
    for ip in local_ips:
        try:
            print(f"Sending M-SEARCH on interface: {ip}")
            # Tell the socket to use this specific interface for multicast
            s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(ip))
            s.sendto(msg.encode('utf-8'), ('239.255.255.250', 1900))
        except OSError as e:
            print(f"Failed to send on {ip}: {e}")

    print("Searching for UPnP devices...")
    try:
        while True:
            data, addr = s.recvfrom(65507)

            r = data.decode('utf-8')
            headers = parse_ssdp_response(r)
            device_desc_url = None
            usn: str|None = None
            for header_name, content in headers.items():
                if header_name.upper().startswith('LOCATION'):
                    device_desc_url = content.strip()
                elif header_name.upper().startswith('USN'):
                    usn = content.strip()

            if usn is not None and device_desc_url is not None:
                if usn.find(':urn:schemas-sony-com:service:DigitalImaging') >= 0:
                    parsed_url = urlparse(device_desc_url)
                    root_url = f"{parsed_url.scheme}://{parsed_url.netloc}"
                    host = parsed_url.hostname
                    print(f"Found Sony Imaging device at {host}")
                    print(f"Fetching device description from {device_desc_url}...")
                    dd_xml = fetch_device_description(device_desc_url)
                    dd = parse_device_info(dd_xml)
                    print(dd)
                    digital_image_desc_url = urljoin(root_url, dd['digitalImagingUrl'])
                    print(f"Fetching digital imaging desc description from {digital_image_desc_url}...")
                    did_xml = fetch_digital_imaging_desc(digital_image_desc_url)
                    did = parse_digital_imaging_info(did_xml)
                    print(did)
                    if host and check_tcp_connection(host, 15740):
                        print(f"Device at {host} accepts TCP connections on port 15740")
                        send_ptp_ip_init_command(host)
                    else:
                        print(f"Device at {host} does not accept TCP connections on port 15740")

    except socket.timeout:
        pass


if __name__ == "__main__":
    discover_sony_cameras_ptp_ip()

