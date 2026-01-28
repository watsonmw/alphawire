#pragma once

#include "mlib/mlib.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
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

#ifdef __cplusplus
}
#endif
