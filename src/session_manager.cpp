#include "session_manager.h"

#include "socket_session.h"

SessionManager::SessionManager()
{
}

SessionManager::~SessionManager()
{
	{
		std::list<SocketSession*>::iterator iter = _live.begin();
		for (; iter != _live.end(); ++iter)
		{
			SocketSession* pUserinfo = (*iter);
			delete pUserinfo;
		}
	}

	{
		std::list<SocketSession*>::iterator iter = _die.begin();
		for (; iter != _die.end(); ++iter)
		{
			SocketSession* pUserinfo = (*iter);
			delete pUserinfo;
		}
	}
}

SocketSession* SessionManager::add(const SocketSession* session)
{
	SocketSession* pSession = nullptr;
	{
		//std::lock_guard < std::mutex > lock(_m);
		lw_lock_guard l(&_m);
		std::list<SocketSession*>::iterator iter = _live.begin();
		for (; iter != _live.end(); ++iter)
		{
			if (session == *iter)
			{
				pSession = *iter; break;
			}
		}

		if (pSession == nullptr)
		{
			{
				if (!_die.empty())
				{
					pSession = _die.front();
					_die.pop_front();
				}
			}

			if (pSession == NULL)
			{
				pSession = new SocketSession(nullptr, nullptr, nullptr);
			}

			*pSession = *session;

			_live.push_back(const_cast<SocketSession*>(pSession));
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
		//std::lock_guard < std::mutex > lock(_m);
		lw_lock_guard l(&_m);
		SocketSession* pSession = nullptr;
		std::list<SocketSession*>::iterator iter = _live.begin();
		for (; iter != _live.end(); ++iter)
		{
			if (pSession == (*iter))
			{
				pSession = *iter;
				_live.erase(iter);
				break;
			}
		}

		if (pSession != nullptr)
		{
			_die.push_back(pSession);
		}
	}
}

void SessionManager::restoreCache()
{
	{
		//std::lock_guard < std::mutex > lock(_m);
		lw_lock_guard l(&_m);
		{
			std::list<SocketSession*>::iterator iter = _die.begin();
			for (; iter != _die.end(); ++iter)
			{
				SocketSession* p = (*iter);
				delete p;
			}

			std::list<SocketSession*>().swap(_die);
		}
	}
}

