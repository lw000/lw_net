#include "socket_client.h"

#include <stdio.h>
#include <assert.h>

#include "event2/event.h"

#include "common_marco.h"

#include "net_package.h"
#include "net_iobuffer.h"
#include "socket_config.h"
#include "socket_hanlder.h"
#include "socket_processor.h"
#include "socket_session.h"
#include "client_session.h"
#include "socket_timer.h"

#include "log4z.h"
#include "socket_heartbeat.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

SocketClient::SocketClient() : _session(nullptr)
{
	this->_processor = new SocketProcessor();
}

SocketClient::~SocketClient()
{
	SAFE_DELETE(this->_heartbeat);
	SAFE_DELETE(this->_session);
	SAFE_DELETE(this->_processor);
}

bool SocketClient::create(SocketConfig* conf)
{						
	bool ret = this->_processor->create(false);
	if (ret) {
		this->_session = new ClientSession();
		if (this->_session != nullptr) {
			int r = this->_session->create(this->_processor, conf);
			this->_session->onConnectedHandler = this->onConnectedHandler;
			this->_session->onDisconnectHandler = this->onDisconnectHandler;
			this->_session->onTimeoutHandler = this->onTimeoutHandler;
			this->_session->onErrorHandler = this->onErrorHandler;
			this->_session->onDataParseHandler = this->onDataParseHandler;
		}
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

void SocketClient::startAutoPing(int tms) {
	this->_session->startAutoPing(tms);
}

std::string SocketClient::debug()
{
	char buf[512];
	sprintf(buf, "%s", _session->debug().c_str());
	return std::string(buf);
}

int SocketClient::close()
{
	return this->_processor->loopexit();
}

ClientSession* SocketClient::getSession()
{
	return this->_session;
}

int SocketClient::addTimer(int tid, unsigned int tms, const TimerCallback& func) {
	int c = this->_processor->addTimer(tid, tms, func);
	return c;
}

void SocketClient::removeTimer(int tid) {
	this->_processor->removeTimer(tid);
}

int SocketClient::onStart() {

	return 0;
}

int SocketClient::onRun() {
	int r = this->_processor->dispatch();
	LOGFMTD("dispatch r = %d", r);

	return 0;
}

int SocketClient::onEnd() {
	
	this->destroy();

	return 0;
}