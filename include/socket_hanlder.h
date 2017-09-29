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

typedef std::function<bool(char* buf, lw_int32 bufsize)> SocketRecvCallback;

typedef std::function<int(SocketSession* session)> SocketEventHandler;

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