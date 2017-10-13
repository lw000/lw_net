#include "socket_listener.h"

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event_compat.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>

#include "common_marco.h"
#include "socket_config.h"
#include "socket_processor.h"

#include <type_traits>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

static void __listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int, void *);
static void __listener_error_cb(struct evconnlistener *, void *);

static void __listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *userdata)
{
	SocketListener * server = (SocketListener*)userdata;
	server->listener_cb(listener, fd, sa, socklen);
}

static void __listener_error_cb(struct evconnlistener * listener, void * userdata)
{
	SocketListener * server = (SocketListener*)userdata;
	server->listener_error_cb(listener);
}

SocketListener::SocketListener() : _listener(nullptr)
{

}

SocketListener::~SocketListener()
{

}

bool SocketListener::create(SocketProcessor* processor, SocketConfig* config)
{	
	this->_config = config;
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	std::string host = this->_config->getHost();
	if (host.empty()) {
		sin.sin_addr.s_addr = htonl(0);
	}
	else {
		sin.sin_addr.s_addr = inet_addr(host.c_str());
	}

	int port = this->_config->getPort();
	if (port > 0) {
		sin.sin_port = htons(port);
	}
	else {
		sin.sin_port = htons(0);
	}

	//inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr.s_addr);
	this->_listener = evconnlistener_new_bind(processor->getBase(), ::__listener_cb, this,
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE, -1, (struct sockaddr*)&sin, sizeof(sin));

	if (this->_listener != nullptr)
	{
		evconnlistener_set_error_cb(this->_listener, __listener_error_cb);
		return true;
	}

	return false;
}

void SocketListener::destroy()
{
	if (this->_config != nullptr) {
		SAFE_DELETE(this->_config);
	}

	if (this->_listener != nullptr)
	{
		evconnlistener_free(this->_listener);
		this->_listener = nullptr;
	}
}

void SocketListener::set_listener_cb(std::function<void(evutil_socket_t fd, struct sockaddr *sa, int socklen)> func)
{
	if (func != nullptr) {
		this->listener_func = func;
	}
}

void SocketListener::set_listener_errorcb(std::function<void(void * userdata, int er)> func)
{
	if (func != nullptr) {
		this->listener_error_func = func;
	}
}

std::string SocketListener::debug()
{
	return std::string("SocketListener");
}

void SocketListener::listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen)
{
	struct event_base *base = evconnlistener_get_base(listener);

	if (this->listener_func != nullptr) {
		this->listener_func(fd, sa, socklen);
	}
}

void SocketListener::listener_error_cb(struct evconnlistener * listener)
{
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	if (this->listener_error_func != nullptr) {
		this->listener_error_func(nullptr, err);
	}
}
