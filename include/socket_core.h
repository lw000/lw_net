#ifndef __socket_core_h__
#define __socket_core_h__

#include "common_type.h"
#include "common_marco.h"
#include "cache_queue.h"

//#include <mutex>
#include "lock.h"

#include <functional>

using namespace lwstar;

#define SOCKET_CALLBACK(__selector__,__target__, ...) std::bind(&__selector__, __target__, std::placeholders::_1, ##__VA_ARGS__)

typedef void(*LW_PARSE_DATA_CALLFUNC)(lw_int32 cmd, lw_char8* buf, lw_int32 bufsize, lw_void* userdata);

struct LW_NET_MESSAGE
{
	lw_int32 cmd;
	lw_char8* buf;
	lw_int32 size;
};

class SocketCore
{
public:
	SocketCore();
	~SocketCore();

public:
	lw_int32 send(lw_int32 cmd, void* object, lw_int32 objectSize, std::function<lw_int32(LW_NET_MESSAGE* p)> func);
	lw_int32 parse(const lw_char8 * buf, lw_int32 size, LW_PARSE_DATA_CALLFUNC func, lw_void* userdata);
	
private:
	SocketCore(const SocketCore&);
	SocketCore& operator=(const SocketCore&);

private:
	CacheQueue	_cq;
	//std::mutex	_m;
	lw_fast_lock _m;
};

#endif // !__socket_core_h__


