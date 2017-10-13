#ifndef __object_h__
#define __object_h__

#include <string>
#include <iosfwd>

class noncopyable
{
protected:
	noncopyable() {}
	~noncopyable() {}

private:  // emphasize the following members are private  
	noncopyable(const noncopyable&);
	const noncopyable& operator=(const noncopyable&);
};

class Object
{
// 	friend std::ostream& operator<<(std::ostream & os, Object & o);

public:
	Object();
	virtual ~Object();

public:
	void setUserData(void* data);
	void* getUserData() const;

	void setTag(int tag);
	int getTag() const;

	void setName(const std::string& name);
	std::string getName() const;

public:
	virtual std::string debug();

private:
	void *_data;
	int _tag;
	std::string _name;
};


#endif // !__object_h__