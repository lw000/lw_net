#include "net.h"

#include <stdio.h>

#if defined(WIN32) || defined(_WIN32)
#include <winsock2.h>
#endif // WIN32

static int lw_socket_init()
{
#if defined(WIN32) || defined(_WIN32)
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	int err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		printf("WSAStartup failed with error: %d\n", err);
		return 0;
	}
	printf("WSAStartup success.\n", err);
#endif
	return 0;
}

static void lw_socket_clean()
{
#if defined(WIN32) || defined(_WIN32)
	WSACleanup();
	printf("WSACleanup.\n");
#endif
}

SocketInit::SocketInit()
{
	lw_socket_init();
}

SocketInit::~SocketInit()
{
	lw_socket_clean();
}


