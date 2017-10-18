#ifndef __session_handler_h__
#define __session_handler_h__

#include "common_type.h"
#include <functional>

class SocketSession;
class SocketServer;
class SocketClient;

class AbstractSocketSessionHanlder;
class AbstractSocketClientHandler;
class AbstractSocketServerHandler;

typedef std::function<bool(lw_char8* buf, lw_int32 bufsize)> SocketRecvHandler;

typedef std::function<void(SocketSession* session)> SocketEventHandler;
typedef std::function<void(SocketSession* session, lw_int32 cmd, lw_char8* buf, lw_int32 bufsize)> SocketParseHandler;

#define SOCKET_EVENT_SELECTOR(__selector__,__target__, ...) std::bind(&__selector__, __target__, std::placeholders::_1, ##__VA_ARGS__)
#define SOCKET_PARSE_SELECTOR_4(__selector__,__target__, ...) std::bind(&__selector__, __target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, ##__VA_ARGS__)

struct SocketRecvHandlerConf
{
	int recvcmd;		//
	int forever;		// 0, 1
	SocketRecvHandler cb;

	SocketRecvHandlerConf() {
		this->recvcmd = -1;
		this->forever = -1;
	}

	SocketRecvHandlerConf(int recvcmd, int forever, const SocketRecvHandler& cb) {
		this->recvcmd = recvcmd;
		this->forever = forever;
		this->cb = cb;
	}
};

class AbstractSocketSessionHanlder
{
	friend class SocketSession;

public:
	virtual ~AbstractSocketSessionHanlder() {}

protected:
	virtual int onSocketConnected(SocketSession* session) = 0;
	virtual int onSocketDisConnect(SocketSession* session) = 0;
	virtual int onSocketTimeout(SocketSession* session) = 0;
	virtual int onSocketError(SocketSession* session) = 0;

protected:
	virtual void onSocketParse(SocketSession* session, lw_int32 cmd, lw_char8* buf, lw_int32 bufsize) = 0;
};

class AbstractSocketClientHandler : public AbstractSocketSessionHanlder
{
	friend class SocketClient;

public:
	virtual ~AbstractSocketClientHandler() {}
};

class AbstractSocketServerHandler : public AbstractSocketSessionHanlder
{
	friend class SocketServer;

public:
	virtual ~AbstractSocketServerHandler() {}

protected:
	virtual void onListener(SocketSession* session) = 0;
};

#endif // !__session_handler_h__