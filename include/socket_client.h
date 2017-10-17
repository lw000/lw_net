#ifndef __SocketClient_H__
#define __SocketClient_H__

#include <string>

#include "object.h"
#include "Threadable.h"
#include "socket_hanlder.h"
#include "socket_timer.h"

class SocketConfig;
class SocketClient;
class SocketProcessor;
class SocketTimer;

class SocketClient : public Object, public Threadable
{
public:
	SocketEventHandler connectedHandler;
	SocketEventHandler disConnectHandler;
	SocketEventHandler timeoutHandler;
	SocketEventHandler errorHandler;

public:
	SocketParseHandler parseHandler;

public:
	SocketClient();
	virtual ~SocketClient();

public:
	bool create(SocketConfig* conf);
	void destroy();

public:
	int close();

public:
	void startTimer(int tid, unsigned int tms, TimerCallback func);
	void killTimer(int tid);

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
	SocketTimer* _timer;
};

#endif // !__SocketClient_H__
