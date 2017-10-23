#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <functional>
#include <string>
#include <unordered_map>
#include "common_type.h"
#include "Threadable.h"

struct event_base;
struct evhttp;
struct evhttp_request;
struct HTTP_METHOD_SIGNATURE;
class HttpServer;
class SocketProcessor;
class SocketConfig;

void lw_http_send_reply(struct evhttp_request * req, const char* what);

class HttpServer : public Threadable
{
	friend class CoreHttp;

	typedef std::unordered_map<std::string, HTTP_METHOD_SIGNATURE*> MAP_METHOD;
	typedef MAP_METHOD::iterator iterator;
	typedef MAP_METHOD::const_iterator const_iterator;

public:
	HttpServer();
	virtual ~HttpServer();

public:
	lw_int32 create(SocketConfig* config);
	void listen();
	void gen(std::function<void (struct evhttp_request *)> cb);
	void get(const char * path, std::function<void(struct evhttp_request *)> cb);
	void post(const char * path, std::function<void(struct evhttp_request *)> cb);

protected:
	virtual int onStart() override;
	virtual int onRun() override;
	virtual int onEnd() override;

	void __doStore(const char * path, lw_int32 cmd, std::function<void(struct evhttp_request *)> cb);

private:
	SocketProcessor* _processor;
	struct evhttp *_htpServ;

private:
	MAP_METHOD _method;
	std::function<void(struct evhttp_request *)> _gen_cb;
};

#endif // !__HTTP_SERVER_H__