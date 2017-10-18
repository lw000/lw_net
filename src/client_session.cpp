#include "client_session.h"

#include "common_marco.h"

#include "socket_config.h"
#include "socket_processor.h"

#include "log4z.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
ClientSession::ClientSession(SocketConfig* conf) : SocketSession(conf) {
}

ClientSession::~ClientSession() {

}

int ClientSession::create(SocketProcessor* processor) {
	int c = SocketSession::create(SESSION_TYPE::client, processor);
	return c;
}

void ClientSession::destroy() {

	SocketSession::destroy();
}