#ifndef __ObjectException_h__
#define __ObjectException_h__

#include <exception>
#include <string>

class ObjectException: public std::exception {
	public:
		ObjectException() :
				err(0) {
		}

		virtual const char *what() const throw () {
			return err.c_str();
		}

		int num() const {
			return err;
		}

	private:
		std::string errstr;
		int err;
};

#endif //__ObjectException_h__
