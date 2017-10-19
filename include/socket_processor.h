#ifndef __event_object_h__
#define __event_object_h__

#include <string>

#include "common_type.h"
#include "socket_object.h"
#include "socket_timer.h"

struct event_base;
class SocketTimer;

class SocketProcessor : public SocketObject
{
public:
	static void use_threads();

public:
	SocketProcessor();
	virtual ~SocketProcessor();

public:
	bool create(bool enableServer);
	void destroy();
	
public:
	struct event_base* getBase();

public:
	int addTimer(int tid, unsigned int tms, const TimerCallback& func);
	void removeTimer(int tid);

public:
	int dispatch();

public:
	int loopbreak();
	int loopexit(int tms = 1000);

public:
	virtual std::string debug() override;


private:
	struct event_base* _base;
	SocketTimer* _timer;
};


#endif // !__event_object_h__
