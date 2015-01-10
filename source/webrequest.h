#include <3ds.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <string>
#ifdef USE_CYASSL
#include <cyassl/ctaocrypt/settings.h>
#include <cyassl/openssl/ssl.h>
#endif

#ifndef __CWEBREQUEST__
#define __CWEBREQUEST__

class CWebRequest{
public:
	CWebRequest();
	virtual ~CWebRequest();
	int get_StatusCode();
	int begin(char *url);
	int add_header(char *key,char *value);
	int send(int mode=0);
	static int InitializeClient();
	static int DestroyClient();
	static int myDateCb(int preverify, CYASSL_X509_STORE_CTX* store);
protected:
	int destroy();
#ifdef USE_CYASSL
	int parse_response();
	int request(int mode=0);
	std::map<std::string,std::string>headers;
	std::map<std::string,std::string>response;
	std::map<std::string,std::string>cookies;

	int sk;
	char *inet_scheme,*inet_host,*inet_path,*_buf;
	CYASSL_METHOD *method;
	CYASSL_CTX *ctx;
	CYASSL *ssl;
	u32 bytesIn;
#endif
public:
	static u32 client,mem;
};

#endif