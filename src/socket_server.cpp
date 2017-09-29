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

#include "net_core.h"
#include "common_marco.h"
#include "socket_session.h"
#include "socket_timer.h"
#include "socket_processor.h"
#include "socket_listener.h"
#include "socket_config.h"

SocketServer::SocketServer() : _onFunc(nullptr)
{
	this->_processor = new SocketProcessor;
	this->_core = new NetCore;
	this->_timer = new Timer;
	this->_listener = new SocketListener;
}

SocketServer::~SocketServer()
{
	SAFE_DELETE(this->_timer);
	SAFE_DELETE(this->_core);
	SAFE_DELETE(this->_listener);
	SAFE_DELETE(this->_processor);
}

bool SocketServer::create(AbstractSocketServerHandler* handler)
{
	this->_handler = handler;

	bool r = this->_processor->create(true);
	if (r)
	{
		this->_timer->create(this->_processor);
	}
	return r;
}

void SocketServer::destroy()
{
	if (_timer != nullptr)
	{
		this->_timer->destroy();
	}

	if (this->_processor != nullptr)
	{
		this->_processor->destroy();
	}
}

std::string SocketServer::debug()
{
	return std::string("SocketServer");
}

lw_int32 SocketServer::listen(SocketConfig* config, std::function<void(lw_int32 what)> func)
{
 	if (nullptr == func) return -1;

	this->_onFunc = func;

	bool ret = _listener->create(_processor, config);
	if (ret)
	{
		_listener->set_listener_cb([this](evutil_socket_t fd, struct sockaddr *sa, int socklen) {
			SocketConfig* config = new SocketConfig;
			SocketSession* pSession = new SocketSession(this->_handler, this->_core, config);
			int r = pSession->create(SESSION_TYPE::Server, this->_processor, fd, EV_READ | EV_WRITE);
			if (r == 0)
			{
				{
					char hostBuf[NI_MAXHOST];
					char portBuf[64];
					getnameinfo(sa, socklen, hostBuf, sizeof(hostBuf), portBuf, sizeof(portBuf), NI_NUMERICHOST | NI_NUMERICSERV);

					config->setHost(hostBuf);
					config->setPort(std::stoi(portBuf));
				}
				this->_handler->onListener(pSession);
			}
			else
			{
				SAFE_DELETE(pSession);
				_processor->loopbreak();
				fprintf(stderr, "error constructing SocketSession!");
			}
		});

		_listener->set_listener_errorcb([this](void * userdata, int er) {
			int err = EVUTIL_SOCKET_ERROR();
			//			printf("got an error %d (%s) on the listener. shutting down.\n", err, evutil_socket_error_to_string(err));

			this->_processor->loopexit();
		});

		// 初始化完成定时器
		{
			this->_timer->start(100, 1000, [this](int tid, unsigned int tms) -> bool
			{
				if (this->_onFunc != nullptr)
				{
					this->_onFunc(0);
				}

				return false;
			});
		}

		this->start();
	}

	return 0;
}

int SocketServer::onStart() {
	return 0;
}

int SocketServer::onRun() {

	int r = _processor->dispatch();

	this->_listener->destroy();

	this->destroy();

	return 0;
}

int SocketServer::onEnd() {
	return 0;
}

int SocketServer::loopbreak()
{
	return this->_processor->loopbreak();
}

int SocketServer::loopexit()
{
	return this->_processor->loopexit();
}
