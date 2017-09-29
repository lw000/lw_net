#ifndef __SocketClient_H__
#define __SocketClient_H__

#include <string>
#include "object.h"
#include "socket_hanlder.h"

#include "Threadable.h"

class SocketConfig;
class SocketClient;
class SocketProcessor;
class NetCore;

class SocketClient : public Object, public Threadable
{
public:
	SocketEventHandler connectedHandler;
	SocketEventHandler disConnectHandler;
	SocketEventHandler timeoutHandler;
	SocketEventHandler errorHandler;

public:
	SocketClient();
	virtual ~SocketClient();

public:
	bool create(AbstractSocketClientHandler* handler, SocketConfig* config);
	void destroy();

public:
	int run();

public:
	int loopbreak();
	int loopexit();

public:
	SocketSession* getSession();

public:
	virtual std::string debug() override;

protected:
	virtual int onStart() override;
	virtual int onRun() override;
	virtual int onEnd() override;

private:
	SocketProcessor* _processor;
	SocketSession* _session;
	NetCore* _core;
};

#endif // !__SocketClient_H__
