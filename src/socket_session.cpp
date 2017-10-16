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
#include "net_iobuffer.h"
#include "socket_config.h"
#include "socket_processor.h"

#include "log4z.h"
#include <memory>

#define RECV_BUFFER_SIZE	16*2

class SessionCore {
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
		session->destroy();
		delete session;
	}

	static void __on_socket_data_parse_cb(lw_int32 cmd, char* buf, lw_int32 bufsize, void* userdata) {
		SocketSession *session = (SocketSession *)userdata;
		session->__onParse(cmd, buf, bufsize);
// 		LOGD("__on_socket_data_parse_cb");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
SocketSession::SocketSession(/*AbstractSocketSessionHanlder* handler, */SocketConfig* config) {
	this->_iobuffer = new NetIOBuffer;
// 	this->_handler = handler;
	this->_config = config;
	this->_connected = false;
	this->_bev = nullptr;
	this->_c = SESSION_TYPE::NONE;
}

SocketSession::~SocketSession() {
	if (this->_config != nullptr) {
		SAFE_DELETE(this->_config);
	}

	if (this->_iobuffer != nullptr) {
		SAFE_DELETE(this->_iobuffer);
	}
}

int SocketSession::create(SESSION_TYPE c, SocketProcessor* processor, evutil_socket_t fd) {

	if (processor == nullptr) {
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
	
// 		struct timeval tv_read = { 10, 0 }, tv_write = { 10, 0 };
// 		bufferevent_set_timeouts(this->_bev, &tv_read, &tv_write);		
// 		bufferevent_enable(this->_bev, BEV_OPT_THREADSAFE);

		bufferevent_setcb(this->_bev, SessionCore::__read_cb, SessionCore::__write_cb/*NULL*/, SessionCore::__event_cb, this);
		bufferevent_enable(this->_bev, EV_READ | EV_WRITE | EV_PERSIST);
//		int nRecvBuf = 32 * 1024;
//		int c1 = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));
//		int nSendBuf = 32 * 1024;
//		int c2 = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int));
		this->_connected = true;
	} break;
	case SESSION_TYPE::Client: {
		struct sockaddr_in saddr;
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(this->_config->getHost().c_str());
		saddr.sin_port = htons(this->_config->getPort());

		this->_bev = bufferevent_socket_new(this->_processor->getBase(), fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
		int con = bufferevent_socket_connect(this->_bev, (struct sockaddr *)&saddr, sizeof(saddr));
		if (con >= 0) {
			bufferevent_setcb(this->_bev, SessionCore::__read_cb, /*CoreSocket::__write_cb*/NULL, SessionCore::__event_cb, this);
			bufferevent_enable(this->_bev, EV_READ | EV_PERSIST);
			
//			evutil_socket_t nfd = bufferevent_getfd(this->_bev);
//			int nRecvBuf = 32 * 1024;
//			int c1 = setsockopt(nfd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));
//			int nSendBuf = 32 * 1024;
//			int c2 = setsockopt(nfd, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int));
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

}

bool SocketSession::connected() {
	return this->_connected;
}

SocketConfig* SocketSession::getConfig() const {
	return this->_config;
}

std::string SocketSession::debug() {
	char buf[512];
	evutil_socket_t fd = bufferevent_getfd(this->_bev);
	sprintf(buf, "fd:%d, ip:%s, port:%d, c:%d, connected:%d", fd, this->_config->getHost().c_str(), this->_config->getPort(), this->_c, this->_connected);
	return std::string(buf);
}

lw_int32 SocketSession::sendData(lw_int32 cmd, void* object, lw_int32 objectSize) {
	if (!this->_connected) {
		LOGD("network is not connected.");
		return -1;
	}
	
	int c = this->_iobuffer->send(cmd, object, objectSize, [this](NET_MESSAGE * p) -> lw_int32 {
		int c1 = bufferevent_write(this->_bev, p->buf, p->size);
		if (c1 != 0) {
			LOGFMTD("error occurred. [%d]", c1);
		}
		return c1;
	});
	return c;
}

lw_int32 SocketSession::sendData(lw_int32 cmd, void* object, lw_int32 objectSize, lw_int32 recvcmd, SocketRecvCallback cb) {
	if (!this->_connected) {
		LOGFMTD("network is not connected.");
		return -1;
	}
		
	std::unordered_map<lw_int32, SocketRecvCallback>::iterator iter = this->_cmd_event_map.find(recvcmd);
	if (iter == _cmd_event_map.end()) {
		this->_cmd_event_map.insert(std::pair<lw_int32, SocketRecvCallback>(recvcmd, cb));
	}
	else {
		iter->second = cb;
	}
	int c = this->_iobuffer->send(cmd, object, objectSize, [this](NET_MESSAGE * p) -> lw_int32 {
		int c1 = bufferevent_write(this->_bev, p->buf, p->size);
		if (c1 != 0) {
			LOGFMTD("error occurred. [%d]", c1);
		}
		return c1;
	});

	return c;
}

void SocketSession::__onWrite() {
	struct evbuffer *output = bufferevent_get_output(this->_bev);
	size_t output_len = evbuffer_get_length(output);
	if (output_len > 0) {
		LOGFMTD("SocketSession::__onRead. output_len: %d", output_len);
// 		std::unique_ptr<lw_char8[]> recv_buf(new lw_char8[output_len]);
// 		while (output_len > 0) {
// 			size_t write_len = bufferevent_read(this->_bev, recv_buf.get(), output_len);
// 			if (write_len > 0) {
// 				int c1 = bufferevent_write(this->_bev, recv_buf.get(), write_len);
// 				if (c1 != 0) {
// 					LOGFMTD("error occurred. [%d]", c1);
// 				}
// 			}
// 			else {
// 				LOGFMTD("SocketSession::__onRead. read_len: %d", write_len);
// 			}
// 			output_len -= write_len;
// 		}
	} 
	else {
	
	}
}

void SocketSession::__onRead() {
	struct evbuffer *input = bufferevent_get_input(this->_bev);
	size_t input_len = evbuffer_get_length(input);

	std::unique_ptr<lw_char8[]> recv_buf(new lw_char8[input_len]);
	while (input_len > 0) {
		size_t recv_len = bufferevent_read(this->_bev, recv_buf.get(), input_len); 
		if (recv_len > 0 && recv_len == input_len) {
			int c = this->_iobuffer->parse(recv_buf.get(), recv_len, SessionCore::__on_socket_data_parse_cb, this);
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
		if (this->parseHandler != nullptr) {
			this->parseHandler(this, cmd, buf, bufsize);
		}

// 		this->_handler->onSocketParse(this, cmd, buf, bufsize);
	}
}

void SocketSession::__onEvent(short ev) {
	if (ev & BEV_EVENT_CONNECTED) {
		this->_connected = true;
		if (this->connectedHandler != nullptr) {
			this->connectedHandler(this);
		}
// 		this->_handler->onSocketConnected(this);
		return;
	}

	if (ev & BEV_EVENT_READING) {
		if (this->errorHandler != nullptr) {
			this->errorHandler(this);
		}
		
// 		this->_handler->onSocketError(this);
	}
	else if (ev & BEV_EVENT_WRITING) {
		if (this->errorHandler != nullptr) {
			this->errorHandler(this);
		}
		
// 		this->_handler->onSocketError(this);
	}
	else if (ev & BEV_EVENT_EOF) {
		if (this->disConnectHandler != nullptr) {
			this->disConnectHandler(this);
		}
		
// 		this->_handler->onSocketDisConnect(this);
	}
	else if (ev & BEV_EVENT_TIMEOUT) {
		if (this->timeoutHandler != nullptr) {
			this->timeoutHandler(this);
		}
		
// 		this->_handler->onSocketTimeout(this);
	}
	else if (ev & BEV_EVENT_ERROR) {
		if (this->errorHandler != nullptr) {
			this->errorHandler(this);
		}
		
// 		this->_handler->onSocketError(this);
	}

	this->_connected = false;
	this->_bev = nullptr;
}
