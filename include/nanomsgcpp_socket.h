#ifndef __nanomsgcpp_socket_h__
#define __nanomsgcpp_socket_h__

#include <stddef.h>
#include <functional>
#include "common_type.h"

class NetIOBuffer;

typedef std::function<int (lw_int32 cmd, lw_char8* buf, lw_int32 bufsize)> NanomsgDataParseHandler;

class AbstractNanomsgcppSocket {
public:
		virtual ~AbstractNanomsgcppSocket() {

		}

public:

};

class NanomsgcppSocket
{
	friend class NanomsgcppCore;

public:
	NanomsgcppSocket();
	virtual ~NanomsgcppSocket();

public:
	int create(int domain, unsigned int protocol);
	int bind(const char *addr);
	int connect(const char *addr);
	int setsockopt(int level, int option, const void *optval, size_t optvallen);
	int getsockopt(int level, int option, void *optval, size_t *optvallen);
	int shutdown();
	int close();

public:
	int send(int cmd, void* object, int objectSize, int flags=0);
	int sendmsg(int cmd, void* object, int objectSize, const struct nn_msghdr *msghdr, int flags=0);
	int recv();
	int recv(const NanomsgDataParseHandler& handler);
	int recvmsg(struct nn_msghdr *msghdr, int flags);

private:
	virtual void onRecv(lw_int32 cmd, char* buf, lw_int32 bufsize) = 0;

protected:
	int _fd;

protected:
	NetIOBuffer* _iobuffer;
	NanomsgDataParseHandler _handler;
};

class NanomsgcppSocketServerThread : public NanomsgcppSocket {
public:
		NanomsgcppSocketServerThread();
		virtual ~NanomsgcppSocketServerThread();

public:
		int recv();
		int recv(const NanomsgDataParseHandler& handler);
};

#endif	//__nanomsgcpp_socket_h__

