#include "server_session.h"

#include "common_marco.h"

#include "socket_config.h"
#include "socket_processor.h"

#include "log4z.h"

/////////////////////////////////////////////////////////////////////////////////////////

ServerSession::ServerSession() {

}

ServerSession::~ServerSession() {
	this->destroy();
}

int ServerSession::create(SocketProcessor* processor, SocketConfig* conf, evutil_socket_t fd) {
	int c = SocketSession::create(SESSION_TYPE::server, processor, conf, fd);
	return c;
}

void ServerSession::destroy() {
	
	SocketSession::destroy();
}