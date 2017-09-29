#include "socket_session.h"

#if defined(WIN32) || defined(_WIN32)
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif // WIN32

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>

#include "common_marco.h"
#include "net_core.h"
#include "socket_config.h"
#include "socket_processor.h"

#include "log4z.h"
#include <memory>


#define RECV_BUFFER_SIZE	16*2

class CoreSocket {
public:
	static void __read_cb(struct bufferevent* bev, void* userdata) {
		SocketSession *session = (SocketSession*)userdata;
		session->__onRead();
	}

	static void __write_cb(struct bufferevent *bev, void *userdata) {
		SocketSession * session = (SocketSession*)userdata;
		session->__onWrite();
	}

	static void __event_cb(struct bufferevent *bev, short ev, void *userdata) {
		SocketSession *session = (SocketSession*)userdata;
		session->__onEvent(ev);
		if (ev & BEV_EVENT_CONNECTED) {
			return;
		}
		delete session;
	}

	static void __on_socket_data_parse_cb(lw_int32 cmd, char* buf, lw_int32 bufsize, void* userdata) {
		SocketSession *session = (SocketSession *)userdata;
		session->__onParse(cmd, buf, bufsize);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
SocketSession::SocketSession(AbstractSocketSessionHanlder* handler, NetCore * core, SocketConfig* config) {
	this->_core = core;
	this->_handler = handler;
	this->_connected = false;
	this->_bev = nullptr;
	this->_config = config;
	this->_c = SESSION_TYPE::NONE;
}

SocketSession::~SocketSession() {
	this->__reset();

	if (this->_config != nullptr) {
		SAFE_DELETE(this->_config);
	}
}

int SocketSession::create(SESSION_TYPE c, SocketProcessor* processor, evutil_socket_t fd, short ev) {

	if (this->_processor == nullptr) {
		LOGD("this->_processor is nullptr");
		return -1;
	}

	if (this->_c != c)
	{
		this->_c = c;
	}
	
	if (this->_processor != processor)
	{
		this->_processor = processor;
	}

	switch (this->_c) {
	case SESSION_TYPE::Server: {
		this->_bev = bufferevent_socket_new(this->_processor->getBase(), fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
		bufferevent_setcb(this->_bev, CoreSocket::__read_cb, CoreSocket::__write_cb, CoreSocket::__event_cb, this);
		bufferevent_enable(this->_bev, ev);
		this->_connected = true;
	} break;
	case SESSION_TYPE::Client: {
		struct sockaddr_in saddr;
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(this->_config->getHost().c_str());
		saddr.sin_port = htons(this->_config->getPort());

		this->_bev = bufferevent_socket_new(this->_processor->getBase(), fd, BEV_OPT_CLOSE_ON_FREE/* | BEV_OPT_THREADSAFE*/);
		int con = bufferevent_socket_connect(this->_bev, (struct sockaddr *)&saddr, sizeof(saddr));
		if (con >= 0) {
			bufferevent_setcb(this->_bev, CoreSocket::__read_cb, NULL, CoreSocket::__event_cb, this);
			bufferevent_enable(this->_bev, ev);
		}
		else {
			this->_connected = false;
		}
	} break;	
	default:
		break;
	}
	return 0;
}

void SocketSession::destroy() {
	this->__reset();
}


bool SocketSession::connected() {
	return this->_connected;
}

void SocketSession::__reset() {
	this->_bev = nullptr;
	this->_handler = nullptr;
	this->_connected = false;
	this->_c = SESSION_TYPE::NONE;
	this->_config->reset();
}

std::string SocketSession::debug() {
	char buf[512];
	evutil_socket_t fd = bufferevent_getfd(this->_bev);
	sprintf(buf, "fd:%d, ip:%s, port:%d, c:%d, connected:%d", fd, this->_config->getHost().c_str(), this->_config->getPort(), this->_c, this->_connected);
	return std::string(buf);
}

lw_int32 SocketSession::sendData(lw_int32 cmd, void* object, lw_int32 objectSize) {
	if (this->_connected) {
		int c = this->_core->send(cmd, object, objectSize, [this](LW_NET_MESSAGE * p) -> lw_int32 {
			int c = bufferevent_write(this->_bev, p->buf, p->size);
			return c;
		});
		return c;
	}
	else
	{
		LOGD("network is not connected.");
	}
	return -1;
}

lw_int32 SocketSession::sendData(lw_int32 cmd, void* object, lw_int32 objectSize, lw_int32 recvcmd, SocketRecvCallback cb) {
	if (this->_connected) {
		std::unordered_map<lw_int32, SocketRecvCallback>::iterator iter = this->_cmd_event_map.find(recvcmd);
		if (iter == _cmd_event_map.end()) {
			this->_cmd_event_map.insert(std::pair<lw_int32, SocketRecvCallback>(recvcmd, cb));
		}
		else {
			iter->second = cb;
		}
		int c = this->_core->send(cmd, object, objectSize, [this](LW_NET_MESSAGE * p) -> lw_int32 {
			int c1 = bufferevent_write(this->_bev, p->buf, p->size);
			return c1;
		});
		return c;
	}
	else
	{
		LOGD("network is not connected.");
	}
	return -1;
}

void SocketSession::__onWrite() {

}

void SocketSession::__onRead() {
	struct evbuffer *input = bufferevent_get_input(this->_bev);
	size_t input_len = evbuffer_get_length(input);

#if 0
	char recv_buf[RECV_BUFFER_SIZE + 1];
	::memset(recv_buf, 0x00, sizeof(recv_buf));
	while (input_len > 0) {
		size_t recv_len = bufferevent_read(this->_bev, recv_buf, RECV_BUFFER_SIZE);
		if (recv_len > 0) {
			if (this->_core->parse(recv_buf, recv_len, CoreSocket::__on_socket_data_parse_cb, this) == 0) {

			}
		}
		else {
			lw_char8 buf[256];
			sprintf(buf, "SocketSession::__onRead [read_len: %d]", recv_len);
			LOGD(buf);
		}
		input_len -= recv_len;
	}
#else
	std::unique_ptr<lw_char8[]> recv_buf(new lw_char8[input_len]);
	while (input_len > 0) {
		size_t recv_len = bufferevent_read(this->_bev, recv_buf.get(), input_len); 
		if (recv_len > 0) {
			if (this->_core->parse(recv_buf.get(), recv_len, CoreSocket::__on_socket_data_parse_cb, this) == 0) {

			}
		}
		else {
			lw_char8 buf[256];
			sprintf(buf, "SocketSession::__onRead. read_len: %d", recv_len);
			LOGD(buf);
		}
		input_len -= recv_len;
	}
#endif
}

void SocketSession::__onParse(lw_int32 cmd, char* buf, lw_int32 bufsize) {
	SocketRecvCallback cb = nullptr;
	if (!_cmd_event_map.empty()) {
		cb = this->_cmd_event_map.at(cmd);
	}
	
	bool goon = true;
	if (cb != nullptr) {
		goon = cb(buf, bufsize);
	}

	if (goon) {
		this->_handler->onSocketParse(this, cmd, buf, bufsize);
	}
}

void SocketSession::__onEvent(short ev) {
	if (ev & BEV_EVENT_CONNECTED) {
		this->_connected = true;
		if (this->connectedHandler != nullptr) {
			this->connectedHandler(this);
		}
		this->_handler->onSocketConnected(this);
		return;
	}

	if (ev & BEV_EVENT_READING) {
		if (this->errorHandler != nullptr) {
			this->errorHandler(this);
		}
		this->_handler->onSocketError(this);
	}
	else if (ev & BEV_EVENT_WRITING) {
		if (this->errorHandler != nullptr) {
			this->errorHandler(this);
		}
		this->_handler->onSocketError(this);
	}
	else if (ev & BEV_EVENT_EOF) {
		if (this->disConnectHandler != nullptr) {
			this->disConnectHandler(this);
		}
		this->_handler->onSocketDisConnect(this);
	}
	else if (ev & BEV_EVENT_TIMEOUT) {
		if (this->timeoutHandler != nullptr) {
			this->timeoutHandler(this);
		}
		this->_handler->onSocketTimeout(this);
	}
	else if (ev & BEV_EVENT_ERROR) {
		if (this->errorHandler != nullptr) {
			this->errorHandler(this);
		}
		this->_handler->onSocketError(this);
	}

	this->_connected = false;
	this->_bev = nullptr;
}
