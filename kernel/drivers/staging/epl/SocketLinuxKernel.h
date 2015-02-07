

#ifndef _SOCKETLINUXKERNEL_H_
#define _SOCKETLINUXKERNEL_H_

#include <linux/net.h>
#include <linux/in.h>

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#define INVALID_SOCKET  0

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef struct socket *SOCKET;

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

int bind(SOCKET s, const struct sockaddr *addr, int addrlen);

int closesocket(SOCKET s);

int recvfrom(SOCKET s, char *buf, int len, int flags, struct sockaddr *from,
	     int *fromlen);

int sendto(SOCKET s, const char *buf, int len, int flags,
	   const struct sockaddr *to, int tolen);

SOCKET socket(int af, int type, int protocol);

#endif // #ifndef _SOCKETLINUXKERNEL_H_
