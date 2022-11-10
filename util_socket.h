#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <errno.h>

#ifdef _WIN32
#define socketerr WSAGetLastError()
#else
#define socketerr   errno
#define SOCKET      int
#define closesocket close
#endif

#define HOST_MAX (260)
