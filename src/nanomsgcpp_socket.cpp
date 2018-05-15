#include "nanomsgcpp_socket.h"

#include "Threadable.h"
#include "net_iobuffer.h"
#include "log4z.h"

#include <nn.h>

class NanomsgcppCore {
	public:
		static void __on_socket_recv(lw_int32 cmd, char* buf, lw_int32 bufsize,
				void* userdata) {
			NanomsgcppSocket * c = (NanomsgcppSocket*) userdata;
			if (c->_handler != nullptr) {
				c->_handler(cmd, buf, bufsize);
			}
			else {
				c->onRecv(cmd, buf, bufsize);
			}
		}
};

NanomsgcppSocket::NanomsgcppSocket() {
	this->_fd = -1;
	_iobuffer = new NetIOBuffer;
}

NanomsgcppSocket::~NanomsgcppSocket() {
	delete _iobuffer;
	close();
}

int NanomsgcppSocket::create(int domain, unsigned int protocol) {
	this->_fd = nn_socket(domain, protocol);
	return this->_fd;
}

int NanomsgcppSocket::bind(const char *addr) {
	int r = nn_bind(this->_fd, addr);
	if (r < 0) {
		nn_close(this->_fd);

		lw_char8 buf[512];
		sprintf(buf, "nn_bind: %s\n", nn_strerror(nn_errno()));
		LOGD(buf);

		return (-1);
	}
	return 0;
}

int NanomsgcppSocket::connect(const char *addr) {
	int r = nn_connect(this->_fd, addr);
	if (r < 0) {
		nn_close(this->_fd);

		lw_char8 buf[512];
		sprintf(buf, "nn_connect: %s\n", nn_strerror(nn_errno()));
		LOGD(buf);

		return (-1);
	}
	return 0;
}

int NanomsgcppSocket::setsockopt(int level, int option, const void *optval,
		size_t optvallen) {
	int r = nn_setsockopt(this->_fd, level, option, optval, optvallen);
	if (r < 0) {
		nn_close(this->_fd);

		lw_char8 buf[512];
		sprintf(buf, "nn_setsockopt: %s\n", nn_strerror(nn_errno()));
		LOGD(buf);
		return (-1);
	}

	return 0;
}

int NanomsgcppSocket::getsockopt(int level, int option, void *optval,
		size_t *optvallen) {
	int r = nn_getsockopt(this->_fd, level, option, optval, optvallen);
	return r;
}

int NanomsgcppSocket::close() {
	int r = nn_close(this->_fd);
	return r;
}

int NanomsgcppSocket::shutdown() {
	int r = nn_shutdown(this->_fd, 0);
	return r;
}

int NanomsgcppSocket::recv() {
	char *buf = NULL;
	lw_int32 c = nn_recv(this->_fd, &buf, NN_MSG, 0);
	if (c > 0) {
		_iobuffer->parse(buf, c, NanomsgcppCore::__on_socket_recv, this);
		nn_freemsg(buf);
	}
	return c;
}

int NanomsgcppSocket::recv(const NanomsgDataParseHandler& handler) {
	this->_handler = handler;
	char *buf = NULL;
	lw_int32 c = nn_recv(this->_fd, &buf, NN_MSG, 0);
	if (c > 0) {
		_iobuffer->parse(buf, c, NanomsgcppCore::__on_socket_recv, this);
		nn_freemsg(buf);
	}
	return c;
}

int NanomsgcppSocket::send(int cmd, void* object, int objectSize, int flags) {
	int c = _iobuffer->send(cmd, object, objectSize,
			[this](NET_MESSAGE * p) -> lw_int32
			{
				int c = nn_send(this->_fd, p->buf, p->size, 0);
				return c;
			});
	return c;
}

int NanomsgcppSocket::sendmsg(int cmd, void* object, int objectSize,
		const struct nn_msghdr *msghdr, int flags) {
	int c = _iobuffer->send(cmd, object, objectSize,
			[this, msghdr, flags](NET_MESSAGE * p) -> lw_int32
			{
				int c = nn_send(this->_fd, p->buf, p->size, 0);
				int rc = nn_sendmsg(this->_fd, msghdr, flags);
					return rc;
			});
	return c;
}

int NanomsgcppSocket::recvmsg(struct nn_msghdr *msghdr, int flags) {
	int rc = nn_recvmsg(this->_fd, msghdr, flags);
	return rc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ServerThread: public Threadable {
	private:
		int s;

	public:
		ServerThread(int s) :
				s(-1) {

		}

		virtual ~ServerThread() {

		}

		virtual int onStart() override {

			return 0;
		}

		virtual int onRun() override {

			return 0;
		}

		virtual int onEnd() override {

			return 0;
		}
};

NanomsgcppSocketServerThread::NanomsgcppSocketServerThread() {

}

NanomsgcppSocketServerThread::~NanomsgcppSocketServerThread() {

}

int NanomsgcppSocketServerThread::recv() {
	for (;;) {
		void *control = NULL;
		char *buf = NULL;
		struct nn_iovec iov;
		struct nn_msghdr hdr;
		memset(&hdr, 0, sizeof(hdr));
		iov.iov_base = &buf;
		iov.iov_len = NN_MSG;
		hdr.msg_iov = &iov;
		hdr.msg_iovlen = 1;
		hdr.msg_control = &control;
		hdr.msg_controllen = NN_MSG;

		int rc = recvmsg(&hdr, 0);

		if (rc < 0) {
			if (nn_errno() == EBADF) {
				return -1; /* Socket closed by another thread. */
			}

			/*  Any error here is unexpected. */
			fprintf(stderr, "nn_recv: %s\n", nn_strerror(nn_errno()));
			break;
		}

		_iobuffer->parse(buf, rc, NanomsgcppCore::__on_socket_recv, this);

		nn_freemsg(buf);
	}

	return 0;
}

int NanomsgcppSocketServerThread::recv(const NanomsgDataParseHandler& handler) {

	return 0;
}
