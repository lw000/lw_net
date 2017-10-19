#ifndef __SocketServer_H__
#define __SocketServer_H__

#include <vector>
#include <list>
#include <unordered_map>
#include <event2/util.h>
#include "socket_object.h"
#include "socket_hanlder.h"
#include "socket_timer.h"

#include "Threadable.h"

#include <functional>

class SocketTimer;
class SocketConfig;
class ServerSession;
class SocketProcessor;
class SocketListener;

class SocketServer : public SocketObject, public Threadable
{
public:
	SocketListenerHandler listenHandler;

// public:
// 	SocketEventHandler disConnectHandler;
// 	SocketEventHandler timeoutHandler;
// 	SocketEventHandler errorHandler;
// 
// public:
// 	SocketParseHandler parseHandler;

public:
	SocketServer();
	virtual ~SocketServer();

public:
	bool create(SocketConfig* config);
	void destroy();

public:
	lw_int32 serv(std::function<void(lw_int32 what)> func);

public:
	void addTimer(int tid, unsigned int tms, TimerCallback func);
	void removeTimer(int tid);

public:
	int close();

public:
	virtual std::string debug() override;

protected:
	virtual int onStart() override;
	virtual int onRun() override;
	virtual int onEnd() override;

private:
	SocketTimer* _timer;
	SocketListener* _listener;
	SocketProcessor* _processor;

private:
	std::function<void(lw_int32 what)> _onFunc;
};

#endif // !__SocketServer_H__ 
