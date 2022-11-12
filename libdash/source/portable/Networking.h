#ifndef PORTABLE_NETWORKING_H_
#define PORTABLE_NETWORKING_H_

#if defined _WIN32 || defined _WIN64

#include <WS2tcpip.h>
#include <WinSock2.h>

#else

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define closesocket(socket) close(socket)
#define WSAStartup(wVersionRequested, lpWSAData) 0
#define WSACleanup() \
    {                \
    }

typedef unsigned char WSADATA;

#endif

#endif // PORTABLE_NETWORKING_H_
