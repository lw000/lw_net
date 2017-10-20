#include "http_client.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <thread>
#include <string.h>
#include <algorithm>

#include "event2/event-config.h"
#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/buffer.h>

#include "socket_processor.h"
#include "common_marco.h"

#include "log4z.h"

#define URL_MAX 4096

static void __http_get_callback(struct evhttp_request *req, void *arg)
{
	if (req == nullptr) return;

	HttpRequest *request = (HttpRequest *)arg;

	request->endtime = clock();
	LOGFMTD("request time: %f", ((double)request->endtime - request->begintime) / CLOCKS_PER_SEC);

	if (request->cancel == 0) {
	/*	char* recvBuffer = nullptr;*/
		ev_ssize_t recvLenght = 0;
		char recvBuffer[4096];
		{
			struct evbuffer *evbuf = evhttp_request_get_input_buffer(req);
			recvLenght = evbuffer_get_length(evbuf);
			//recvBuffer = new char[recvLenght];
			recvBuffer[recvLenght] = '\0';
			memcpy(recvBuffer, evbuffer_pullup(evbuf, recvLenght), recvLenght);
			evbuffer_drain(evbuf, recvLenght);
		}

		if (request->_cb != nullptr) {
			HttpReponse reponse;
			reponse.code = req->response_code;
			reponse.setTag(request->getTag());
			if (recvLenght > 0 && recvBuffer != nullptr) {
				reponse._buffer.insert(reponse._buffer.end(), recvBuffer, recvBuffer + recvLenght);
			}
			request->_cb(&reponse);
		}

// 		if (recvBuffer != nullptr) {
// 			delete recvBuffer;
// 			recvBuffer = nullptr;
// 		}
	}

	SAFE_DELETE(request);
}

static void __http_post_callback(struct evhttp_request *req, void *arg)
{
	if (req == NULL) return;

	HttpRequest *request = (HttpRequest *)arg;
}

static void __http_connect_cb(struct evhttp_request *proxy_req, void *arg)
{
	HttpRequest *request = (HttpRequest *)arg;
	if (request->cancel == 0) {
		if (request->_evcon != nullptr)
		{
			if (request->_method == HttpRequest::MEDHOD::GET) {
				struct evhttp_request *req = evhttp_request_new(__http_get_callback, arg);
				evhttp_add_header(req->output_headers, "Connection", "close");
				char buffer[URL_MAX];
				const char* url = evhttp_uri_join(request->_host, buffer, URL_MAX);
				evhttp_make_request(request->_evcon, req, EVHTTP_REQ_GET, url);
			}
			else if (request->_method == HttpRequest::MEDHOD::POST) {
				struct evhttp_request *req = evhttp_request_new(__http_post_callback, arg);
				evhttp_add_header(req->output_headers, "Connection", "close");
				char buffer[URL_MAX];
				const char* url = evhttp_uri_join(request->_host, buffer, URL_MAX);
				evhttp_make_request(request->_evcon, req, EVHTTP_REQ_POST, url);
			}
			else {

			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

class NetworkThreadServer : public Threadable {
private:
	HttpClient* _client;
	bool _quit;

public:
	NetworkThreadServer(HttpClient* client) : _client(client) {
		_quit = false;
	}

	virtual ~NetworkThreadServer() {

	}

public:
	void quitServer() {
		this->_quit = true;
	}

public:
	virtual int onStart() override {
		return 0;
	}

	virtual int onRun() override {

		while (!_quit)
		{		
			{
				lw_lock_guard l(&this->_client->_requestLock);
				if (this->_client->_requestQueue.empty())
				{
					this->_client->_requestCond.wait(&this->_client->_requestLock);
				}
			}

			if (_quit) {
				{
					lw_lock_guard l(&this->_client->_requestLock);
					while (!this->_client->_requestQueue.empty())
					{
						HttpRequest *request = this->_client->_requestQueue.front();
						this->_client->_requestQueue.pop();
						SAFE_DELETE(request);
					}
				}
				break;
			}

			HttpRequest * request = nullptr;
			{
				lw_lock_guard l(&this->_client->_requestLock);
				request = this->_client->_requestQueue.front();
				this->_client->_requestQueue.pop();
			}
			
			if (request != nullptr) {
				if (request->cancel == 0) {
					request->start();
				}
				else {
					SAFE_DELETE(request);
				}
			}
		}

		return 0;
	}

	virtual int onEnd() override {

		return 0;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
HttpRequest::HttpRequest(HttpClient* client) : _client(client) {
	this->_evcon = nullptr;
	this->_host = nullptr;
	this->_method = MEDHOD::NONE;
	this->cancel = 0;

	this->begintime = 0;
	this->endtime = 0;
}

HttpRequest::~HttpRequest() {
	evhttp_uri_free(this->_host);
	evhttp_connection_free(this->_evcon);

	LOGD("~HttpRequest");
}

void HttpRequest::setUrl(const std::string& url) {
	if (url.empty()) return;

	if (this->_host != nullptr) {
		evhttp_uri_free(this->_host);
		this->_host = nullptr;
	}
	this->_host = evhttp_uri_parse(url.c_str());
}

void HttpRequest::setMethod(MEDHOD method) {
	this->_method = method;
}

void HttpRequest::setCallback(const HttpCallback& cb) {
	this->_cb = cb;
}

void HttpRequest::start() {
	char buffer[URL_MAX];

	const char* h = evhttp_uri_get_host(this->_host);
	int p = evhttp_uri_get_port(this->_host);

	this->_evcon = evhttp_connection_base_new(this->_client->getProcessor()->getBase(), NULL, h, p);
	struct evhttp_request* req = evhttp_request_new(__http_connect_cb, this);

	evhttp_add_header(req->output_headers, "Connection", "keep-alive");
	evhttp_add_header(req->output_headers, "Proxy-Connection", "keep-alive");
	evutil_snprintf(buffer, URL_MAX, "%s:%d", h, p);

	int c = evhttp_make_request(this->_evcon, req, EVHTTP_REQ_CONNECT, buffer);
	this->begintime = clock();
}

std::string HttpRequest::debug() {
	return std::string("HttpRequest");
}

//////////////////////////////////////////////////////////////////////////////////////////

HttpReponse::HttpReponse() : code(0) {

}

HttpReponse::~HttpReponse() {

}

std::string HttpReponse::debug() {
	return std::string("HttpReponse");
}

int HttpReponse::getCode() const {
	return this->code;
}

std::string HttpReponse::getContent() const {
	std::string data(_buffer.begin(), _buffer.end());
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////
HttpClient::HttpClient()
{
	this->_processor = new SocketProcessor();
	this->serverThread = new NetworkThreadServer(this);
}

HttpClient::~HttpClient()
{
	SAFE_DELETE(this->serverThread);
	SAFE_DELETE(this->_processor);
}

bool HttpClient::create() {

	bool r = this->_processor->create(false);
	if (r) {
		this->serverThread->start();
	}
	return r;
}

void HttpClient::destroy() {
	this->_processor->destroy();
}

SocketProcessor* HttpClient::getProcessor() const {
	return this->_processor;
}

void HttpClient::add(HttpRequest* request) {
	{
		lw_lock_guard l(&_requestLock);
		_requestQueue.push(request);
		_requestCond.signal();
	}
}

void HttpClient::cancel(HttpRequest* request) {
	request->cancel = 1;
}

int HttpClient::onStart() {
	return 0;
}

int HttpClient::onRun() {

	int c = this->_processor->dispatch();

	return 0;
}

int HttpClient::onEnd()
{
	this->destroy();

	return 0;
}