#include "session_manager.h"
#include "socket_session.h"
#include "socket_config.h"

SessionManager::SessionManager()
{
}

SessionManager::~SessionManager()
{
	this->destroy();
}

bool SessionManager::create() {

	return true;
}

void SessionManager::destroy() {
	{
		lw_lock_guard l(&_lock);
		{
			std::list<SocketSession*>::iterator iter = _alive.begin();
			for (; iter != _alive.end(); ++iter)
			{
				SocketSession* pUserinfo = (*iter);
				delete pUserinfo;
			}
		}

	{
		std::list<SocketSession*>::iterator iter = _free.begin();
		for (; iter != _free.end(); ++iter)
		{
			SocketSession* pUserinfo = (*iter);
			delete pUserinfo;
		}
	}
	}
}

SocketSession* SessionManager::add(const SocketSession* session)
{
	SocketSession* pSession = nullptr;
	{
		lw_lock_guard l(&_lock);
		std::list<SocketSession*>::iterator iter = _alive.begin();
		for (; iter != _alive.end(); ++iter)
		{
			if (session == *iter)
			{
				pSession = *iter; break;
			}
		}

		if (pSession == nullptr)
		{
			{
				if (!_free.empty())
				{
					pSession = _free.front();
					_free.pop_front();
				}
			}

			if (pSession == NULL)
			{
				pSession = new SocketSession(new SocketConfig("", -1));
			}

			*pSession = *session;

			_alive.push_back(const_cast<SocketSession*>(pSession));
		}
		else
		{
			*pSession = *session;
		}
	}

	return pSession;
}

void SessionManager::remove(const SocketSession* session)
{
	{
		lw_lock_guard l(&_lock);
		SocketSession* pSession = nullptr;
		std::list<SocketSession*>::iterator iter = _alive.begin();
		for (; iter != _alive.end(); ++iter)
		{
			if (pSession == (*iter))
			{
				pSession = *iter;
				_alive.erase(iter);
				break;
			}
		}

		if (pSession != nullptr)
		{
			_free.push_back(pSession);
		}
	}
}

void SessionManager::restoreCache()
{
	{
		lw_lock_guard l(&_lock);
		{
			std::list<SocketSession*>::iterator iter = _free.begin();
			for (; iter != _free.end(); ++iter)
			{
				SocketSession* p = (*iter);
				delete p;
			}

			std::list<SocketSession*>().swap(_free);
		}
	}
}

