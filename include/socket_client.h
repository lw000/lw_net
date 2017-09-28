#ifndef __SocketClient_H__
#define __SocketClient_H__

#include <string>
#include "object.h"
#include "socket_hanlder.h"

#include "Threadable.h"

class SocketClient;
class SocketProcessor;
class SocketCore;

class SocketClient : public Object, public Threadable
{
public:
	SocketClient();
	virtual ~SocketClient();

public:
	bool create(SocketProcessor* processor, AbstractSocketClientHandler* handler);
	void destroy();

public:
	void setAddr(const std::string& addr);
	void setPort(int port);

public:
	int run(const std::string& addr, int port);

public:
	int loopbreak();
	int loopexit();

public:
	SocketSession* getSession();

public:
	virtual std::string debug() override;

protected:
	virtual int onStart() override;
	virtual int run() override;
	virtual int onEnd() override;

private:
	SocketProcessor* _processor;
	SocketSession* _session;
	SocketCore* _core;
	AbstractSocketClientHandler* _handler;
};

#endif // !__SocketClient_H__
