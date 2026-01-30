#pragma once

#include "mlib/mlib.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    typedef SOCKET MSock;
    typedef struct sockaddr MSockAddress;
    #define MSOCK_INVALID (INVALID_SOCKET)
    #define MSOCK_ERROR (INVALID_SOCKET)
#else
    typedef int MSock;
    typedef struct sockaddr MSockAddress;
    #define MSOCK_INVALID (-1)
    #define MSOCK_ERROR (-1)
#endif

//
// Socket interface for working with WinSock and POSIX sockets
//

int MSockInit();
void MSockDeinit();

// Set send and recv timeouts for socket
void MSockSetSocketTimeout(MSock s, int timeoutMs);

// Set non-blocking mode
void MSockSetNonBlocking(MSock s, b32 nonBlocking);

MSock MSockMakeTcpSocket();
MSock MSockMakeUdpSocket();
void MSockSetBroadcast(MSock s, int broadcast);
void MSockSetMulticastInterface(MSock s, const char* ip);
int MSockBind(MSock s, const char* ip, u16 port);
int MSockListen(MSock s, int backlog);
MSock MSockAccept(MSock s);
int MSockConnect(MSock s, const char* ip, u16 port);
int MSockConnectAddress(MSock s, MSockAddress* address);
int MSockConnectHost(MSock s, MStrView host, u16 port, MSockAddress* outAddr);

int MSockSend(MSock s, const void* buf, int len);
int MSockSendAll(MSock s, MMemIO* memIo);
int MSockRecv(MSock s, void* buf, int len);
int MSockRecvAll(MSock s, MMemIO* memIo);
int MSockSendTo(MSock s, const void* buf, int len, const char* ip, u16 port);
int MSockRecvFrom(MSock s, void* buf, int len, char* outIp, int outIpLen, u16* outPort);

#define MSockClose(s) if ((s) != MSOCK_INVALID) { M_SockClose((s)); (s) = MSOCK_INVALID;}
int MSockGetLastError();

int M_SockClose(MSock s);

typedef struct MSockInterface {
    MStrView name;                  // interface name (e.g. "eth0", "en0")
    int family;                     // AF_INET / AF_INET6
    int isUp;
    int isLoopback;
    struct sockaddr addr;
    char nameBuffer[128];
} MSockInterface;

typedef enum MSockInterfaceEnumFlags {
    MSockIfAddrFlag_Down = 1,          // include interface even if currently down
    MSockIfAddrFlag_Loopback = 1 << 1, // include loopback
    MSockIfAddrFlag_IPV4 = 1 << 2,     // include ipv4
    MSockIfAddrFlag_IPV6 = 1 << 3,     // include ipv6
                                       // if neither ipv4 or ipv6 is specified then return both
} MSockInterfaceEnumFlags;

// Enumerate interfaces and address
int MSockGetInterfaces(MAllocator* allocator, MSockInterface** outAddr, int flags);

#ifdef __cplusplus
}
#endif
