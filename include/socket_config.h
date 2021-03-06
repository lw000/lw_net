#ifndef __socket_config_h__
#define __socket_config_h__

#include <string>
#include "socket_object.h"

class SocketConfig : public SocketObject
{
public:
	SocketConfig() {
		this->_port = -1;
	}

	SocketConfig(const std::string& host, int port) {
		this->_host = host;
		this->_port = port;
	}

	virtual ~SocketConfig() {

	}

public:
	void setHost(const std::string& host) {
		if (this->_host.compare(host) != 0) {
			this->_host = host;
		}
	}

	std::string getHost() const {
		return this->_host;
	}

	void setPort(int port) {
		this->_port = port;
	}

	int getPort() const {
		return this->_port;
	}

	void reset() {
		this->_port = 0;
		_host.clear();
	}

public:
	virtual std::string debug() override {
		char buf[256];
		sprintf(buf, "host:%s, port:%d", this->_host.c_str(), _port);
		return std::string(buf);
	}

private:
	std::string _host;
	int _port;
};

#endif // !__socket_config_h__
