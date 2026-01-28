#include "platform/ip/ptp-backend-ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mlib/msock.h"
#include "mlib/mxml.h"
#include "platform/ip/http-client.h"
#include "platform/usb-const.h"
#include "ptp/ptp-device-list.h"

typedef enum PTPIpPacketTypes {
    PTPIP_TYPE_INIT_COMMAND_REQUEST = 0x1,
    PTPIP_TYPE_INIT_COMMAND_ACK = 0x2,
    PTPIP_TYPE_INIT_EVENT_REQUEST = 0x3,
    PTPIP_TYPE_INIT_EVENT_ACK = 0x4,
    PTPIP_TYPE_INIT_FAIL = 0x5,
    PTPIP_TYPE_CMD_REQUEST = 0x6,
    PTPIP_TYPE_CMD_RESPONSE = 0x7,
    PTPIP_TYPE_EVENT = 0x8,
    PTPIP_TYPE_DATA_PACKET_START = 0x9,
    PTPIP_TYPE_DATA_PACKET = 0xa,
    PTPIP_TYPE_DATA_PACKET_CANCEL = 0xb,
    PTPIP_TYPE_DATA_PACKET_END = 0xc,
    PTPIP_TYPE_PROBE_PACKET_REQ = 0xd,
    PTPIP_TYPE_PROBE_PACKET_RES = 0xe
} PTPIpPacketTypes;

typedef enum PTPIpInitFailError {
    PTPIP_FAIL_REJECTED_INITIATOR = 0x00000001,
    PTPIP_FAIL_BUSY = 0x00000002,
    PTPIP_FAIL_UNSPECIFIED = 0x00000003,
    PTPIP_FAIL_INVALID_GUID = 0x00000004,
} PTPIpInitFailError;

// SSDP Multicast IP address is 239.255.255.250
#define SSDP_MULTICAST_ADDR 0xEFFFFFFA

static const char * PTPIp_GetInitFailErrorString(u32 failureCode) {
    switch (failureCode) {
        case PTPIP_FAIL_REJECTED_INITIATOR:
            return "Connection rejected for client (often due to GUID/name mismatch).";
        case PTPIP_FAIL_BUSY:
            return "Currently busy and cannot accept new connections (e.g. maximum sessions reached).";
        case PTPIP_FAIL_UNSPECIFIED:
            return "A general error occurred during init phase";
        case PTPIP_FAIL_INVALID_GUID:
            return "The GUID provided is invalid or not recognized";
        default:
            return "Unknown failure code";
    }
}

typedef struct {
    MSock dataSock;
    MSock eventSock;
    u32 sessionId;
    u32 transactionId;
} PTPIpDevice;

typedef struct {
    PTPDeviceInfo* deviceList;
    PTPIpDevice* openDevices;
    MSock discoverySock;
    u64 discoveryStartTime;
    b32 isDiscoveryInProgress;
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    PTPLog logger;
} PTPIpBackend;

static void PTPIp_ReleaseList(PTPIpBackend* backend) {

}

static b32 PTPIp_NeedsRefresh(PTPIpBackend* backend) {
    return FALSE;
}

static b32 PTPIp_Close(PTPIpBackend* backend) {
    PTP_TRACE("PTPIp_Close");
    PTPIp_ReleaseList(backend);
    if (backend->openDevices) {
        MArrayFree(backend->allocator, backend->openDevices);
    }
    MSockDeinit();
    return TRUE;
}

void* PTPDeviceIp_ReallocBuffer(void* deviceSelf, PTPBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize) {
    PTPDevice* self = (PTPDevice*)deviceSelf;
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataNewSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataOldSize + headerSize); dataMem = NULL;
    }
    dataMem = MMallocZ(self->transport.allocator, dataSize);
    return ((u8*)dataMem) + headerSize;
}

void PTPDeviceIp_FreeBuffer(void* deviceSelf, PTPBufferType type, void* dataMem, size_t dataOldSize) {
    PTPDevice* self = (PTPDevice*)deviceSelf;
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataOldSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataSize); dataMem = NULL;
    }
}

static int TcpReadBytesIntoBuffer(SOCKET socket, MMemIO* in, MMemIO* inRead, u32 bytesToRead) {
    i32 bytesRead = in->size - inRead->size;
    while (bytesToRead > bytesRead) {
        i32 memSize = in->capacity - in->size;
        if (memSize < 1024) {
            MMemGrowBytes(in, 1024 - memSize);
            inRead->mem = in->mem;
            inRead->capacity = in->capacity;
            memSize = in->capacity - in->size;
        }
        char* mem = (char*)(in->mem + in->size);
        int r = recv(socket, mem, memSize, 0);
        if (r == -1 || r == 0) {
            return r;
        }
        bytesRead += r;
        in->size += r;
    }
    return bytesRead;
}

static int TcpSendAllBytes(SOCKET socket, const void* data, size_t dataSize) {
    int totalSent = 0;
    while (totalSent < dataSize) {
        int s = send(socket, (char *) data + totalSent, (int) (dataSize - totalSent), 0);
        if (s == SOCKET_ERROR || s == 0) {
            return s;
        }
        totalSent += s;
    }
    return totalSent;
}

// PTP IP example packets:
//
// Send1: SDIO_Connect request
// 1e 00 00 00 06 00 00 00 01 00 00 00 01 92 04 00 | 00 00 02 00 00 00 00 00 00 00 00 00 00 00
// <len      > <cmd req  > <dataphase> <op > <tid  |     > <param1   > <param2   > <param3   >
//
// Recv1: SDIO_Connect response1
// 14 00 00 00 09 00 00 00 04 00 00 00 08 00 00 00 | 00 00 00 00
// <len      > <datastart> <tid      > <data len64 |           >
//
// Recv2: SDIO_Connect response2
// 14 00 00 00 0a 00 00 00 04 00 00 00 00 00 00 00 | 00 00 00 00 0c 00 00 00 0c 00 00 00 04 00 00 00 | 0e 00 00 00 07 00 00 00 01 20 04 00 00 00
// <len      > <data more> <tid      > <data       |           > <len      > <data end > <tid      > | <len      > <cmd   res> <res> <tid      >
//

static PTPResult PTPDeviceIp_SendAndRecvEx(void* deviceSelf, PTPRequestHeader* request, u8* dataIn, size_t dataInSize,
                                           PTPResponseHeader* response, u8* dataOut, size_t dataOutSize,
                                           size_t* actualDataOutSize) {
    PTPDevice* self = (PTPDevice*)deviceSelf;
    PTPIpDevice* dev = (PTPIpDevice*)self->device;
    MAllocator* allocator = self->transport.allocator;
    PTPResult error = PTP_OK;

    // 1. Send request packet
    u32 packetLen = 4 + 4 + 4 + 2 + 4 + (request->NumParams * 4);
    MMemIO out;
    MMemInitEmpty(&out, allocator);
    MMemWriteU32LE(&out, packetLen);
    MMemWriteU32LE(&out, PTPIP_TYPE_CMD_REQUEST);
    MMemWriteU32LE(&out, dataInSize == 0 ? 1 : 2);
    MMemWriteU16LE(&out, request->OpCode);
    MMemWriteU32LE(&out, request->TransactionId);
    for (int i = 0; i < request->NumParams; ++i) {
        MMemWriteU32LE(&out, request->Params[i]);
    }
    int s = TcpSendAllBytes(dev->dataSock, out.mem, out.size);
    if (s == SOCKET_ERROR) {
        error = PTP_AW_TIMEOUT;
        goto exitWithError;
    } else if (s == 0) {
        error = PTP_AW_CONNECTION_CLOSED;
        goto exitWithError;
    }

    // 2. Send data packets if needed
    if (dataInSize > 0) {
        // Send data packet start, data / end
        MMemReset(&out);
        MMemWriteU32LE(&out, 4 + 4 + 4 + 8);
        MMemWriteU32LE(&out, PTPIP_TYPE_DATA_PACKET_START);
        MMemWriteU32LE(&out, request->TransactionId);
        MMemWriteU64LE(&out, dataInSize);
        MMemWriteU32LE(&out, 4 + 4 + 4 + dataInSize);
        MMemWriteU32LE(&out, PTPIP_TYPE_DATA_PACKET);
        MMemWriteU32LE(&out, request->TransactionId);
        MMemWriteU8CopyN(&out, dataIn, (u32)dataInSize);
        MMemWriteU32LE(&out, 4 + 4 + 4);
        MMemWriteU32LE(&out, PTPIP_TYPE_DATA_PACKET_END);
        MMemWriteU32LE(&out, request->TransactionId);
        s = TcpSendAllBytes(dev->dataSock, out.mem, out.size);
        if (s == SOCKET_ERROR) {
            goto exitWithError;
        } else if (s == 0) {
            goto exitWithError;
        }
    }

    MMemFree(&out);

    // 3. Receive Response(s)
    MMemIO in;
    MMemInitAlloc(&in, allocator, 1024);

    MMemIO inRead = {in.mem, 0, in.capacity};
    u64 transferLen = 0;
    u8* dataOutCurrent = dataOut;
    i64 dataRemaining = (i64)dataOutSize;

    while (TRUE) {
        int r = TcpReadBytesIntoBuffer(dev->dataSock, &in, &inRead, 8);
        if (r == SOCKET_ERROR) {
            error = PTP_AW_TIMEOUT;
            goto exitWithError;
        } else if (r == 0) {
            error = PTP_AW_CONNECTION_CLOSED;
            goto exitWithError;
        }

        u32 responseLen, responseType;
        MMemReadU32LE(&inRead, &responseLen);
        MMemReadU32LE(&inRead, &responseType);
        if (responseLen < 8) {
            error = PTP_AW_MALFORMED_RESPONSE;
            goto exitWithError;
        }

        u32 payloadLen = responseLen - 8;
        r = TcpReadBytesIntoBuffer(dev->dataSock, &in, &inRead, payloadLen);
        inRead.mem = in.mem;
        if (r == SOCKET_ERROR) {
            error = PTP_AW_TIMEOUT;
            goto exitWithError;
        } else if (r == 0) {
            error = PTP_AW_CONNECTION_CLOSED;
            goto exitWithError;
        }

        i32 bytesReadAfterPacket = in.size - responseLen;  // could be partial PTP packet
        if (responseType == PTPIP_TYPE_CMD_RESPONSE) {
            // Read u16 OpCode and u32 transaction id
            if (payloadLen >= 6) {
                u16 responseCode;
                u32 transactionId;
                MMemReadU16LE(&inRead, &responseCode);
                MMemReadU32LE(&inRead, &transactionId);
                response->ResponseCode = responseCode;
                // Read any params as well
                if (payloadLen > 6) {
                    u32 numParams = (payloadLen - 6) / 4;
                    for (int i = 0; i < numParams; ++i) {
                        u32 param = 0;
                        MMemReadU32LE(&inRead, &param);
                        response->Params[i] = param;
                    }
                }
            } else {
                error = PTP_AW_MALFORMED_RESPONSE;
                goto exitWithError;
            }
            break;
        } else if (responseType == PTPIP_TYPE_DATA_PACKET_START) {
            if (payloadLen >= 12) {
                u32 transactionId;
                MMemReadU32LE(&inRead, &transactionId);
                MMemReadU64LE(&inRead, &transferLen);

                if (transferLen > (u64)dataOutSize) {
                    PTP_WARNING_F("Response data size: %llu but buffer out only: %llu", transferLen, (u64)dataOutSize);
                }
            } else {
                error = PTP_AW_MALFORMED_RESPONSE;
                goto exitWithError;
            }
        } else if (responseType == PTPIP_TYPE_DATA_PACKET) {
            if (payloadLen >= 4) {
                u32 transactionId;
                MMemReadU32LE(&inRead, &transactionId);
                payloadLen -= 4;

                if (payloadLen > 0) {
                    if (dataRemaining >= payloadLen) {
                        MMemReadU8CopyN(&inRead, (u8*)dataOutCurrent, payloadLen);
                        dataRemaining -= payloadLen;
                        dataOutCurrent += payloadLen;
                    }
                }
            } else {
                error = PTP_AW_MALFORMED_RESPONSE;
                goto exitWithError;
            }
        } else if (responseType == PTPIP_TYPE_DATA_PACKET_END) {
            if (payloadLen == 4) {
                u32 transactionId;
                MMemReadU32LE(&inRead, &transactionId);
            } else {
                error = PTP_AW_MALFORMED_RESPONSE;
                goto exitWithError;
            }
        } else {
            error = PTP_AW_MALFORMED_RESPONSE;
            goto exitWithError;
        }

        // Copy partial packet
        if (bytesReadAfterPacket > 0) {
            memcpy(in.mem, (u8*)in.mem + responseLen, bytesReadAfterPacket);
            in.size = bytesReadAfterPacket;
            inRead.size = 0;
        } else if (in.size == inRead.size) {
            in.size = 0;
            inRead.size = 0;
        }
    }

    *actualDataOutSize = dataOutCurrent - dataOut;
    MMemFree(&in);
    return response->ResponseCode;

exitWithError:
    MMemFree(&out);
    MMemFree(&in);
    return error;
}

static b32 PTPIp_OpenDevice(PTPIpBackend* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTP_TRACE("PTPIp_OpenDevice");
    MMemIO out;
    PTPResult error = PTP_OK;

    PTPIpDevice dev = {};
    dev.eventSock = INVALID_SOCKET;
    dev.dataSock = MSockMakeTcpSocket();

    MSockSetSocketTimeout(dev.dataSock, 60000);
    MSockAddress socketAddr = {};
    if (MSockConnectHost(dev.dataSock, MStrViewFromStr(deviceInfo->ipAddress), 15740, &socketAddr) == MSOCK_ERROR) {
        goto exitWithError;
    }

    // Init Command Request (Type 1)
    u8 guid[16] = {'A', 'l', 'p', 'h', 'a', 'W', 'i', 'r', 'e', ' ', 'P', 'T', 'P', ' ', 'I', 'P'};
    const char* friendlyName = "AlphaWire";
    u32 nameLen = MCStrLen(friendlyName);
    u32 initCmdPacketLen = 8 + 16 + (nameLen + 1) * 2 + 4;
    
    MMemInitEmpty(&out, self->allocator);
    MMemWriteU32LE(&out, initCmdPacketLen);
    MMemWriteU32LE(&out, PTPIP_TYPE_INIT_COMMAND_REQUEST);
    MMemWriteU8CopyN(&out, guid, 16);
    
    // Friendly name as UTF-16LE
    for (u32 i = 0; i < nameLen; i++) {
        MMemWriteU16LE(&out, (u16)friendlyName[i]);
    }
    MMemWriteU16LE(&out, 0); // Null terminator
    MMemWriteU32LE(&out, 0x00010000); // Protocol version
    
    int s = TcpSendAllBytes(dev.dataSock, out.mem, out.size);
    if (s == SOCKET_ERROR) {
        error = PTP_AW_TIMEOUT;
        goto exitWithError;
    } else if (s == 0) {
        error = PTP_AW_CONNECTION_CLOSED;
        goto exitWithError;
    }

    MMemFree(&out);

    MMemIO in;
    MMemInitAlloc(&in, self->allocator, 1024);
    MMemIO inRead = {in.mem, 0, in.capacity};

    // Wait for Init Command Ack
    int r = TcpReadBytesIntoBuffer(dev.dataSock, &in, &inRead, 8);
    if (r == SOCKET_ERROR) {
        error = PTP_AW_TIMEOUT;
        goto exitWithError;
    } else if (r == 0) {
        error = PTP_AW_CONNECTION_CLOSED;
        goto exitWithError;
    }

    u32 responseLen, responseType;
    MMemReadU32LE(&inRead, &responseLen);
    MMemReadU32LE(&inRead, &responseType);
    if (responseLen < 8) {
        error = PTP_AW_MALFORMED_RESPONSE;
        goto exitWithError;
    }

    u32 payloadLen = responseLen - 8;
    r = TcpReadBytesIntoBuffer(dev.dataSock, &in, &inRead, payloadLen);
    if (r == SOCKET_ERROR) {
        error = PTP_AW_TIMEOUT;
        goto exitWithError;
    } else if (r == 0) {
        error = PTP_AW_CONNECTION_CLOSED;
        goto exitWithError;
    }

    if (responseType != PTPIP_TYPE_INIT_COMMAND_ACK) {
        if (responseType == PTPIP_TYPE_INIT_FAIL) {
            u32 failureCode = 0;
            MMemReadU32LE(&inRead, &failureCode);
            PTP_ERROR_F("PTPIp_OpenDevice init fail: %s (0x%08x)", PTPIp_GetInitFailErrorString(failureCode),
                failureCode);
            goto exitWithError;
        } else {
            PTP_ERROR_F("PTPIp_OpenDevice unexpected init response packet type: 0x%08x", responseType);
            goto exitWithError;
        }
    }
    
    MMemReadU32LE(&inRead, &dev.sessionId);

    // Setup Event Socket
    dev.eventSock = MSockMakeTcpSocket();
    if (dev.eventSock == INVALID_SOCKET) {
        goto exitWithError;
    }

    MSockSetSocketTimeout(dev.eventSock, 5000);
    if (MSockConnectAddress(dev.eventSock, &socketAddr) == MSOCK_INVALID) {
        goto exitWithError;
    }

    // Init Event Request
    MMemInitEmpty(&out, self->allocator);
    MMemWriteU32LE(&out, 12); // Length: 8 (header) + 4 (sessionId)
    MMemWriteU32LE(&out, PTPIP_TYPE_INIT_EVENT_REQUEST);
    MMemWriteU32LE(&out, dev.sessionId);

    s = TcpSendAllBytes(dev.eventSock, out.mem, out.size);
    if (s == SOCKET_ERROR) {
        error = PTP_AW_TIMEOUT;
        goto exitWithError;
    } else if (s == 0) {
        error = PTP_AW_CONNECTION_CLOSED;
        goto exitWithError;
    }
    MMemFree(&out);

    MMemReset(&in);
    MMemReset(&inRead);

    // Wait for Init Event Ack
    r = TcpReadBytesIntoBuffer(dev.eventSock, &in, &inRead, 8);
    if (r == SOCKET_ERROR) {
        error = PTP_AW_TIMEOUT;
        goto exitWithError;
    } else if (r == 0) {
        error = PTP_AW_CONNECTION_CLOSED;
        goto exitWithError;
    }

    MMemReadU32LE(&inRead, &responseLen);
    MMemReadU32LE(&inRead, &responseType);
    if (responseLen < 8) {
        error = PTP_AW_MALFORMED_RESPONSE;
        goto exitWithError;
    }

    if (responseType != PTPIP_TYPE_INIT_EVENT_ACK) {
        goto exitWithError;
    }

    MMemFree(&in);

    PTPIpDevice* newDev = MArrayAddPtr(self->allocator, self->openDevices);
    *newDev = dev;
    PTPDevice* device = *deviceOut;
    device->backendType = PTP_BACKEND_IP;
    device->device = newDev;
    device->transport.allocator = self->allocator;
    device->transport.sendAndRecvEx = PTPDeviceIp_SendAndRecvEx;
    device->transport.reallocBuffer = PTPDeviceIp_ReallocBuffer;
    device->transport.freeBuffer = PTPDeviceIp_FreeBuffer;
    device->transport.reset = NULL;
    device->transport.requiresSessionOpenClose = TRUE;
    device->logger = self->logger;
    device->disconnected = FALSE;
    return TRUE;

exitWithError:
    MMemFree(&out);
    MMemFree(&in);

    MSockClose(dev.dataSock);
    MSockClose(dev.eventSock);
    return FALSE;
}

static b32 PTPIp_CloseDevice(PTPIpBackend* backend, PTPDevice* device) {
    PTP_TRACE("PTPIp_CloseDevice");
    PTPIpDevice* dev = (PTPIpDevice*)device->device;
    MSockClose(dev->dataSock);
    MSockClose(dev->eventSock);

    MArrayEachPtr(backend->openDevices, it) {
        if (it.p == dev) {
            MArrayRemoveIndex(backend->openDevices, it.i);
            break;
        }
    }
    return TRUE;
}

static b32 PTPIp_RefreshList(PTPIpBackend* self, PTPDeviceInfo** deviceList) {
    PTP_TRACE("PTPIp_RefreshList");

    self->isDiscoveryInProgress = TRUE;
    self->discoveryStartTime = MGetTimeMilliseconds();
    MSockClose(self->discoverySock);

    // SSDP Discovery
    self->discoverySock = MSockMakeUdpSocket();
    int broadcast = 1;
    MSockSetBroadcast(self->discoverySock, broadcast);
    MSockSetNonBlocking(self->discoverySock, TRUE);

    const char* msg = 
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "ST: ssdp:all\r\n"
        "MX: 2\r\n"
        "\r\n";

    int msgSize = (int)MCStrLen(msg);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1900);
    addr.sin_addr.s_addr = htonl(SSDP_MULTICAST_ADDR);

    // Get all local IPv4 addresses and send M-SEARCH on each interface
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct addrinfo hints, *res, *p;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
            for (p = res; p != NULL; p = p->ai_next) {
                struct sockaddr_in* ip = (struct sockaddr_in*)p->ai_addr;

                char ipStr[INET6_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &ip->sin_addr, ipStr, sizeof(ipStr)) == NULL) {
                    ipStr[0] = '\0';
                }

                PTP_INFO_F("Sending M-SEARCH on interface: %s", ipStr);
                
                if (setsockopt(self->discoverySock, IPPROTO_IP, IP_MULTICAST_IF, (char*)&ip->sin_addr,
                    sizeof(ip->sin_addr)) == SOCKET_ERROR) {
                    PTP_ERROR_F("Failed to set IP_MULTICAST_IF for %s: %d", ipStr, MSockGetLastError());
                }

                int sent = sendto(self->discoverySock, msg, msgSize, 0, (struct sockaddr*)&addr, sizeof(addr));
                if (sent == SOCKET_ERROR) {
                    PTP_ERROR_F("PTPIp_RefreshList: sendto failed for %s with error %d", ipStr, MSockGetLastError());
                }
            }
            freeaddrinfo(res);
        } else {
            PTP_ERROR_F("getaddrinfo failed for hostname %s: %d", hostname, MSockGetLastError());
            goto exitWithError;
        }
    } else {
        PTP_ERROR_F("gethostname failed: %d", MSockGetLastError());
        goto exitWithError;
    }

    return TRUE;

exitWithError:
    MSockClose(self->discoverySock);
    self->isDiscoveryInProgress = FALSE;
    return FALSE;
}

static b32 PTPIp_PollListUpdates(PTPIpBackend* self, PTPDeviceInfo** deviceList) {
    b32 foundDevice = FALSE;
    char buffer[4096];
    struct sockaddr_in from;
    int fromLen = sizeof(from);
    int n;

    while ((n = recvfrom(self->discoverySock, buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&from, &fromLen)) > 0) {
        MStrView view = MStrViewMakeP(buffer, n);
        i32 locPos = MStrViewFindC(view, "LOCATION:");
        if (locPos == -1) {
            locPos = MStrViewFindC(view, "location:");
        }

        i32 usnPos = MStrViewFindC(view, "USN:");
        if (usnPos == -1) {
            usnPos = MStrViewFindC(view, "usn:");
        }

        if (locPos != -1 && usnPos != -1) {
            MStrView location = MStrViewSub(view, locPos + 9, -1);
            while (!MStrViewIsEmpty(location) && location.str[0] == ' ') {
                MStrViewAdvance(&location, 1);
            }
            i32 locEndPos = MStrViewFindC(location, "\r\n");

            MStrView usn = MStrViewSub(view, usnPos + 4, -1);
            while (!MStrViewIsEmpty(usn) && usn.str[0] == ' ') {
                MStrViewAdvance(&usn, 1);
            }

            i32 usnEndPos = MStrViewFindC(usn, "\r\n");

            if (locEndPos != -1 && usnEndPos != -1) {
                usn = MStrViewLeft(usn, usnEndPos);
                b32 isSonyImaging = MStrViewFindC(usn, ":urn:schemas-sony-com:service:DigitalImaging") != -1;

                if (isSonyImaging) {
                    MStrView url = MStrViewLeft(location, locEndPos);
                    PTP_INFO_F("Found Sony Imaging device at location: %.*s", url.size, url.str);
                    HttpResponse resp;
                    if (Http_Get(self->allocator, url, &resp)) {
                        if (resp.statusCode == 200) {
                            MXml parser;
                            MXml_Init(&parser, resp.body);

                            MStrView cameraModelTag = MStrViewFromCStr("friendlyName");
                            MStrView manufacturerTag = MStrViewFromCStr("manufacturer");
                            MStrView currentTag = {0};
                            MStr cameraModel = {0};
                            MStr manufacturer = {0};

                            MXmlToken token;
                            while ((token = MXml_NextToken(&parser)).type != MXmlTokenType_EOF && token.type != MXmlTokenType_ERROR) {
                                if (token.type == MXmlTokenType_TAG_START) {
                                    currentTag = token.name;
                                } else if (token.type == MXmlTokenType_TEXT) {
                                    if (MStrViewEq(&cameraModelTag, &currentTag)) {
                                        cameraModel = MStrMakeCopyLen(self->allocator, token.value.str, token.value.size);
                                    } else if (MStrViewEq(&manufacturerTag, &currentTag)) {
                                        manufacturer = MStrMakeCopyLen(self->allocator, token.value.str, token.value.size);
                                    }
                                } else if (token.type == MXmlTokenType_TAG_CLOSE) {
                                    currentTag = (MStrView){0};
                                }

                                if (!MStrIsEmpty(cameraModel) && !MStrIsEmpty(manufacturer)) {
                                    break;
                                }
                            }

                            if (!MStrIsEmpty(cameraModel)) {
                                PTPDeviceInfo* info = MArrayAddPtrZ(self->allocator, *deviceList);
                                info->backendType = PTP_BACKEND_IP;
                                info->product = cameraModel;
                                info->manufacturer = manufacturer;

                                HttpUrl hurl = {};
                                Http_ParseUrl(self->allocator, url, &hurl);
                                info->ipAddress = MStrMakeCopyLen(self->allocator, hurl.host.str, hurl.host.size);
                                info->device = (void*)info->ipAddress.str; // Store host as device data
                                Http_FreeUrl(self->allocator, &hurl);
                                foundDevice = TRUE;
                            }
                        }
                        Http_FreeResponse(self->allocator, &resp);
                    }
                }
            }
        }
    }

    u64 currentTime = MGetTimeMilliseconds();
    if (currentTime - self->discoveryStartTime > 10000) {
        PTP_TRACE("SSDP discovery stopped after waiting for responses for 10s.");
        MSockClose(self->discoverySock);
    }
    return foundDevice;
}

static b32 PTPIp_Close_(PTPBackend* backend) {
    PTPIpBackend* self = backend->self;
    b32 r = PTPIp_Close(self);
    MFree(self->allocator, self, sizeof(PTPIpBackend));
    return r;
}

static b32 PTPIp_RefreshList_(PTPBackend* backend, PTPDeviceInfo** deviceList) {
    PTPIpBackend* self = backend->self;
    return PTPIp_RefreshList(self, deviceList);
}

static b32 PTPIp_NeedsRefresh_(PTPBackend* backend) {
    PTPIpBackend* self = backend->self;
    return PTPIp_NeedsRefresh(self);
}

static b32 PTPIp_IsRefreshingList_(PTPBackend* backend) {
    PTPIpBackend* self = backend->self;
    return self->isDiscoveryInProgress;
}

static b32 PTPIp_PollListUpdates_(PTPBackend* backend, PTPDeviceInfo** deviceList) {
    PTPIpBackend* self = backend->self;
    return PTPIp_PollListUpdates(self, deviceList);
}

static void PTPIp_ReleaseList_(PTPBackend* backend) {
    PTPIpBackend* self = backend->self;
    PTPIp_ReleaseList(self);
}

static b32 PTPIp_OpenDevice_(PTPBackend* backend, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTPIpBackend* self = backend->self;
    return PTPIp_OpenDevice(self, deviceInfo, deviceOut);
}

static b32 PTPIp_CloseDevice_(PTPBackend* backend, PTPDevice* device) {
    PTPIpBackend* self = backend->self;
    return PTPIp_CloseDevice(self, device);
}

b32 PTPIpDeviceList_OpenBackend(PTPBackend* backend, int timeoutMilliseconds) {
    PTP_LOG_TRACE(&backend->logger, "PTPIpDeviceList_OpenBackend");

    if (timeoutMilliseconds == 0) {
        timeoutMilliseconds = USB_TIMEOUT_DEFAULT_MILLISECONDS;
    }

    PTPIpBackend* self = MMallocZ(backend->allocator, sizeof(PTPIpBackend));
    backend->self = self;
    backend->close = PTPIp_Close_;
    backend->refreshList = PTPIp_RefreshList_;
    backend->needsRefresh = PTPIp_NeedsRefresh_;
    backend->isRefreshingList = PTPIp_IsRefreshingList_;
    backend->pollListUpdates = PTPIp_PollListUpdates_;
    backend->releaseList = PTPIp_ReleaseList_;
    backend->openDevice = PTPIp_OpenDevice_;
    backend->closeDevice = PTPIp_CloseDevice_;
    backend->type = PTP_BACKEND_IP;
    self->timeoutMilliseconds = timeoutMilliseconds;
    self->logger = backend->logger;
    self->allocator = backend->allocator;
    MSockInit();
    return TRUE;
}
