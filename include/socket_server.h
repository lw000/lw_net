#ifndef __SocketServer_H__
#define __SocketServer_H__

#include <vector>
#include <list>
#include <unordered_map>
#include <event2/util.h>
#include "socket_object.h"
#include "Threadable.h"
#include "socket_hanlder.h"

#include <functional>

class SocketTimer;
class SocketConfig;
class SocketSession;
class SocketProcessor;
class SocketListener;

class SocketServer : public SocketObject, public Threadable
{
public:
	SocketEventHandler connectedHandler;
	SocketEventHandler disConnectHandler;
	SocketEventHandler timeoutHandler;
	SocketEventHandler errorHandler;

public:
	SocketParseHandler parseHandler;

public:
	SocketEventHandler listenHandler;

public:
	SocketServer();
	virtual ~SocketServer();

public:
	bool create(SocketConfig* config);
	void destroy();

public:
	lw_int32 serv(std::function<void(lw_int32 what)> func);

public:
	int loopbreak();
	int loopexit();

public:
	virtual std::string debug() override;

protected:
	virtual int onStart() override;
	virtual int onRun() override;
	virtual int onEnd() override;

private:
	SocketListener* _listener;
	SocketProcessor* _processor;

private:
	std::function<void(lw_int32 what)> _onFunc;
	//AbstractSocketServerHandler* _handler;
};

#endif // !__SocketServer_H__ 
