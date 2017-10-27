#include "socket_server.h"

#include <assert.h>
#include <signal.h>
#include <vector>
#include <algorithm>


#ifdef WIN32
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif // _WIN32

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/thread.h>

#include "net_package.h"

#include "common_marco.h"
#include "server_session.h"
#include "socket_timer.h"
#include "socket_processor.h"
#include "socket_listener.h"
#include "socket_config.h"

#include "log4z.h"
#include "socket_heartbeat.h"

SocketServer::SocketServer() : _onFunc(nullptr)
{
	this->_processor = new SocketProcessor;
	this->_listener = new SocketListener;
}

SocketServer::~SocketServer()
{
	SAFE_DELETE(this->_listener);
	SAFE_DELETE(this->_processor);
}

bool SocketServer::create(SocketConfig* conf)
{
	bool ret = this->_processor->create(true);
	if (ret) {
		ret = this->_listener->create(_processor, conf);
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

int SocketServer::addTimer(int tid, unsigned int tms, const TimerCallback& func) {
	int c = this->_processor->addTimer(tid, tms, func);
	return c;
}

void SocketServer::removeTimer(int tid) {
	this->_processor->removeTimer(tid);
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

		if (this->listenHandler != nullptr) {

			ServerSession* pSession = (ServerSession*)this->listenHandler(this->_processor, fd);
			
			pSession->startAutoPing();

			char hostBuf[NI_MAXHOST];
			char portBuf[64];
			getnameinfo(sa, socklen, hostBuf, sizeof(hostBuf), portBuf, sizeof(portBuf), NI_NUMERICHOST | NI_NUMERICSERV);
			pSession->getConf()->setHost(hostBuf);
			pSession->getConf()->setPort(std::stoi(portBuf));

			LOGFMTD("%s", pSession->debug().c_str());
		}

// 		ServerSession* pSession = new ServerSession(new SocketConfig);
// 
// 		pSession->disConnectHandler = [this](SocketSession* session) {
// 			this->disConnectHandler(session);
// 		};
// 
// 		pSession->timeoutHandler = [this](SocketSession* session) {
// 			this->timeoutHandler(session);
// 		};
// 
// 		pSession->errorHandler = [this](SocketSession* session) {
// 			this->errorHandler(session);
// 		};
// 
// 		pSession->parseHandler = [this](SocketSession* session, lw_int32 cmd,
// 			lw_char8* buf, lw_int32 bufsize) {
// 
// 			if (cmd == NET_HEART_BEAT_PING) {
// 				LOGFMTD("NET_HEART_BEAT_PING: %d", NET_HEART_BEAT_PING);
// 				session->sendData(NET_HEART_BEAT_PONG, NULL, 0);
// 				return;
// 			}
// 
// 			this->parseHandler(session, cmd, buf, bufsize);
// 		};
// 
// 		int r = pSession->create(this->_processor, fd);
// 		if (r == 0)
// 		{
// 			char hostBuf[NI_MAXHOST];
// 			char portBuf[64];
// 			getnameinfo(sa, socklen, hostBuf, sizeof(hostBuf), portBuf, sizeof(portBuf), NI_NUMERICHOST | NI_NUMERICSERV);
// 
// 			pSession->getConf()->setHost(hostBuf);
// 			pSession->getConf()->setPort(std::stoi(portBuf));
// 
// 			if (this->listenHandler != nullptr) {
// 
// 				this->listenHandler(this->_processor, fd);
// 			}
// 		}
// 		else
// 		{
// 			pSession->destroy();
// 			SAFE_DELETE(pSession);
// 			LOGD("error constructing ServerSession!");
// 		}

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
