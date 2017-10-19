#ifndef __SocketTimer_H__
#define __SocketTimer_H__

#include "common_type.h"
#include "socket_object.h"

#include <unordered_map>
#include <functional>

class SocketProcessor;

struct TIMER_ITEM;

typedef std::function<bool(int tid, unsigned int tms)> TimerCallback;

class ISocketTimer
{
	friend class SocketTimerCore;

public:
	typedef std::unordered_map<lw_int32, TIMER_ITEM*> TIMERS;
	typedef std::unordered_map<lw_int32, TIMER_ITEM*>::iterator iterator;
	typedef std::unordered_map<lw_int32, TIMER_ITEM*>::const_iterator const_iterator;

public:
	virtual ~ISocketTimer() {}

public:
	virtual int create(SocketProcessor* processor) = 0;
	virtual void destroy() = 0;

public:
	virtual int add(int tid, int tms, const TimerCallback& func) = 0;
	virtual void remove(int tid) = 0;

protected:
	virtual void _timer_cb(TIMER_ITEM* timer) = 0;
};

class SocketTimer : public SocketObject
{
public:
	SocketTimer();
	virtual ~SocketTimer();

public:
	int create(SocketProcessor* processor);
	void destroy();

public:
	int add(int tid, int tms, TimerCallback func);
	void remove(int tid);

public:
	std::string debug() override;

private:
	ISocketTimer* _timer;
};

#endif // !__SocketTimer_H__
