#ifndef __client_session_h__
#define __client_session_h__

#include <string>

#include "common_type.h"
#include "socket_session.h"

class SocketConfig;

class ClientSession : public SocketSession
{
public:
	ClientSession();
	virtual ~ClientSession();

public:
	int create(SocketProcessor* processor, SocketConfig* conf);
	void destroy();
};


#endif // !__client_session_h__