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

////////////////////////////////////////////////////////////////////////////////////////////////////

SocketClient::SocketClient() : _session(nullptr)
{
	_enable_heart_beat = false;
	_heart_beat_tms = 30000;
	this->_processor = new SocketProcessor();
	this->_timer = new SocketTimer;
}

SocketClient::~SocketClient()
{
	SAFE_DELETE(this->_timer);
	SAFE_DELETE(this->_session);
	SAFE_DELETE(this->_processor);
}

bool SocketClient::create(SocketConfig* conf)
{						
	bool ret = this->_processor->create(false);
	if (ret) {
		int c = 0;
		c = this->_timer->create(this->_processor);

		this->_session = new ClientSession(conf);

		this->_session->connectedHandler = [this](SocketSession* session) {
 			
			addTimer(0, _heart_beat_tms, [this](int tid, unsigned int tms) -> bool {
				this->_session->sendData(NET_HEART_BEAT_PING, NULL, 0);
 				return true;
 			});
			
			this->connectedHandler(session);
		};

		this->_session->disConnectHandler = [this](SocketSession* session) {

			this->disConnectHandler(session);
		};

		this->_session->timeoutHandler = [this](SocketSession* session){

			this->timeoutHandler(session);
		}; 

		this->_session->errorHandler = [this](SocketSession* session) {

			this->errorHandler(session);
		}; 

		this->_session->parseHandler = [this](SocketSession* session, lw_int32 cmd,
			lw_char8* buf, lw_int32 bufsize) {

			if (cmd == NET_HEART_BEAT_PONG) {
				LOGFMTD("NET_HEART_BEAT_PONG: %d", NET_HEART_BEAT_PONG);
				return;
			}

			this->parseHandler(session, cmd, buf, bufsize);
		};

		this->start();
	}

	return true;
}

void SocketClient::destroy()
{
	if (this->_timer != nullptr)
	{
		this->_timer->destroy();
	}

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

void SocketClient::setAutoHeartBeat(int tms) {
	_enable_heart_beat = true;
	_heart_beat_tms = tms;
}

int SocketClient::close()
{
	return this->_processor->loopexit();
}

ClientSession* SocketClient::getSession()
{
	return this->_session;
}

void SocketClient::addTimer(int tid, unsigned int tms, TimerCallback func) {
	int c = this->_timer->add(tid, tms, func);
}

void SocketClient::removeTimer(int tid) {
	this->_timer->remove(tid);
}

int SocketClient::onStart() {

	return 0;
}

int SocketClient::onRun() {
	int r = this->_session->create(this->_processor);
	if (r == 0)
	{
		int r = this->_processor->dispatch();
		LOGFMTD("dispatch r = %d", r);
	}

	LOGFMTD("SocketClient::onRun() r = %d", r);

	return 0;
}

int SocketClient::onEnd() {
	
	this->destroy();

	return 0;
}