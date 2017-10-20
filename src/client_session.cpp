#include "client_session.h"

#include "common_marco.h"

#include "socket_config.h"
#include "socket_processor.h"

#include "log4z.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
ClientSession::ClientSession() {
}

ClientSession::~ClientSession() {

}

int ClientSession::create(SocketProcessor* processor, SocketConfig* conf) {
	int c = SocketSession::create(SESSION_TYPE::client, processor, conf);
	return c;
}

void ClientSession::destroy() {

	SocketSession::destroy();
}