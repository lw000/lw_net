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
class ClientSession;
class SocketHeartbeat;

class SocketClient : public Object, public Threadable
{
public:
	SocketEventHandler connectedHandler;

public:
	SocketEventHandler disConnectHandler;
	SocketEventHandler timeoutHandler;
	SocketEventHandler errorHandler;

public:
	SocketParseDataHandler parseHandler;

public:
	SocketClient();
	virtual ~SocketClient();

public:
	bool create(SocketConfig* conf);
	void destroy();

public:
	void setAutoHeartBeat(int tms = 30000);

public:
	int close();

public:
	int addTimer(int tid, unsigned int tms, const TimerCallback& func);
	void removeTimer(int tid);

public:
	ClientSession* getSession();

public:
	virtual std::string debug() override;

protected:
	virtual int onStart() override;
	virtual int onRun() override;
	virtual int onEnd() override;

private:
	SocketHeartbeat* _heartbeat;
	SocketProcessor* _processor;
	ClientSession* _session;
};

#endif // !__SocketClient_H__
