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
class NetCore;
class SocketConfig;
class SocketSession;
class SocketProcessor;
class SocketListener;

class SocketServer : public Object, public Threadable
{
public:
	SocketServer();
	virtual ~SocketServer();

public:
	bool create(AbstractSocketServerHandler* handler, SocketConfig* config);
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
	NetCore* _core;
	SocketListener* _listener;
	SocketProcessor* _processor;

private:
	std::function<void(lw_int32 what)> _onFunc;
	AbstractSocketServerHandler* _handler;
};

#endif // !__SocketServer_H__ 
