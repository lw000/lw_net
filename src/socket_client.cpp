#include "socket_client.h"

#include <stdio.h>
#include <assert.h>

#include "event2/event.h"

#include "net_iobuffer.h"
#include "socket_config.h"
#include "socket_hanlder.h"
#include "socket_processor.h"
#include "socket_session.h"

#include "common_marco.h"

#include "log4z.h"
using namespace zsummer::log4z;

////////////////////////////////////////////////////////////////////////////////////////////////////

SocketClient::SocketClient() : _processor(nullptr), _session(nullptr)
{
	this->_processor = new SocketProcessor();
}

SocketClient::~SocketClient()
{
	SAFE_DELETE(this->_session);
	SAFE_DELETE(this->_processor);
}

bool SocketClient::create(AbstractSocketClientHandler* handler, SocketConfig* config)
{						
	bool ret = this->_processor->create(false);
	if (ret) {
		this->_session = new SocketSession(handler, config);
		this->_session->connectedHandler = this->connectedHandler;
		this->_session->disConnectHandler = this->disConnectHandler;
		this->_session->timeoutHandler = this->timeoutHandler;
		this->_session->errorHandler = this->errorHandler;
	}

	return true;
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

int SocketClient::open()
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
	int r = this->_session->create(SESSION_TYPE::Client, this->_processor);
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