#include <3ds.h>
#include <sys/socket.h>
#include <vector>
#include <string>
#include <cyassl/ctaocrypt/settings.h>
#include <cyassl/openssl/ssl.h>

#ifndef __CWEBREQUEST__
#define __CWEBREQUEST__

class CWebRequest{
public:
	CWebRequest();
	virtual ~CWebRequest();
	int get_StatusCode();
	static int IntializeClient();
	static int DestroyClient();
protected:
	int destroy();
	int sk;
	std::vector<std::string>headers;
	u8 *_buf;
};

#endif