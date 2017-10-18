#ifndef __session_H__
#define __session_H__

#include <string>
#include <unordered_map>

#include <event2/util.h>

#include "common_type.h"
#include "socket_object.h"
#include "socket_hanlder.h"

class NetIOBuffer;
class SocketConfig;
class SocketProcessor;
class SocketTimer;

enum SESSION_TYPE
{
	none = 0,
	client = 1,
	server = 2,
};

class SocketSession : public SocketObject
{
	friend class SessionCore;

public:
	SocketEventHandler connectedHandler;
	SocketEventHandler disConnectHandler;
	SocketEventHandler timeoutHandler;
	SocketEventHandler errorHandler;
	SocketParseHandler parseHandler;

public:
	SocketSession(SocketConfig* conf);
	virtual ~SocketSession();

public:
	int create(SESSION_TYPE c, SocketProcessor* processor, evutil_socket_t fd = -1);
	void destroy();

public:
	bool connected() const;

public:
	SocketConfig* getConf() const;

public:
	virtual std::string debug() override;

public:
	lw_int32 sendData(lw_int32 cmd, void* object, lw_int32 objectSize);
	lw_int32 sendData(lw_int32 cmd, void* object, lw_int32 objectSize, const SocketRecvHandlerConf& cb);

private:
	void __on_read();
	void __on_write();
	void __on_event(short ev);
	void __on_parse(lw_int32 cmd, char* buf, lw_int32 bufsize);

protected:
	SESSION_TYPE _c;	//session¿‡–Õ
	bool _connected;
	SocketConfig* _conf;
	NetIOBuffer* _iobuffer;
	SocketTimer* _timer;
	struct bufferevent* _bev;
	std::unordered_map<lw_int32, SocketRecvHandlerConf> _event_callback_map;
};


#endif // !__session_H__