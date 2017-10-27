#include "socket_timer.h"

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

#if defined(WIN32) || defined(_WIN32)
	#include <winsock2.h>
	#include <mmsystem.h>
	#pragma comment(lib, "Winmm.lib")
#else
	#include <arpa/inet.h>
#endif // WIN32

#include "socket_processor.h"
#include "lock.h"

struct TIMER_ITEM
{
	ISocketTimer* owner;
	unsigned int begin;
	unsigned int end;

	int tms;		// 时间（毫秒）
	int real_tid;	// 内核定时器ID
	struct event* ev;	// 内核定时器事件
	int tid;		// 用户定时器ID

	TIMER_ITEM(ISocketTimer* owner) : owner(owner), real_tid(-1), tms(-1), tid(-1),begin(0), end(0)
	{
		ev = new struct event;
	}

	~TIMER_ITEM()
	{
#ifndef _WIN32
		delete ev;
#endif	// !_WIN32
	}

	bool operator == (const TIMER_ITEM& item)
	{
		return (this->tid == item.tid) && (this->tms == item.tms) ? true : false;
	}
};

struct timer_item_hash_func  // hash函数  
{
	size_t operator()(const TIMER_ITEM& item) const
	{
		return item.tid;
	}
};

struct timer_item_cmp_fun // 比较函数 ==  
{
	bool operator()(const TIMER_ITEM& item1, const TIMER_ITEM& item2) const
	{
		return (item1.tid == item2.tid) && (item1.tms == item2.tms) ? true : false;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////

class SocketTimerCore
{
public:
#ifdef _WIN32
	static void CALLBACK __timer_cb_win32__(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		TIMER_ITEM *pCurrentTimer = (TIMER_ITEM*)dwUser;
		if (pCurrentTimer != nullptr)
		{
			pCurrentTimer->owner->_timer_cb(pCurrentTimer);
		}
	}
#endif

	static void  __timer_cb__(evutil_socket_t fd, short event, void *arg)
	{
		TIMER_ITEM *pCurrentTimer = (TIMER_ITEM*)arg;
		if (pCurrentTimer != nullptr)
		{
			pCurrentTimer->owner->_timer_cb(pCurrentTimer);
		}
	}

};

//////////////////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

class TimerWin32 : public Object, public ISocketTimer
{
public:
	TimerWin32();
	virtual ~TimerWin32();

public:
	virtual int create(SocketProcessor* base = nullptr) override;
	virtual void destroy() override;

public:
	virtual int add(int tid, int tms, const TimerCallback& func) override;
	virtual void remove(int tid) override;

public:
	std::string debug() override;

protected:
	virtual void _timer_cb(TIMER_ITEM* timer) override;

private:
	int __start(int tid, int tms, const TimerCallback& func, unsigned int fuEvent);

private:
	TIMERS _timers;
	TimerCallback _onTimer;
};

TimerWin32::TimerWin32()
{
	TIMECAPS caps;
	UINT r = ::timeGetDevCaps(&caps, sizeof(caps));
	UINT r1 = ::timeBeginPeriod(10);
}

TimerWin32::~TimerWin32()
{
	TIMERS::iterator iter = _timers.begin();
	while (iter != _timers.end())
	{
		TIMER_ITEM* ptimer = iter->second;

		UINT err = 0;
		err = ::timeKillEvent(ptimer->tid);

		delete ptimer;

		iter = _timers.erase(iter);
	}

	UINT r = ::timeEndPeriod(10);
}

int TimerWin32::create(SocketProcessor* base)
{
	return true;
}

void TimerWin32::destroy()
{
}

std::string TimerWin32::debug()
{
	return std::string("TimerWin32");
}

int TimerWin32::add(int tid, int tms, const TimerCallback& func)
{
	int r = this->__start(tid, tms, func, TIME_PERIODIC);
	return r;
}

void TimerWin32::remove(int tid)
{
	if (tid < 0) return;

	TIMER_ITEM* ptimer = nullptr;
	if (!this->_timers.empty()) {
		auto v = this->_timers.find(tid);
		if (v != this->_timers.end()) {
			ptimer = v->second;
		}
	}

	if (ptimer == nullptr) return;

	UINT err = 0;
	err = ::timeKillEvent(ptimer->real_tid);
	if (err == TIMERR_NOERROR)
	{
		TIMERS::iterator iter = _timers.begin();
		while (iter != _timers.end())
		{
			TIMER_ITEM* ptimer = iter->second;
			if (ptimer->tid == tid)
			{
				delete ptimer;
				ptimer = nullptr;

				iter = _timers.erase(iter);
				break;
			}
			else
			{
				++iter;
			}
		}
	}
}

int TimerWin32::__start(int tid, int tms, const TimerCallback& func, unsigned int fuEvent)
{
	if (tid < 0) return -1;

	this->_onTimer = func;

	TIMER_ITEM* ptimer = nullptr;
	if (!this->_timers.empty()) {
		auto v = this->_timers.find(tid);
		if (v != this->_timers.end()) {
			ptimer = v->second;
		}
	}

	if (ptimer != nullptr)
	{
		UINT err = 0;
		err = ::timeKillEvent(ptimer->real_tid);
	}
	
	ptimer = new TIMER_ITEM(this);

	UINT real_tid = 0;
	real_tid = ::timeSetEvent(tms, 10, SocketTimerCore::__timer_cb_win32__, (DWORD_PTR)ptimer, fuEvent);
	if (tid != 0)
	{
		ptimer->tms = tms;
		ptimer->tid = tid;
		ptimer->begin = ::timeGetTime();
		ptimer->end = ptimer->begin;
		ptimer->real_tid = real_tid;
		
		this->_timers.insert(std::pair<lw_int32, TIMER_ITEM*>(tid, ptimer));
	}
	else
	{
		UINT err = 0;
		err = ::timeKillEvent(ptimer->real_tid);
		delete ptimer;
		ptimer = nullptr;
	}

	return tid;
}

void TimerWin32::_timer_cb(TIMER_ITEM* timer)
{
	timer->end = ::timeGetTime();
	unsigned int new_begin = timer->begin;
	timer->begin = timer->end;
	bool r = this->_onTimer(timer->tid, timer->end - new_begin);
	if (!r)
	{
		this->remove(timer->tid);
	}
}

#endif

class TimerLinux : public Object, public ISocketTimer
{
public:
	TimerLinux();
	virtual ~TimerLinux();

public:
	virtual int create(SocketProcessor* processor = nullptr) override;
	virtual void destroy() override;

public:
	virtual int add(int tid, int tms, const TimerCallback& func) override;
	virtual void remove(int tid) override;

public:
	std::string debug() override;

protected:
	virtual void _timer_cb(TIMER_ITEM* timer) override;

private:
	SocketProcessor* _processor;
	TIMERS _timers;
	TimerCallback _on_timer;
	lw_fast_mutex _lock;
};

//////////////////////////////////////////////////////////////////////////////////////////

TimerLinux::TimerLinux() : _processor(nullptr)
{

}

TimerLinux::~TimerLinux()
{

}

int TimerLinux::create(SocketProcessor* processor)
{
	this->_processor = processor;
	return true;
}

void TimerLinux::destroy()
{
	{
		lw_fast_lock_guard l(_lock);
		TIMERS::iterator iter = _timers.begin();
		while (iter != _timers.end())
		{
			TIMER_ITEM* pTimer = iter->second;

			event_del(pTimer->ev);

			delete pTimer;

			iter = _timers.erase(iter);
		}
	}
}

std::string TimerLinux::debug()
{
	return std::string("TimerLinux");
}

int TimerLinux::add(int tid, int tms, const TimerCallback& func)
{
	if (tid < 0) return -1;
	int r = 0;

	{
		lw_fast_lock_guard l(_lock);

		this->_on_timer = func;

		TIMER_ITEM* ptimer = nullptr;
		if (!this->_timers.empty()) {
			auto v = this->_timers.find(tid);
			if (v != this->_timers.end()) {
				ptimer = v->second;
			}		
		}

		if (ptimer == nullptr)
		{
			ptimer = new TIMER_ITEM(this);
			this->_timers.insert(std::pair<lw_int32, TIMER_ITEM*>(tid, ptimer));
		}
		else
		{
			int r = 0;
			r = event_del(ptimer->ev);
		}

		ptimer->tid = tid;
		ptimer->tms = tms;

		// 设置定时器
		int r = 0;
		/* 定时器只执行一次，每次需要添加*/
		//r = event_assign(ptimer->ev, this->_base, -1, 0, TimerCore::__timer_cb__, ptimer);

		/* 定时器无限次执行*/
		r = event_assign(ptimer->ev, this->_processor->getBase(), -1, EV_PERSIST, SocketTimerCore::__timer_cb__, ptimer);

		struct timeval tv;
		evutil_timerclear(&tv);
		tv.tv_sec = tms / 1000;
		tv.tv_usec = (tms % 1000) * 1000;

		r = event_add(ptimer->ev, &tv);
	}

	return r;
}

void TimerLinux::remove(int tid)
{
	if (tid < 0) return;

	{
		lw_fast_lock_guard l(_lock);
		
		TIMER_ITEM* ptimer = nullptr;
		if (!this->_timers.empty()) {
			auto v = this->_timers.find(tid);
			if (v != this->_timers.end()) {
				ptimer = v->second;
			}
		}

		if (ptimer != nullptr)
		{
			TIMERS::iterator iter = _timers.begin();
			while (iter != _timers.end())
			{
				TIMER_ITEM* timer = iter->second;
				if (timer->tid == tid)
				{
					int r = 0;
					r = event_del(ptimer->ev);
					delete timer;

					iter = _timers.erase(iter);

					break;
				}
				++iter;
			}
		}
	}	
}

void TimerLinux::_timer_cb(TIMER_ITEM* timer)
{
	TIMER_ITEM* ptimer = timer;
	bool r = this->_on_timer(ptimer->tid, 0);
	if (!r)
	{
		this->remove(ptimer->tid);
	}
}

SocketTimer::SocketTimer() : _timer(nullptr)
{
#ifdef _WIN32
	//_timer = new TimerWin32;
	_timer = new TimerLinux;
#else
	_timer = new TimerLinux;
#endif
}

SocketTimer::~SocketTimer()
{
	
}

int SocketTimer::create(SocketProcessor* processor)
{
	return _timer->create(processor);
}

void SocketTimer::destroy()
{
	_timer->destroy();
}

int SocketTimer::add(int tid, int tms, TimerCallback func)
{
	return _timer->add(tid, tms, func);
}

void SocketTimer::remove(int tid)
{
	_timer->remove(tid);
}

std::string SocketTimer::debug()
{
	return std::string("Timer");
}
