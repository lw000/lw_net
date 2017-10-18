#include "server_session.h"

#include "common_marco.h"

#include "socket_config.h"
#include "socket_processor.h"

#include "log4z.h"

/////////////////////////////////////////////////////////////////////////////////////////

ServerSession::ServerSession(SocketConfig* conf) : SocketSession(conf) {

}

ServerSession::~ServerSession() {
	
}

int ServerSession::create(SocketProcessor* processor, evutil_socket_t fd) {
	int c = SocketSession::create(SESSION_TYPE::server, processor, fd);
	return c;
}

void ServerSession::destroy() {
	
	SocketSession::destroy();
}