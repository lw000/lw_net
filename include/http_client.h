#ifndef __http_client_h__
#define __http_client_h__

#include <string>
#include <queue>
#include <functional>
#include <time.h>

#include "common_type.h"
#include "Threadable.h"
#include "lock.h"
#include "object.h"


struct evhttp_connection;
struct evhttp_uri;

class SocketProcessor;
class NetworkThreadServer;
class HttpClient;
class HttpReponse;

typedef std::function<void(HttpReponse* reponse)> HttpCallback;

class HttpRequest : public Object {
public:
	enum class MEDHOD {
		NONE = 0,
		GET = 1,
		POST = 2,
	};

public:
	HttpRequest(HttpClient* client);
	virtual ~HttpRequest();

public:
	void setUrl(const std::string& url);
	void setMethod(MEDHOD method);
	void setCallback(const HttpCallback& cb);

public:
	void start();

public:
	virtual std::string debug() override;

public:
	struct evhttp_connection *_evcon;
	struct evhttp_uri *_host;

public:
	int cancel;	// 0, 1
	MEDHOD _method;
	HttpClient* _client;
	HttpCallback _cb;

public:
	clock_t begintime;
	clock_t endtime;
};

class HttpReponse : public Object {
public:
	HttpReponse();
	virtual ~HttpReponse();

public:
	int getCode() const;
	std::string getContent() const;

public:
	virtual std::string debug() override;

public:
	int code;
	std::vector<char> _buffer;
};

class HttpClient : public Threadable {
public:
	NetworkThreadServer* serverThread;
	SocketProcessor* _processor;
	std::queue<HttpRequest*> _requestQueue;
	std::queue<HttpRequest*> _reponseQueue;
	lw_fast_lock _requestLock;
	lw_fast_lock _reponseLock;
	lw_fast_cond _requestCond;

public:
	HttpClient();
	virtual ~HttpClient();

public:
	bool create();
	void destroy();

public:
	SocketProcessor* getProcessor() const;

public:
	void add(HttpRequest* request);
	void cancel(HttpRequest* request);

protected:
	virtual int onStart() override;
	virtual int onRun() override;
	virtual int onEnd() override;
};

#endif // !__http_client_h__
