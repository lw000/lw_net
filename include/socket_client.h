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

class SocketClient : public Object, public Threadable
{
public:
	SocketEventHandler connectedHandler;

public:
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
	void setAutoHeartBeat(int tms = 30000);

	int close();

public:
	void addTimer(int tid, unsigned int tms, TimerCallback func);
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
	bool _enable_heart_beat;
	int _heart_beat_tms;
	SocketProcessor* _processor;
	ClientSession* _session;
	SocketTimer* _timer;
};

#endif // !__SocketClient_H__
