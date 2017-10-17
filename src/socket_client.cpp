#include "socket_client.h"

#include <stdio.h>
#include <assert.h>

#include "event2/event.h"

#include "net_iobuffer.h"
#include "socket_config.h"
#include "socket_hanlder.h"
#include "socket_processor.h"
#include "socket_session.h"
#include "socket_timer.h"

#include "common_marco.h"

#include "log4z.h"
using namespace zsummer::log4z;

////////////////////////////////////////////////////////////////////////////////////////////////////

SocketClient::SocketClient() : _session(nullptr)
{
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
// 		c = this->_timer->start(1, 10000,
// 			[this](int tid, unsigned int tms) -> bool {
// 			printf("client heart [%d]", tid);
// 			return true;
// 		});
		this->_session = new SocketSession(conf);
		this->_session->connectedHandler = this->connectedHandler;
		this->_session->disConnectHandler = this->disConnectHandler;
		this->_session->timeoutHandler = this->timeoutHandler;
		this->_session->errorHandler = this->errorHandler;
		this->_session->parseHandler = this->parseHandler;

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

int SocketClient::close()
{
	return this->_processor->loopexit();
}

SocketSession* SocketClient::getSession()
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
	int r = this->_session->create(SESSION_TYPE::client, this->_processor);
	if (r == 0)
	{
		this->_processor->dispatch();
	}
	else {
		LOGFMTD("SocketClient::onRun() r = %d", r);
	}

	return 0;
}

int SocketClient::onEnd() {
	
	this->destroy();

	return 0;
}