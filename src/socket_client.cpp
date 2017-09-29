#include "socket_client.h"

#include <stdio.h>
#include <assert.h>

#include "event2/event.h"

#include "net_core.h"
#include "socket_config.h"
#include "socket_hanlder.h"
#include "socket_processor.h"
#include "socket_session.h"

#include "common_marco.h"

#include "log4z.h"
using namespace zsummer::log4z;

using namespace lwstar;

////////////////////////////////////////////////////////////////////////////////////////////////////

SocketClient::SocketClient() : _processor(nullptr), _session(nullptr)
{
	_core = new NetCore;

	this->_processor = new SocketProcessor;
}

SocketClient::~SocketClient()
{
	SAFE_DELETE(this->_session);
	SAFE_DELETE(this->_core);
	SAFE_DELETE(this->_processor);
}

bool SocketClient::create(AbstractSocketClientHandler* handler, SocketConfig* config)
{													  
	bool r = this->_processor->create(false);
	if (r)
	{
		this->_session = new SocketSession(handler, this->_core, config);
		this->_session->connectedHandler = this->connectedHandler;
		this->_session->disConnectHandler = this->disConnectHandler;
		this->_session->timeoutHandler = this->timeoutHandler;
		this->_session->errorHandler = this->errorHandler;
		return true;
	}

	return false;
}

void SocketClient::destroy()
{
	if (this->_session != nullptr)
	{
		this->_session->destroy();
	}

	if (this->_processor != nullptr)
	{
		this->_processor->destroy();
	}
}

std::string SocketClient::debug()
{
	char buf[512];
	sprintf(buf, "%s", _session->debug().c_str());
	return std::string(buf);
}

int SocketClient::run()
{
	this->start();

	return 0;
}

int SocketClient::loopbreak()
{
	return this->_processor->loopbreak();
}

int SocketClient::loopexit()
{
	return this->_processor->loopexit();
}

SocketSession* SocketClient::getSession()
{
	return this->_session;
}

int SocketClient::onStart() {
	return 0;
}

int SocketClient::onRun() {
	int r = this->_session->create(SESSION_TYPE::Client, this->_processor, -1, EV_READ | EV_PERSIST);
	if (r == 0)
	{
		this->_processor->dispatch();
	}
	else {
		LOGFMTD("SocketClient::onRun() r = %d", r);
	}

	this->destroy();

	return 0;
}

int SocketClient::onEnd() {
	return 0;
}