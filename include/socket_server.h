#ifndef __SocketServer_H__
#define __SocketServer_H__

#include <vector>
#include <list>
#include <unordered_map>
#include <event2/util.h>
#include "object.h"
#include "Threadable.h"
#include "socket_hanlder.h"

#include <functional>

class Timer;
class SocketConfig;
class SocketSession;
class SocketProcessor;
class NetCore;
class SocketListener;

struct evconnlistener;

class SocketServer : public Object, public Threadable
{
public:
	SocketServer();
	virtual ~SocketServer();

public:
	bool create(AbstractSocketServerHandler* handler);
	void destroy();

public:
	lw_int32 listen(SocketConfig* config, std::function<void(lw_int32 what)> func);

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
	NetCore* _core;
	SocketListener* _listener;
	SocketProcessor* _processor;

private:
	Timer* _timer;

private:
	std::function<void(lw_int32 what)> _onFunc;
	AbstractSocketServerHandler* _handler;
};

#endif // !__SocketServer_H__ 
