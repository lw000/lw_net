#include "socket_session.h"

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

#include "log4z.h"
#include <memory>
#include "socket_timer.h"
#include "net_package.h"

static int RECV_BUFFER_SIZE	 = 32 * 1024;
static int SEND_BUFFER_SIZE = 32 * 1024;

class SessionCore {
public:
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
		if (ev & BEV_EVENT_CONNECTED) {
			return;
		}

		int er = EVUTIL_SOCKET_ERROR();
		// 10061 Unable to connect because the target computer actively refused
		session->destroy();
		delete session;
	}

	static void __on_data_parse_cb(lw_int32 cmd, char* buf, lw_int32 bufsize, void* userdata) {
		SocketSession *session = (SocketSession *)userdata;
		session->__on_parse(cmd, buf, bufsize);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
SocketSession::SocketSession(SocketConfig* conf) {
	this->_c = SESSION_TYPE::none;
	this->_connected = false;
	this->_bev = nullptr;
	this->_conf = conf;
	this->_iobuffer = new NetIOBuffer;
	this->_timer = new SocketTimer;
}

SocketSession::~SocketSession() {

	this->destroy();

	if (this->_conf != nullptr) {
		SAFE_DELETE(this->_conf);
	}

	if (this->_iobuffer != nullptr) {
		SAFE_DELETE(this->_iobuffer);
	}

	if (this->_timer != nullptr) {
		SAFE_DELETE(this->_timer);
	}
}

int SocketSession::create(SESSION_TYPE c, SocketProcessor* processor, evutil_socket_t fd) {
	if (processor == nullptr) {
		LOGD("this->_processor is nullptr");
		return -1;
	}

	this->_c = c;

	int ret = 0;
	
	switch (this->_c) {
	case SESSION_TYPE::server: {
		this->_bev = bufferevent_socket_new(processor->getBase(), fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
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

		if (this->_connected) {
			this->_timer->create(processor);
		}
	} break;
	case SESSION_TYPE::client: {
		struct sockaddr_in saddr;
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(this->_conf->getHost().c_str());
		saddr.sin_port = htons(this->_conf->getPort());

		this->_bev = bufferevent_socket_new(processor->getBase(), fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
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
	this->_timer->destroy();
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

lw_int32 SocketSession::sendData(lw_int32 cmd, void* object, lw_int32 objectSize, const SocketRecvHandlerConf& cb) {
	if (!this->_connected) {
		LOGFMTD("network is not connected.");
		return -1;
	}
		
	std::unordered_map<lw_int32, SocketRecvHandlerConf>::iterator iter = this->_event_callback_map.find(cb.recvcmd);
	if (iter == _event_callback_map.end()) {
		this->_event_callback_map.insert(std::pair<lw_int32, SocketRecvHandlerConf>(cb.recvcmd, cb));
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

// 	if (cmd == NET_HEART_BEAT_PING) {
// 		LOGFMTD("NET_HEART_BEAT_PING: %d", NET_HEART_BEAT_PING);
// 		this->sendData(NET_HEART_BEAT_PONG, NULL, 0);
// 		return;
// 	}
// 
// 	if (cmd == NET_HEART_BEAT_PONG) {
// 		LOGFMTD("NET_HEART_BEAT_PONG: %d", NET_HEART_BEAT_PONG);
// 		return;
// 	}

	SocketRecvHandlerConf conf;
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
		if (this->parseHandler != nullptr) {
			this->parseHandler(this, cmd, buf, bufsize);
		}
	}
}

void SocketSession::__on_event(short ev) {
	if (ev & BEV_EVENT_CONNECTED) {
		this->_connected = true;
		if (this->connectedHandler != nullptr) {
			this->connectedHandler(this);
		}
		return;
	}

	if (ev & BEV_EVENT_READING) {
		if (this->errorHandler != nullptr) {
			this->errorHandler(this);
		}	
	}
	else if (ev & BEV_EVENT_WRITING) {
		if (this->errorHandler != nullptr) {
			this->errorHandler(this);
		}	
	}
	else if (ev & BEV_EVENT_EOF) {
		if (this->disConnectHandler != nullptr) {
			this->disConnectHandler(this);
		}
	}
	else if (ev & BEV_EVENT_TIMEOUT) {
		if (this->timeoutHandler != nullptr) {
			this->timeoutHandler(this);
		}
	}
	else if (ev & BEV_EVENT_ERROR) {
		if (this->errorHandler != nullptr) {
			this->errorHandler(this);
		}
	}

	this->_connected = false;
/*	bufferevent_free(this->_bev);*/
	this->_bev = nullptr;
}
