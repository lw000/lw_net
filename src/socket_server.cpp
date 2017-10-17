#include "socket_server.h"

#include <assert.h>
#include <signal.h>
#include <vector>
#include <algorithm>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/thread.h>

#ifdef WIN32
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif // _WIN32

#include "common_marco.h"
#include "socket_session.h"
#include "socket_timer.h"
#include "socket_processor.h"
#include "socket_listener.h"
#include "socket_config.h"

#include <log4z.h>

SocketServer::SocketServer() : _onFunc(nullptr)
{
	this->_processor = new SocketProcessor;
	this->_listener = new SocketListener;
	this->_timer = new SocketTimer;
}

SocketServer::~SocketServer()
{
	SAFE_DELETE(this->_timer);
	SAFE_DELETE(this->_listener);
	SAFE_DELETE(this->_processor);
}

bool SocketServer::create(SocketConfig* config)
{
	bool ret = this->_processor->create(true);
	if (ret) {
		ret = this->_listener->create(_processor, config);
		if (ret) {
			int c = this->_timer->create(_processor);
		}
	}
	
	return ret;
}

void SocketServer::destroy()
{
	if (this->_listener != nullptr)
	{
		this->_listener->destroy();
	}
}

void SocketServer::addTimer(int tid, unsigned int tms, TimerCallback func) {
	int c = this->_timer->add(tid, tms, func);
}

void SocketServer::removeTimer(int tid) {
	this->_timer->remove(tid);
}

int SocketServer::close()
{
	return this->_processor->loopexit();
}

std::string SocketServer::debug()
{
	return std::string("SocketServer");
}

lw_int32 SocketServer::serv(std::function<void(lw_int32 what)> func)
{
	if (nullptr == func) {
		LOGD("func is null");
		return -1;
	}

	if (nullptr == _listener) {
		LOGD("_listener is null");
		return -2;
	}

	this->_onFunc = func;

	_listener->listenerCB([this](evutil_socket_t fd, struct sockaddr *sa, int socklen) {
		SocketSession* pSession = new SocketSession(new SocketConfig);
		pSession->connectedHandler = this->connectedHandler;
		pSession->disConnectHandler = this->disConnectHandler;
		pSession->timeoutHandler = this->timeoutHandler;
		pSession->errorHandler = this->errorHandler;
		pSession->parseHandler = this->parseHandler;

		int r = pSession->create(SESSION_TYPE::server, this->_processor, fd);
		if (r == 0)
		{
			char hostBuf[NI_MAXHOST];
			char portBuf[64];
			getnameinfo(sa, socklen, hostBuf, sizeof(hostBuf), portBuf, sizeof(portBuf), NI_NUMERICHOST | NI_NUMERICSERV);

			pSession->getConf()->setHost(hostBuf);
			pSession->getConf()->setPort(std::stoi(portBuf));

			if (this->listenHandler != nullptr) {
				this->listenHandler(pSession);
			}
		}
		else
		{
			SAFE_DELETE(pSession);
			_processor->loopbreak();
			LOGD("error constructing SocketSession!");
		}
	});

	_listener->listenerErrorCB([this](void * userdata, int er) {
		LOGFMTD("got an error %d (%s) on the listener. shutting down.\n", er, evutil_socket_error_to_string(er));
		this->_processor->loopexit();
	});

	if (this->_onFunc != nullptr)
	{
		this->_onFunc(0);
	}

	this->start();

	return 0;
}

int SocketServer::onStart() {
	return 0;
}

int SocketServer::onRun() {

	int ret = _processor->dispatch();
	LOGFMTD("SocketServer::onRun ret = %d", ret);
	
	return 0;
}

int SocketServer::onEnd() {

	this->destroy();

	return 0;
}
