#ifndef __SocketHeartbeat_H__
#define __SocketHeartbeat_H__

#include "common_type.h"
#include "socket_object.h"
#include "socket_hanlder.h"

class SocketSession;
class SocketProcessor;

class SocketHeartbeat : public SocketObject
{
private:
	bool _enabled;
	int _keepalivetime;
	int _keepaliveinterval;
	SocketSession* _session;
	SocketProcessor* _processor;

public:
	SocketEventHandler onConnectedHandler;
	SocketEventHandler onDisconnectHandler;
	SocketEventHandler onTimeoutHandler;
	SocketEventHandler onErrorHandler;
	SocketDataParseHandler onDataParseHandler;

public:
	SocketHeartbeat(SocketSession* session);
	virtual ~SocketHeartbeat();

public:
	bool create(SocketProcessor* processor);
	void destroy();

public:
	void setAutoHeartBeat(int tms = 10000);
	bool enableHeartBeat() const;

public:
	void ping();
	void pong();
};

#endif // !__SocketHeartbeat_H__ 
