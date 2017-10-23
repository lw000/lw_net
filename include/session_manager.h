#ifndef __SessionManager_h__
#define __SessionManager_h__

#include <list>
#include "lock.h"

class SocketSession;

class SessionManager
{
public:
	SessionManager();
	~SessionManager();

public:
	bool create();
	void destroy();

public:
	SocketSession* add(const SocketSession* session);
	void remove(const SocketSession* session);

public:
	void restoreCache();

private:
	lw_fast_mutex _lock;
	std::list<SocketSession*> _alive;
	std::list<SocketSession*> _free;
};

#endif	// !__SessionManager_h__
