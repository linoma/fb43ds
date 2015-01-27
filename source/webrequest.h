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
	int get_statuscode(u32 *ret);
	int get_responseheader(char *key,char *buf,u32 size);
	int begin(char *url);
	int add_header(char *key,char *value);
	int add_postdata_raw(char *data,u32 size,u32 flags=0);
	int add_postdata(char *key,char *value,u32 flags=0);
	int send(int mode=RQ_GET);
	int close();
	int download_data(char *buf,u32 size,u32 *ret=NULL);
	static int InitializeClient();
	static int DestroyClient();
	static int myDateCb(int preverify, CYASSL_X509_STORE_CTX* store);
	enum {RQ_GET=0,RQ_POST=1,RQ_FORMDATA=2};
	
protected:
	int destroy();
#ifdef USE_CYASSL
	int parse_response();
	int request(int mode=0);
	std::map<std::string,std::string>headers;
	std::multimap<std::string,std::string>response;

	int sk;
	char *inet_scheme,*inet_host,*inet_path,*_buf,*_postdata;
	CYASSL_METHOD *method;
	CYASSL_CTX *ctx;
	CYASSL *ssl;
	u32 bytesIn,_postdata_len,_postdata_size;
#endif
public:
	static u32 client,mem;
};

#endif