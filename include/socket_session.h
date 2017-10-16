#ifndef __session_H__
#define __session_H__

#include <string>
#include <unordered_map>

#include <event2/util.h>

#include "common_type.h"
#include "object.h"

#include "socket_hanlder.h"

class NetIOBuffer;
class SocketConfig;
class SocketSession;
class SocketProcessor;

enum SESSION_TYPE
{
	NONE = 0,
	Client = 1,
	Server = 2,
};

class SocketSession : public Object
{
	friend class CoreSocket;

public:
	SocketEventHandler connectedHandler;
	SocketEventHandler disConnectHandler;
	SocketEventHandler timeoutHandler;
	SocketEventHandler errorHandler;

public:
	SocketSession(AbstractSocketSessionHanlder* handler, SocketConfig* config);
	virtual ~SocketSession();

public:
	int create(SESSION_TYPE c, SocketProcessor* processor, evutil_socket_t fd = -1);
	void destroy();

public:
	bool connected();
	SocketConfig* getConfig() const;

public:
	virtual std::string debug() override;

public:
	lw_int32 sendData(lw_int32 cmd, void* object, lw_int32 objectSize);
	lw_int32 sendData(lw_int32 cmd, void* object, lw_int32 objectSize, lw_int32 recvcmd, SocketRecvCallback cb);

private:
	void __onRead();
	void __onWrite();
	void __onEvent(short ev);
	void __onParse(lw_int32 cmd, char* buf, lw_int32 bufsize);

private:
	std::unordered_map<lw_int32, SocketRecvCallback> _cmd_event_map;

private:
	SESSION_TYPE _c;	//session¿‡–Õ

private:
	bool _connected;
	SocketConfig* _config;

private:
	NetIOBuffer* _iobuffer;
	SocketProcessor* _processor;
	struct bufferevent* _bev;
	AbstractSocketSessionHanlder * _handler;
};


#endif // !__session_H__