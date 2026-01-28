#include "mlib/msock.h"

#include <string.h>

#ifndef _WIN32
    #include <sys/fcntl.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
#endif

int MSockInit() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2,2), &wsa) == 0 ? 0 : -1;
#else
    return 0;
#endif
}

void MSockDeinit() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void MSockSetSocketTimeout(MSock s, int timeoutMs) {
#ifdef _WIN32
    DWORD timeout = (DWORD)timeoutMs;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
#endif
}

static MSock MSockMakeSocket(int type) {
    return (MSock)socket(AF_INET, type, 0);

    // AF_INET, SOCK_DGRAM, IPPROTO_UDP
}

MSock MSockMakeTcpSocket() {
    return MSockMakeSocket(SOCK_STREAM);
}

MSock MSockMakeUdpSocket() {
    return MSockMakeSocket(SOCK_DGRAM);
}

void MSockSetBroadcast(MSock s, int broadcast) {
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));
}

void MSockSetNonBlocking(MSock s, b32 nonBlocking) {
#ifdef _WIN32
    u_long mode = nonBlocking ? 1 : 0;
    ioctlsocket(s, FIONBIO, &mode);
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags == -1) return;
    if (nonBlocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    fcntl(s, F_SETFL, flags);
#endif
}

void MSockSetMulticastInterface(MSock s, const char* ip) {
    struct in_addr addr;
#ifdef _WIN32
    InetPtonA(AF_INET, ip, &addr);
#else
    inet_pton(AF_INET, ip, &addr);
#endif
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char*)&addr, sizeof(addr));
}

static void FillAddr(struct sockaddr_in* addr, const char* ip, u16 port) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (!ip || ip[0] == '\0') {
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
#ifdef _WIN32
        InetPtonA(AF_INET, ip, &addr->sin_addr);
#else
        inet_pton(AF_INET, ip, &addr->sin_addr);
#endif
    }
}

int MSockBind(MSock s, const char* ip, u16 port) {
    struct sockaddr_in addr;
    FillAddr(&addr, ip, port);
    return bind(s, (struct sockaddr*)&addr, sizeof(addr));
}

int MSockListen(MSock s, int backlog) {
    return listen(s, backlog);
}

MSock MSockAccept(MSock s) {
    return (MSock)accept(s, NULL, NULL);
}

int MSockConnect(MSock s, const char* ip, u16 port) {
    struct sockaddr_in addr;
    FillAddr(&addr, ip, port);
    return connect(s, (struct sockaddr*)&addr, sizeof(addr));
}

int MSockConnectAddress(MSock s, MSockAddress* address) {
    return connect(s, (struct sockaddr*)address, sizeof(*address));
}

int MSockConnectHost(MSock sock, MStrView host, u16 port, MSockAddress* outAddr) {
    // getaddrinfo requires null-terminated strings, so create a temporary copy
    char hostStr[256];
    u32 hostLen = host.size < sizeof(hostStr) - 1 ? host.size : sizeof(hostStr) - 1;
    memcpy(hostStr, host.str, hostLen);
    hostStr[hostLen] = '\0';

    struct addrinfo hints;
    struct addrinfo* res;
    memset(&hints, 0, sizeof(hints));
    if (getaddrinfo(hostStr, NULL, &hints, &res) != 0) {
        return -2;
    }

    if (res->ai_family == AF_INET) {
        struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
        addr->sin_port = htons(port);
    } else if (res->ai_family == AF_INET6) {
        struct sockaddr_in6* addr = (struct sockaddr_in6*)res->ai_addr;
        addr->sin6_port = htons(port);
    }

    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) == MSOCK_ERROR) {
        freeaddrinfo(res);
        return -3;
    }

    if (outAddr) {
        *outAddr = *res->ai_addr;
    }

    freeaddrinfo(res);

    return 0;
}

int MSockSend(MSock s, const void* buf, int len) {
#ifdef _WIN32
    return send(s, (const char*)buf, len, 0);
#else
    return (int)send(s, buf, (size_t)len, 0);
#endif
}

int MSockSendAll(MSock s, MMemIO* memIo) {
    int totalSent = 0;
    int remaining = (int) memIo->size;
    const char* ptr = (const char*) memIo->mem;

    while (remaining > 0) {
        int sent = MSockSend(s, ptr + totalSent, remaining);
        if (sent < 0) {
            return sent;
        }
        if (sent == 0) {
            break;
        }
        totalSent += sent;
        remaining -= sent;
    }

    return totalSent;
}

int MSockRecv(MSock s, void* buf, int len) {
#ifdef _WIN32
    return recv(s, (char*)buf, len, 0);
#else
    return (int)recv(s, buf, (size_t)len, 0);
#endif
}

int MSockRecvAll(MSock s, MMemIO* memIo) {
    int totalReceived = 0;

    while (TRUE) {
        u32 remainingSpace = memIo->capacity - memIo->size;
        if (remainingSpace < 1024) {
            MMemGrowBytes(memIo, 1024 * 4);
            remainingSpace = memIo->capacity - memIo->size;
        }
        int received = MSockRecv(s, memIo->mem + memIo->size, remainingSpace);
        if (received < 0) {
            return received;
        }
        if (received == 0) {
            break;
        }
        memIo->size += received;
        totalReceived += received;
    }

    return totalReceived;
}

int MSockSendTo(MSock s, const void* buf, int len, const char* ip, u16 port) {
    struct sockaddr_in addr;
    FillAddr(&addr, ip, port);
#ifdef _WIN32
    return sendto(s, (const char*)buf, len, 0, (struct sockaddr*)&addr, sizeof(addr));
#else
    return (int)sendto(s, buf, (size_t)len, 0, (struct sockaddr*)&addr, sizeof(addr));
#endif
}

int MSockRecvFrom(MSock s, void* buf, int len, char* outIp, int outIpLen, u16* outPort) {
    struct sockaddr_in addr;
#ifdef _WIN32
    int addrlen = sizeof(addr);
    int n = recvfrom(s, (char*)buf, len, 0, (struct sockaddr*)&addr, &addrlen);
#else
    socklen_t addrlen = sizeof(addr);
    int n = (int)recvfrom(s, buf, (size_t)len, 0, (struct sockaddr*)&addr, &addrlen);
#endif
    if (n >= 0 && outIp && outIpLen > 0) {
#ifdef _WIN32
        InetNtopA(AF_INET, &addr.sin_addr, outIp, outIpLen);
#else
        inet_ntop(AF_INET, &addr.sin_addr, outIp, (socklen_t)outIpLen);
#endif
    }
    if (n >= 0 && outPort) {
        *outPort = ntohs(addr.sin_port);
    }
    return n;
}

int M_SockClose(MSock s) {
#ifdef _WIN32
    return closesocket(s);
#else
    return close(s);
#endif
}

int MSockGetLastError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}
