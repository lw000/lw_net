#include "socket_heartbeat.h"

#include "net_package.h"

#include "socket_session.h"
#include "socket_processor.h"

#include "log4z.h"

SocketHeartbeat::SocketHeartbeat(SocketSession* session) : _session(session), _enabled(false)
{
	this->_keepalivetime = 10000;
	this->_keepaliveinterval = 3;
	this->_processor = nullptr;

	this->onConnectedHandler = [this](SocketSession* session)
	{
		if (!this->_enabled) return;

		this->_processor->addTimer(0, _keepalivetime, [this](int tid, unsigned int tms) -> bool {
			this->ping();
			return true;
		});
	};

	this->onDisconnectHandler = [this](SocketSession* session)
	{
		if (!this->_enabled) return;

		this->_processor->removeTimer(0);
	};

	this->onTimeoutHandler = [this](SocketSession* session)
	{
		if (!this->_enabled) return;

		this->_processor->removeTimer(0);
	};

	this->onErrorHandler = [this](SocketSession* session)
	{
		if (!this->_enabled) return;

		this->_processor->removeTimer(0);
	};

	this->onDataParseHandler = [this](SocketSession* session, lw_int32 cmd, lw_char8* buf, lw_int32 bufsize) -> int
	{
		if (!this->_enabled) return 0;

		if (cmd == NET_HEART_BEAT_PING) {
			LOGFMTD("NET_HEART_BEAT_PING: %d", NET_HEART_BEAT_PING);

			session->sendData(NET_HEART_BEAT_PONG, NULL, 0);

			return -1;	// ping package
		}

		if (cmd == NET_HEART_BEAT_PONG) {
			LOGFMTD("NET_HEART_BEAT_PONG: %d", NET_HEART_BEAT_PONG);

			return -2;	// pong package
		}

		return 0;
	};
}

SocketHeartbeat::~SocketHeartbeat()
{

}

bool SocketHeartbeat::create(SocketProcessor* processor)
{
	this->_processor = processor;

	return true;
}

void SocketHeartbeat::destroy()
{
	
}

void SocketHeartbeat::setAutoHeartBeat(int tms) {
	this->_enabled = true;
	this->_keepalivetime = tms;
}

bool SocketHeartbeat::enableHeartBeat() const {
	return this->_enabled;
}

void SocketHeartbeat::ping() {
	_session->sendData(NET_HEART_BEAT_PING, NULL, 0);
}

void SocketHeartbeat::pong() {
	_session->sendData(NET_HEART_BEAT_PONG, NULL, 0);
}
