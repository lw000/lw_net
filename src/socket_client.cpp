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

			this->_session->onConnectedHandler = [this](SocketSession* session) {
				this->onConnectedHandler(session);
			};

			this->_session->onDisconnectHandler = [this](SocketSession* session) {
				this->onDisconnectHandler(session);
			};

			this->_session->onTimeoutHandler = [this](SocketSession* session){
				this->onTimeoutHandler(session);
			};

			this->_session->onErrorHandler = [this](SocketSession* session) {
				this->onErrorHandler(session);
			};

			this->_session->onDataParseHandler = [this](SocketSession* session, lw_int32 cmd,
				lw_char8* buf, lw_int32 bufsize) -> int {
				int c = this->onDataParseHandler(session, cmd, buf, bufsize);
				return c;
			};

			this->start();
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

void SocketClient::setAutoHeartBeat(int tms) {
	this->_session->setAutoHeartBeat(tms);
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