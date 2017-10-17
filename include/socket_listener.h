#ifndef __listener_object_h__
#define __listener_object_h__

#include <string>

#include "common_type.h"
#include "socket_object.h"

#include <event2/util.h>
#include <functional>

struct evconnlistener;
class SocketConfig;
class SocketProcessor;

class SocketListener : public SocketObject
{
	friend class ListenerCore;

public:
	SocketListener();
	virtual ~SocketListener();

public:
	bool create(SocketProcessor* processor, SocketConfig* config);
	void destroy();

public:
	void listenerCB(std::function<void(evutil_socket_t fd, struct sockaddr *sa, int socklen)> func);
	void listenerErrorCB(std::function<void(void * userdata, int er)> func);

public:
	virtual std::string debug() override;

private:
	void __listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int);
	void __listener_error_cb(struct evconnlistener *);

private:
	SocketConfig* _config;
	struct evconnlistener* _listener;
	std::function<void(evutil_socket_t fd, struct sockaddr *sa, int socklen)> _on_listener_func;
	std::function<void(void * userdata, int err)> _on_listener_error_func;
};


#endif // !__listener_object_h__