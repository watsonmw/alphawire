#include "platform/ip/ptp-backend-ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mlib/msock.h"
#include "mlib/mxml.h"
#include "platform/ip/http-client.h"
#include "platform/usb-const.h"
#include "ptp/ptp-device-list.h"

#ifndef _WIN32
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

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
    u32 sessionId; // Session id for device connection
    u32 transactionId; // Next requestion transaction id

    // Event socket
    MSock eventSock;
    MMemIO eventMem; // Event buffer for reading and parsing events (reused across calls)
    u32 eventSockTimeoutMilliseconds; // Cached timeout for event socket operations in milliseconds
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

static AwResult PTPIp_ReleaseList(PTPIpBackend* backend) {
    return (AwResult){.code=AW_RESULT_OK};
}

static b32 PTPIp_NeedsRefresh(PTPIpBackend* backend) {
    return FALSE;
}

static AwResult PTPIp_Close(PTPIpBackend* backend) {
    PTP_TRACE("PTPIp_Close");
    PTPIp_ReleaseList(backend);
    if (backend->openDevices) {
        MArrayFree(backend->allocator, backend->openDevices);
    }
    MSockDeinit();
    return (AwResult){.code=AW_RESULT_OK};
}

static void* PTPDeviceIp_ReallocBuffer(PTPDevice* self, PTPBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize) {
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataNewSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataOldSize + headerSize); dataMem = NULL;
    }
    dataMem = MMallocZ(self->transport.allocator, dataSize);
    return ((u8*)dataMem) + headerSize;
}

static void PTPDeviceIp_FreeBuffer(PTPDevice* self, PTPBufferType type, void* dataMem, size_t dataOldSize) {
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataOldSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataSize); dataMem = NULL;
    }
}

static int TcpReadBytesIntoBuffer(MSock socket, MMemIO* in, MMemIO* inRead, u32 bytesToRead) {
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

static int TcpSendAllBytes(MSock socket, const void* data, size_t dataSize) {
    int totalSent = 0;
    while (totalSent < dataSize) {
        int s = send(socket, (char *) data + totalSent, (int) (dataSize - totalSent), 0);
        if (s == MSOCK_ERROR || s == 0) {
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

static AwResult PTPDeviceIp_SendAndRecv(PTPDevice* self, PTPRequestHeader* request, u8* dataIn, size_t dataInSize,
                                        PTPResponseHeader* response, u8* dataOut, size_t dataOutSize,
                                        size_t* actualDataOutSize) {
    PTPIpDevice* dev = (PTPIpDevice*)self->device;
    MAllocator* allocator = self->transport.allocator;
    AwResult error = {AW_RESULT_OK, PTP_OK};

    MMemIO in = {0};
    MMemIO out;
    MMemInitEmpty(&out, allocator);

    // 1. Send request packet
    u32 packetLen = 4 + 4 + 4 + 2 + 4 + (request->NumParams * 4);
    MMemWriteU32LE(&out, packetLen);
    MMemWriteU32LE(&out, PTPIP_TYPE_CMD_REQUEST);
    MMemWriteU32LE(&out, dataInSize == 0 ? 1 : 2);
    MMemWriteU16LE(&out, request->OpCode);
    MMemWriteU32LE(&out, request->TransactionId);
    for (int i = 0; i < request->NumParams; ++i) {
        MMemWriteU32LE(&out, request->Params[i]);
    }
    int s = TcpSendAllBytes(dev->dataSock, out.mem, out.size);
    if (s == MSOCK_ERROR) {
        error.code = AW_RESULT_TIMEOUT;
        goto exitWithError;
    } else if (s == 0) {
        error.code = AW_RESULT_CONNECTION_CLOSED;
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
        if (s == MSOCK_ERROR) {
            goto exitWithError;
        } else if (s == 0) {
            goto exitWithError;
        }
    }

    MMemFree(&out);

    // 3. Receive Response(s)
    MMemInitAlloc(&in, allocator, 1024);

    MMemIO inRead = {in.mem, 0, in.capacity};
    u64 transferLen = 0;
    u8* dataOutCurrent = dataOut;
    i64 dataRemaining = (i64)dataOutSize;

    while (TRUE) {
        int r = TcpReadBytesIntoBuffer(dev->dataSock, &in, &inRead, 8);
        if (r == MSOCK_ERROR) {
            error.code = AW_RESULT_TIMEOUT;
            goto exitWithError;
        } else if (r == 0) {
            error.code = AW_RESULT_CONNECTION_CLOSED;
            goto exitWithError;
        }

        u32 responseLen, responseType;
        MMemReadU32LE(&inRead, &responseLen);
        MMemReadU32LE(&inRead, &responseType);
        if (responseLen < 8) {
            error.code = AW_RESULT_MALFORMED_RESPONSE;
            goto exitWithError;
        }

        u32 payloadLen = responseLen - 8;
        r = TcpReadBytesIntoBuffer(dev->dataSock, &in, &inRead, payloadLen);
        inRead.mem = in.mem;
        if (r == MSOCK_ERROR) {
            error.code = AW_RESULT_TIMEOUT;
            goto exitWithError;
        } else if (r == 0) {
            error.code = AW_RESULT_CONNECTION_CLOSED;
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
                error.code = AW_RESULT_MALFORMED_RESPONSE;
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
                error.code = AW_RESULT_MALFORMED_RESPONSE;
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
                error.code = AW_RESULT_MALFORMED_RESPONSE;
                goto exitWithError;
            }
        } else if (responseType == PTPIP_TYPE_DATA_PACKET_END) {
            if (payloadLen == 4) {
                u32 transactionId;
                MMemReadU32LE(&inRead, &transactionId);
            } else {
                error.code = AW_RESULT_MALFORMED_RESPONSE;
                goto exitWithError;
            }
        } else {
            error.code = AW_RESULT_MALFORMED_RESPONSE;
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

    if (response->ResponseCode == PTP_OK) {
        return (AwResult){.code=AW_RESULT_OK,.ptp=PTP_OK};
    } else {
        return (AwResult){.code=AW_RESULT_PTP_FAILURE,.ptp=response->ResponseCode};
    }

exitWithError:
    MMemFree(&out);
    MMemFree(&in);
    return error;
}

// TODO fix PTPResult it's really two things AW error + PTP error code
static AwResult PTPDeviceIp_ReadEvents(PTPDevice* self, int timeoutMilliseconds, MAllocator* alloc, PTPEvent** outEvents) {
    if (!outEvents) {
        return (AwResult){.code=AW_RESULT_UNSUPPORTED};
    }
    b32 gotEvent = FALSE;

    PTPIpDevice* dev = (PTPIpDevice*)self->device;
    if (!dev || dev->eventSock == MSOCK_INVALID) {
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    // Initialize event buffer on first use
    if (dev->eventMem.allocator == NULL) {
        MMemInitAlloc(&dev->eventMem, self->transport.allocator, 4096);
    } else {
        // Ensure we have space in buffer
        if (dev->eventMem.size + 1024 > dev->eventMem.capacity) {
            MMemGrowBytes(&dev->eventMem, 4096);
        }
    }

    if (dev->eventSockTimeoutMilliseconds != timeoutMilliseconds) {
        if (timeoutMilliseconds > 0) {
            MSockSetNonBlocking(dev->eventSock, 0);
            MSockSetSocketTimeout(dev->eventSock, timeoutMilliseconds);
        } else {
            MSockSetNonBlocking(dev->eventSock, 1);
        }
        dev->eventSockTimeoutMilliseconds = timeoutMilliseconds;
    }

    // Parse events
    // TODO: Capacity should be the size of the data
    MMemIO inRead = {dev->eventMem.mem, 0, dev->eventMem.capacity};

    // TODO: Start non blocking, only move to blocking if no data is found (and timeout is > 0)
    int r = 0;
    do {
        r = TcpReadBytesIntoBuffer(dev->eventSock, &dev->eventMem, &inRead, 8);
        if (r < 0) {
            // we weren't able to read he required number of bytes (within the timeout, if any)
            break;
        }

        // Try parsing again with new data
        if (dev->eventMem.size >= 8) {
            u32 packetLen = 0;
            u32 packetType = 0;
            MMemReadU32LE(&inRead, &packetLen);
            MMemReadU32LE(&inRead, &packetType);
            if (packetLen > 8) {
                r = TcpReadBytesIntoBuffer(dev->eventSock, &dev->eventMem, &inRead, packetLen-8);
                if (r < 0) {
                    // we weren't able to read he required number of bytes (within the timeout, if any)
                    break;
                }
                if (packetType == PTPIP_TYPE_EVENT) {
                    PTPEvent* outEvent = MArrayAddPtrZ(alloc, *outEvents);

                    MMemIO payloadRead;
                    MMemInitRead(&payloadRead, inRead.mem + inRead.size, packetLen - 8);
                    u16 eventCode = 0;
                    MMemReadU16LE(&payloadRead, &eventCode);
                    outEvent->code = (u32)eventCode;
                    outEvent->size = packetLen - 8;

                    u32 transactionId = 0;
                    MMemReadU32LE(&payloadRead, &transactionId);
                    MMemReadU32LE(&payloadRead, &outEvent->param1);
                    MMemReadU32LE(&payloadRead, &outEvent->param2);
                    MMemReadU32LE(&payloadRead, &outEvent->param3);

                    gotEvent = TRUE;
                }
                // Skip the rest if the packet
                MMemReadSkipBytes(&inRead, packetLen - 8);
            }
        } else {
            break;
        }
    } while (TRUE);

    // Remember remaining data packet data for next time
    if (inRead.mem + inRead.size < dev->eventMem.mem + dev->eventMem.size) {
        dev->eventMem.size = dev->eventMem.size - inRead.size;
        memmove(dev->eventMem.mem, inRead.mem + inRead.size, dev->eventMem.size);
    } else {
        dev->eventMem.size = 0;
    }

    if (r == MSOCK_ERROR) {
        MSockError e = MSockGetLastError();
        if (e.timeout) {
            if (gotEvent) {
                return (AwResult){.code=AW_RESULT_OK};
            } else {
                return (AwResult){.code=AW_RESULT_TIMEOUT};
            }
        }
    } else if (r == 0) {
        return (AwResult){.code=AW_RESULT_CONNECTION_CLOSED};
    } else {
        return (AwResult){.code=AW_RESULT_OK};
    }
}

static AwResult PTPIp_OpenDevice(PTPIpBackend* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTP_TRACE("PTPIp_OpenDevice");
    MMemIO out = {0};
    MMemIO in = {0};
    AwResult error = {AW_RESULT_OK };

    PTPIpDevice dev = {};
    dev.eventSock = MSOCK_INVALID;
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
    if (s == MSOCK_ERROR) {
        error.code = AW_RESULT_TIMEOUT;
        goto exitWithError;
    } else if (s == 0) {
        error.code = AW_RESULT_CONNECTION_CLOSED;
        goto exitWithError;
    }

    MMemFree(&out);

    MMemInitAlloc(&in, self->allocator, 1024);
    MMemIO inRead = {in.mem, 0, in.capacity};

    // Wait for Init Command Ack
    int r = TcpReadBytesIntoBuffer(dev.dataSock, &in, &inRead, 8);
    if (r == MSOCK_ERROR) {
        error.code = AW_RESULT_TIMEOUT;
        goto exitWithError;
    } else if (r == 0) {
        error.code = AW_RESULT_CONNECTION_CLOSED;
        goto exitWithError;
    }

    u32 responseLen, responseType;
    MMemReadU32LE(&inRead, &responseLen);
    MMemReadU32LE(&inRead, &responseType);
    if (responseLen < 8) {
        error.code = AW_RESULT_MALFORMED_RESPONSE;
        goto exitWithError;
    }

    u32 payloadLen = responseLen - 8;
    r = TcpReadBytesIntoBuffer(dev.dataSock, &in, &inRead, payloadLen);
    if (r == MSOCK_ERROR) {
        error.code = AW_RESULT_TIMEOUT;
        goto exitWithError;
    } else if (r == 0) {
        error.code = AW_RESULT_CONNECTION_CLOSED;
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
    if (dev.eventSock == MSOCK_INVALID) {
        goto exitWithError;
    }

    MSockSetSocketTimeout(dev.eventSock, 5000);
    dev.eventSockTimeoutMilliseconds = 5000;
    if (MSockConnectAddress(dev.eventSock, &socketAddr) == MSOCK_INVALID) {
        goto exitWithError;
    }

    // Init Event Request
    MMemInitEmpty(&out, self->allocator);
    MMemWriteU32LE(&out, 12); // Length: 8 (header) + 4 (sessionId)
    MMemWriteU32LE(&out, PTPIP_TYPE_INIT_EVENT_REQUEST);
    MMemWriteU32LE(&out, dev.sessionId);

    s = TcpSendAllBytes(dev.eventSock, out.mem, out.size);
    if (s == MSOCK_ERROR) {
        error.code = AW_RESULT_TIMEOUT;
        goto exitWithError;
    } else if (s == 0) {
        error.code = AW_RESULT_CONNECTION_CLOSED;
        goto exitWithError;
    }
    MMemFree(&out);

    MMemReset(&in);
    MMemReset(&inRead);

    // Wait for Init Event Ack
    r = TcpReadBytesIntoBuffer(dev.eventSock, &in, &inRead, 8);
    if (r == MSOCK_ERROR) {
        error.code = AW_RESULT_TIMEOUT;
        goto exitWithError;
    } else if (r == 0) {
        error.code = AW_RESULT_CONNECTION_CLOSED;
        goto exitWithError;
    }

    MMemReadU32LE(&inRead, &responseLen);
    MMemReadU32LE(&inRead, &responseType);
    if (responseLen < 8) {
        error.code = AW_RESULT_MALFORMED_RESPONSE;
        goto exitWithError;
    }

    if (responseType != PTPIP_TYPE_INIT_EVENT_ACK) {
        goto exitWithError;
    }

    MMemFree(&in);

    PTPIpDevice* newDev = MArrayAddPtr(self->allocator, self->openDevices);
    memset(newDev, 0, sizeof(PTPIpDevice));
    *newDev = dev;
    PTPDevice* device = *deviceOut;
    device->backendType = PTP_BACKEND_IP;
    device->device = newDev;
    device->transport.allocator = self->allocator;
    device->transport.sendAndRecv = PTPDeviceIp_SendAndRecv;
    device->transport.reallocBuffer = PTPDeviceIp_ReallocBuffer;
    device->transport.freeBuffer = PTPDeviceIp_FreeBuffer;
    device->transport.readEvents = PTPDeviceIp_ReadEvents;
    device->transport.reset = NULL;
    device->transport.requiresSessionOpenClose = TRUE;
    device->logger = self->logger;
    device->disconnected = FALSE;
    return error;

exitWithError:
    MMemFree(&out);
    MMemFree(&in);

    MSockClose(dev.dataSock);
    MSockClose(dev.eventSock);
    return error;
}

static AwResult PTPIp_CloseDevice(PTPIpBackend* backend, PTPDevice* device) {
    PTP_TRACE("PTPIp_CloseDevice");
    PTPIpDevice* dev = (PTPIpDevice*)device->device;
    MSockClose(dev->dataSock);
    MSockClose(dev->eventSock);

    // Free event buffer if allocated
    MMemFree(&dev->eventMem);

    MArrayEachPtr(backend->openDevices, it) {
        if (it.p == dev) {
            MArrayRemoveIndex(backend->openDevices, it.i);
            break;
        }
    }
    return (AwResult){.code=AW_RESULT_OK};
}

static AwResult PTPIp_RefreshList(PTPIpBackend* self, PTPDeviceInfo** deviceList) {
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
    MSockInterface* interfaces = NULL;
    if (MSockGetInterfaces(self->allocator, &interfaces, MSockIfAddrFlag_IPV4)) {
        MArrayEachPtr(interfaces, it) {
            MSockInterface* iface = it.p;
            struct sockaddr_in* ip = (struct sockaddr_in*)&iface->addr;

            char ipStr[INET6_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &ip->sin_addr, ipStr, sizeof(ipStr)) == NULL) {
                ipStr[0] = '\0';
            }

            PTP_INFO_F("SSDP device search on interface: %s", ipStr);

            if (setsockopt(self->discoverySock, IPPROTO_IP, IP_MULTICAST_IF, (char*)&ip->sin_addr,
                sizeof(ip->sin_addr)) == MSOCK_ERROR) {
                PTP_ERROR_F("Failed to set IP_MULTICAST_IF for %s: %d", ipStr, MSockGetLastError());
            }

            int sent = sendto(self->discoverySock, msg, msgSize, 0, (struct sockaddr*)&addr, sizeof(addr));
            if (sent == MSOCK_ERROR) {
                PTP_ERROR_F("PTPIp_RefreshList: sendto failed for %s with error %d", ipStr, MSockGetLastError());
            }
        }
    } else {
        PTP_ERROR_F("MSockGetInterfaces failed: %d", MSockGetLastError());
        goto exitWithError;
    }
    MArrayFree(self->allocator, interfaces);

    return (AwResult){.code=AW_RESULT_OK};

exitWithError:
    MSockClose(self->discoverySock);
    self->isDiscoveryInProgress = FALSE;
    return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
}

static b32 PTPIp_PollListUpdates(PTPIpBackend* self, PTPDeviceInfo** deviceList) {
    b32 foundDevice = FALSE;
    char buffer[4096];
    struct sockaddr_in from;
    unsigned int fromLen = sizeof(from);
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

static AwResult PTPIp_Close_(PTPBackend* backend) {
    PTPIpBackend* self = backend->self;
    AwResult r = PTPIp_Close(self);
    MFree(self->allocator, self, sizeof(PTPIpBackend));
    return r;
}

static AwResult PTPIp_RefreshList_(PTPBackend* backend, PTPDeviceInfo** deviceList) {
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

static AwResult PTPIp_ReleaseList_(PTPBackend* backend) {
    PTPIpBackend* self = backend->self;
    return PTPIp_ReleaseList(self);
}

static AwResult PTPIp_OpenDevice_(PTPBackend* backend, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTPIpBackend* self = backend->self;
    return PTPIp_OpenDevice(self, deviceInfo, deviceOut);
}

static AwResult PTPIp_CloseDevice_(PTPBackend* backend, PTPDevice* device) {
    PTPIpBackend* self = backend->self;
    return PTPIp_CloseDevice(self, device);
}

AwResult PTPIpDeviceList_OpenBackend(PTPBackend* backend, int timeoutMilliseconds) {
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
    return (AwResult){.code=AW_RESULT_OK};
}
