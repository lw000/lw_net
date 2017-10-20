#include "socket_processor.h"

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event_compat.h>
#include <event2/util.h>
#include <event2/thread.h>


#include "socket_timer.h"
#include "common_marco.h"

void SocketProcessor::use_threads()
{
#if defined(__WIN32) || defined(WIN32)
	evthread_use_windows_threads();
#else
	evthread_use_pthreads();
#endif
}

SocketProcessor::SocketProcessor() : _base(nullptr)
{
	this->_timer = new SocketTimer;
}

SocketProcessor::~SocketProcessor()
{
	SAFE_DELETE(this->_timer);

	if (this->_base != nullptr)
	{
		event_base_free(this->_base);
		this->_base = nullptr;
	}
}

bool SocketProcessor::create(bool enableServer) {
	
	if (this->_base == nullptr)
	{

#ifdef WIN32
		if (enableServer) {
			struct event_config *cfg = event_config_new();
			event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
			if (cfg)
			{
				this->_base = event_base_new_with_config(cfg);
				event_config_free(cfg);
			}
		}
		else {
			this->_base = event_base_new();
		}
#else
		this->_base = event_base_new();
#endif
		int c = 0;
		c = this->_timer->create(this);
	}

	return true;
}

void SocketProcessor::destroy() {
	if (this->_timer != nullptr)
	{
		this->_timer->destroy();
	}
}

struct event_base* SocketProcessor::getBase()
{ 
	return this->_base;
}

int SocketProcessor::addTimer(int tid, unsigned int tms, const TimerCallback& func) {
	int c = this->_timer->add(tid, tms, func);
	return c;
}

void SocketProcessor::removeTimer(int tid) {
	this->_timer->remove(tid);
}

int SocketProcessor::dispatch()
{
	//EVLOOP_NO_EXIT_ON_EMPTY
	//int r = event_base_dispatch(this->_base);
	int r = event_base_loop(this->_base, EVLOOP_NO_EXIT_ON_EMPTY);
	return r;
}

int SocketProcessor::loopbreak()
{
	int r = event_base_loopbreak(this->_base);
	return r;
}

int SocketProcessor::loopexit(int tms)
{
	struct timeval delay = { 0, tms*1000};
	int r = event_base_loopexit(this->_base, &delay);
	return r;
}

std::string SocketProcessor::debug()
{
	char buf[256];
	if (this->_base == nullptr)
	{
		sprintf(buf, "[SocketProcessor: %X]", this);
	}
	else
	{
		sprintf(buf, "[SocketProcessor: %X, %X]", this, this->_base);
	}
	return std::string(buf);
}
