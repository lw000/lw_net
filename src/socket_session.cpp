#include "socket_session.h"

#include <memory>

#if defined(WIN32) || defined(_WIN32)
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif // WIN32

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include "common_marco.h"
#include "net_iobuffer.h"
#include "socket_config.h"
#include "socket_processor.h"
#include "socket_timer.h"
#include "socket_heartbeat.h"

#include "log4z.h"

static int RECV_BUFFER_SIZE	 = 32 * 1024;
static int SEND_BUFFER_SIZE = 32 * 1024;

class SessionCore {
public:
	static enum bufferevent_filter_result __filter_read_cb(
	struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, void *ctx) {


		return BEV_OK;
	}

	static enum bufferevent_filter_result __filter_write_cb(
	struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, void *ctx) {


		return BEV_OK;
	}

	static void __read_cb(struct bufferevent* bev, void* userdata) {
		SocketSession *session = (SocketSession*)userdata;
		session->__on_read();
	}

	static void __write_cb(struct bufferevent *bev, void *userdata) {
		SocketSession * session = (SocketSession*)userdata;
		session->__on_write();
	}

	static void __event_cb(struct bufferevent *bev, short ev, void *userdata) {
		SocketSession *session = (SocketSession*)userdata;
		session->__on_event(ev);
	}

	static void __on_data_parse_cb(lw_int32 cmd, char* buf, lw_int32 bufsize, void* userdata) {
		SocketSession *session = (SocketSession *)userdata;
		session->__on_parse(cmd, buf, bufsize);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
SocketSession::SocketSession() {
	this->_c = SESSION_TYPE::none;
	this->_connected = false;
	this->_bev = nullptr;
	this->_conf = nullptr;
	this->_processor = nullptr;
	this->_iobuffer = new NetIOBuffer;
	this->_heartbeat = new SocketHeartbeat(this);
}

SocketSession::~SocketSession() {
	this->destroy();
	SAFE_DELETE(this->_heartbeat);
	SAFE_DELETE(this->_conf);
	SAFE_DELETE(this->_iobuffer);
}

int SocketSession::create(SESSION_TYPE c, SocketProcessor* processor, SocketConfig* conf, evutil_socket_t fd) {
	if (processor == nullptr) {
		LOGD("this->_processor is nullptr");
		return -1;
	}

	this->_c = c;

	this->_processor = processor;

	this->_conf = conf;

	this->_heartbeat->create(this->_processor);

	int ret = 0;
	
	switch (this->_c) {
	case SESSION_TYPE::server: {
		this->_bev = bufferevent_socket_new(this->_processor->getBase(), fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
		if (this->_bev != nullptr) {
			
//			{
//				int c0 = bufferevent_set_max_single_write(this->_bev, SEND_BUFFER_SIZE);
//				int c1 = bufferevent_set_max_single_read(this->_bev, RECV_BUFFER_SIZE);
//
//				int d1 = bufferevent_get_max_single_read(this->_bev);
//				int d2 = bufferevent_get_max_single_write(this->_bev);
//				int d3 = bufferevent_get_max_to_read(this->_bev);
//				int d4 = bufferevent_get_max_to_write(this->_bev);
//			}
			
//			bufferevent_filter_new(this->_bev, SessionCore::__filter_read_cb, SessionCore::__filter_write_cb, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE, NULL, NULL);
			
			bufferevent_setcb(this->_bev, SessionCore::__read_cb, SessionCore::__write_cb, SessionCore::__event_cb, this);
			bufferevent_enable(this->_bev, EV_READ | EV_WRITE | EV_PERSIST);

			{
				evutil_socket_t nfd = bufferevent_getfd(this->_bev);
				int c1 = setsockopt(nfd, SOL_SOCKET, SO_RCVBUF, (const char*)&RECV_BUFFER_SIZE, sizeof(int));
				int c2 = setsockopt(nfd, SOL_SOCKET, SO_SNDBUF, (const char*)&SEND_BUFFER_SIZE, sizeof(int));
			}

			this->_connected = true;

			ret = 0;
		}
		else {
			ret = -1;
		}
	} break;
	case SESSION_TYPE::client: {
		struct sockaddr_in saddr;
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(this->_conf->getHost().c_str());
		saddr.sin_port = htons(this->_conf->getPort());

		this->_bev = bufferevent_socket_new(this->_processor->getBase(), fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
		if (this->_bev != nullptr) {
			int con = bufferevent_socket_connect(this->_bev, (struct sockaddr *)&saddr, sizeof(saddr));
			if (con == 0) {
				bufferevent_setcb(this->_bev, SessionCore::__read_cb, NULL, SessionCore::__event_cb, this);
				bufferevent_enable(this->_bev, EV_READ | EV_PERSIST);

				evutil_socket_t nfd = bufferevent_getfd(this->_bev);
				int c1 = setsockopt(nfd, SOL_SOCKET, SO_RCVBUF, (const char*)&RECV_BUFFER_SIZE, sizeof(int));
				int c2 = setsockopt(nfd, SOL_SOCKET, SO_SNDBUF, (const char*)&SEND_BUFFER_SIZE, sizeof(int));

				ret = 0;
			}
			else {
				this->_connected = false;
				ret = -2;
			}
		}
		else {
			ret = -1;
		}
		
	} break;
	default: {
		ret = -3;
		LOGD("unknow SESSION_TYPE [none, server, client]");
		break;
	}
	}

	return ret;
}

void SocketSession::destroy() {
	this->_event_callback_map.clear();
}

void SocketSession::startAutoPing(int tms) {
	this->_heartbeat->setAutoHeartBeat(tms);
}

bool SocketSession::connected() const {
	return this->_connected;
}

SocketConfig* SocketSession::getConf() const {
	return this->_conf;
}

std::string SocketSession::debug() {
	char buf[512];
	evutil_socket_t fd = bufferevent_getfd(this->_bev);
	sprintf(buf, "fd:%d, c:%d, connected:%d, conf:%s", fd, this->_c, this->_connected, this->_conf->debug().c_str());
	return std::string(buf);
}

lw_int32 SocketSession::sendData(lw_int32 cmd, void* object, lw_int32 objectSize) {
	if (!this->_connected) {
		LOGD("network is not connected.");
		return -1;
	}
	
	int c = this->_iobuffer->send(cmd, object, objectSize, [this](NET_MESSAGE * p) -> lw_int32 {
		int c = bufferevent_write(this->_bev, p->buf, p->size);
		if (c != 0) {
			LOGFMTD("error occurred. [%d]", c);
		}
		return c;
	});
	return c;
}

lw_int32 SocketSession::sendData(lw_int32 cmd, void* object, lw_int32 objectSize, const SendDataCallback& cb) {
	if (!this->_connected) {
		LOGFMTD("network is not connected.");
		return -1;
	}
		
	std::unordered_map<lw_int32, SendDataCallback>::iterator iter = this->_event_callback_map.find(cb.recvcmd);
	if (iter == _event_callback_map.end()) {
		this->_event_callback_map.insert(std::pair<lw_int32, SendDataCallback>(cb.recvcmd, cb));
	}
	else {
		iter->second = cb;
	}

	int c = this->_iobuffer->send(cmd, object, objectSize, [this](NET_MESSAGE * p) -> lw_int32 {
		int c = bufferevent_write(this->_bev, p->buf, p->size);
		if (c != 0) {
			LOGFMTD("error occurred. [%d]", c);
		}
		return c;
	});

	return c;
}

void SocketSession::__on_write() {
	struct evbuffer *output = bufferevent_get_output(this->_bev);
	size_t output_len = evbuffer_get_length(output);
	if (output_len > 0) {
		LOGFMTD("SocketSession::__onRead. output_len: %d", output_len);
	} 
	else {
	
	}
}

void SocketSession::__on_read() {
	struct evbuffer *input = bufferevent_get_input(this->_bev);
	size_t input_len = evbuffer_get_length(input);

	std::unique_ptr<lw_char8[]> recv_buf(new lw_char8[input_len]);
	while (input_len > 0) {
		size_t recv_len = bufferevent_read(this->_bev, recv_buf.get(), input_len); 
		if (recv_len > 0 && recv_len == input_len) {
			int c = this->_iobuffer->parse(recv_buf.get(), recv_len, SessionCore::__on_data_parse_cb, this);
			if (c != 0) {
				LOGFMTD("parse faild. [%d]", c);
			}
		}
		else {
			LOGFMTD("SocketSession::__onRead recv_len: %d", recv_len);
		}
		input_len -= recv_len;
	}
}

void SocketSession::__on_parse(lw_int32 cmd, char* buf, lw_int32 bufsize) {
	int c = this->_heartbeat->onDataParseHandler(this, cmd, buf, bufsize);
	if (c == 0) {
		return;
	}

	if (c == -1) {	// ping package

	}

	if (c == -2) {	// pong package

	}

	SendDataCallback conf;
	if (!_event_callback_map.empty()) {
		conf = this->_event_callback_map.at(cmd);
	}

	bool goon = true;
	if (conf.cb != nullptr) {
		goon = conf.cb(buf, bufsize);

		if (conf.forever <= 0) {
			_event_callback_map.erase(cmd);
		}
	}

	if (goon) {
		if (this->onDataParseHandler != nullptr) {
			this->onDataParseHandler(this, cmd, buf, bufsize);
		}
	}
}

void SocketSession::__on_event(short ev) {

	if (ev & BEV_EVENT_CONNECTED) {
		this->_connected = true;
		this->_heartbeat->onConnectedHandler(this);
		if (this->onConnectedHandler != nullptr) {
			this->onConnectedHandler(this);
		}
		return;
	}

	if (ev & BEV_EVENT_READING) {
		this->_heartbeat->onErrorHandler(this);
		if (this->onErrorHandler != nullptr) {
			this->onErrorHandler(this);
		}
	}
	else if (ev & BEV_EVENT_WRITING) {	
		this->_heartbeat->onErrorHandler(this);
		if (this->onErrorHandler != nullptr) {
			this->onErrorHandler(this);
		}
	}
	else if (ev & BEV_EVENT_EOF) {
		this->_heartbeat->onDisconnectHandler(this);
		if (this->onDisconnectHandler != nullptr) {
			this->onDisconnectHandler(this);
		}
	}
	else if (ev & BEV_EVENT_TIMEOUT) {
		this->_heartbeat->onTimeoutHandler(this);
		if (this->onTimeoutHandler != nullptr) {
			this->onTimeoutHandler(this);
		}
	}
	else if (ev & BEV_EVENT_ERROR) {
		this->_heartbeat->onErrorHandler(this);
		if (this->onErrorHandler != nullptr) {
			this->onErrorHandler(this);
		}
	}
	this->_connected = false;

	bufferevent_free(this->_bev);
	this->_bev = nullptr;

	int errcode = EVUTIL_SOCKET_ERROR();
	const char* errstr = evutil_socket_error_to_string(errcode);
	// 10061 Unable to connect because the target computer actively refused
	LOGFMTD("__event_cb: %s", errstr);
}
