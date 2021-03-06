﻿#include "http_server.h"

#include <stdio.h>
#include <signal.h>
#include <string>
#include <vector>
#include <unordered_map>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif

#include <memory>

#include <event2/event.h>
#include <event2/event-config.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/util.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "socket_processor.h"
#include "socket_config.h"

#include "utils.h"

#include "log4z.h"
#include "common_marco.h"

struct HTTP_METHOD_SIGNATURE
{
	std::string sign;
	lw_uint32 cmd;
	std::function<void(struct evhttp_request *)> _request_cb;
};

struct EqualKey
{
	bool operator () (const HTTP_METHOD_SIGNATURE &lhs, const HTTP_METHOD_SIGNATURE &rhs) const
	{
		return lhs.sign == rhs.sign && lhs.cmd == rhs.cmd;
	}
};

struct HashFunc
{
	std::size_t operator()(const HTTP_METHOD_SIGNATURE &key) const
	{
		using std::size_t;
		using std::hash;

		return ((hash<std::string>()(key.sign) ^ (hash<lw_uint32>()(key.cmd) << 1)) >> 1);
	}
};

void lw_http_send_reply(struct evhttp_request * req, const char* what)
{
	struct evbuffer *buf = evbuffer_new();
	evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
	evbuffer_add_printf(buf, what);
	evhttp_send_reply(req, HTTP_OK, NULL, buf);
	evbuffer_free(buf);
}

class CoreHttp {

public:
	static void __genHandler(struct evhttp_request *req, void *arg)
	{
		HttpServer* serv = (HttpServer*)arg;
		if (serv->_gen_cb != nullptr)
		{
			serv->_gen_cb(req);
		}
	}

public:
	static void __requestHandler(struct evhttp_request *req, void *arg)
	{
		HTTP_METHOD_SIGNATURE* signature = (HTTP_METHOD_SIGNATURE*)arg;
		evhttp_cmd_type cmd = evhttp_request_get_command(req);
		switch (cmd)
		{
		case EVHTTP_REQ_GET:
		{
			if (signature->cmd == cmd) {
				signature->_request_cb(req);
			}
			else {
				lw_http_send_reply(req, "{\"code\":0,\"what\":\"The requested resource does not support http method 'GET'.""}");
			}
		} break;
		case EVHTTP_REQ_POST:
		{
			if (signature->cmd == cmd) {
				signature->_request_cb(req);
			}
			else {
				lw_http_send_reply(req, "{\"code\":0,\"what\":\"The requested resource does not support http method 'POST'.""}");
			}
		} break;
		default: {
			lw_http_send_reply(req, "{\"code\":0,\"what\":\"The requested resource only support http method 'GET' OR 'POST'.""}");
		} break;
		}
	}
};


//////////////////////////////////////////////////////////////////////////////////////////

HttpServer::HttpServer()
{
	this->_htpServ = nullptr;
	this->_processor = new SocketProcessor;
}

HttpServer::~HttpServer()
{
	SAFE_DELETE(this->_processor);
}

lw_int32 HttpServer::create(SocketConfig* config)
{
	if (config == nullptr) {
		return -1;
	}

/*	this->_config = config;*/

	bool r = this->_processor->create(false);
	if (r) {
		this->_htpServ = evhttp_new(this->_processor->getBase());
		if (!this->_htpServ)
		{
			LOGD("couldn't?create?evhttp.?Exiting.\n");
			return -1;
		}

		struct evhttp_bound_socket *handle = evhttp_bind_socket_with_handle(this->_htpServ, config->getHost().c_str(), config->getPort());
		if (!handle)
		{
			LOGFMTD("couldn't?bind?to?port?%d.?exiting.", (int)config->getPort());
			evhttp_free(this->_htpServ);
			return -1;
		}

		// 设置超时
		evhttp_set_timeout(this->_htpServ, 30);

		/* Extract and display the address we're listening on. */
		do
		{
			struct sockaddr_storage ss;
			memset(&ss, 0, sizeof(ss));
			ev_socklen_t socklen = sizeof(ss);		
			evutil_socket_t fd = evhttp_bound_socket_get_fd(handle);
			if (getsockname(fd, (struct sockaddr *)&ss, &socklen))
			{
				LOGFMTD("getsockname() failed");
				break;
			}

			void *inaddr;
			int got_port = -1;
			if (ss.ss_family == AF_INET)
			{
				got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
				inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
			}
			else if (ss.ss_family == AF_INET6)
			{
				got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
				inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
			}
			else
			{
				LOGFMTD("weird address family %d", ss.ss_family);
				break;
			}
			char addrbuf[128];
			const char *addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf, sizeof(addrbuf));
			if (addr != NULL)
			{
				LOGFMTD("HTTP服务启动完成 [http://%s:%d]", addr, got_port);
			}
			else
			{
				LOGFMTD("evutil_inet_ntop failed");
				break;
			}
		} while (0);
	}

	return 0;
}

void HttpServer::listen()
{
	this->start();
}

// 设置回调 
void HttpServer::gen(std::function<void(struct evhttp_request *)> cb)
{
	this->_gen_cb = cb;
	evhttp_set_gencb(this->_htpServ, CoreHttp::__genHandler, this);
}

void HttpServer::get(const char * path, std::function<void(struct evhttp_request *)> cb)
{
	__doStore(path, EVHTTP_REQ_GET, cb);
}

void HttpServer::post(const char * path, std::function<void(struct evhttp_request *)> cb)
{
	__doStore(path, EVHTTP_REQ_POST, cb);
}

void HttpServer::__doStore(const char * path, lw_int32 cmd, std::function<void(struct evhttp_request *)> cb)
{
	HTTP_METHOD_SIGNATURE* signature = new HTTP_METHOD_SIGNATURE;
	signature->sign = path;
	signature->cmd = cmd;
	signature->_request_cb = cb;
	int r = evhttp_set_cb(this->_htpServ, path, CoreHttp::__requestHandler, signature);
	if (r == 0)
	{
		iterator iter = this->_method.find(path);
		if (iter == _method.end())
		{
			this->_method.insert(std::pair<std::string, HTTP_METHOD_SIGNATURE*>(path, signature));
		}
		else
		{
			iter->second = signature;
		}
	}
}

int HttpServer::onStart() {
	return 0;
}

int HttpServer::onRun() {
	int c = this->_processor->dispatch();
	LOGFMTD("dispatch c = %d", c);
	return 0;
}

int HttpServer::onEnd() {
	evhttp_free(this->_htpServ);
	this->_htpServ = nullptr;
	this->_processor->destroy();
	return 0;
}