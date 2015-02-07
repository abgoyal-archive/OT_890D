

#include <linux/net.h>
#include <linux/in.h>
#include "SocketLinuxKernel.h"

/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// modul globale vars
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//  Kernel Module specific Data Structures
//---------------------------------------------------------------------------

//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
//
//
// Parameters:
//
//
// Returns:
//
//
// State:
//
//---------------------------------------------------------------------------

SOCKET socket(int af, int type, int protocol)
{
	int rc;
	SOCKET socket;

	rc = sock_create_kern(af, type, protocol, &socket);
	if (rc < 0) {
		socket = NULL;
		goto Exit;
	}

      Exit:
	return socket;
}

int bind(SOCKET socket_p, const struct sockaddr *addr, int addrlen)
{
	int rc;

	rc = socket_p->ops->bind(socket_p, (struct sockaddr *)addr, addrlen);

	return rc;
}

int closesocket(SOCKET socket_p)
{
	sock_release(socket_p);

	return 0;
}

int recvfrom(SOCKET socket_p, char *buf, int len, int flags,
	     struct sockaddr *from, int *fromlen)
{
	int rc;
	struct msghdr msg;
	struct kvec iov;

	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_name = from;	// will be struct sock_addr
	msg.msg_namelen = *fromlen;
	iov.iov_len = len;
	iov.iov_base = buf;

	rc = kernel_recvmsg(socket_p, &msg, &iov, 1, iov.iov_len, 0);

	return rc;
}

int sendto(SOCKET socket_p, const char *buf, int len, int flags,
	   const struct sockaddr *to, int tolen)
{
	int rc;
	struct msghdr msg;
	struct kvec iov;

	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_name = (struct sockaddr *)to;	// will be struct sock_addr
	msg.msg_namelen = tolen;
	msg.msg_flags = 0;
	iov.iov_len = len;
	iov.iov_base = (char *)buf;

	rc = kernel_sendmsg(socket_p, &msg, &iov, 1, len);

	return rc;
}

// EOF
