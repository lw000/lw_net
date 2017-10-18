#ifndef __server_session_h__
#define __server_session_h__

#include <string>

#include "common_type.h"
#include "socket_session.h"

#include <event2/util.h>

class SocketConfig;

class ServerSession : public SocketSession
{
public:
	int session_id;

public:
	ServerSession(SocketConfig* conf);
	virtual ~ServerSession();

public:
	int create(SocketProcessor* processor, evutil_socket_t fd);
	void destroy();
};


#endif // !__server_session_h__