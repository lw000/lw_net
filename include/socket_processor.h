#ifndef __event_object_h__
#define __event_object_h__

#include <string>

#include "common_type.h"
#include "object.h"

struct event_base;

class SocketProcessor : public Object
{
public:
	SocketProcessor();
	virtual ~SocketProcessor();

public:
	bool create(bool enableServer);

public:
	struct event_base* getBase();

public:
	int dispatch();

public:
	int loopbreak();
	int loopexit();

public:
	virtual std::string debug() override;

private:
	void use_threads();

private:
	struct event_base* _base;
};


#endif // !__event_object_h__