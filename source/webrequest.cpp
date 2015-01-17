#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "webrequest.h"
#include "ssl_cert_bin.h"
#include "gui.h"
#include "utils.h"

u32 CWebRequest::client = 0;
u32 CWebRequest::mem=0;
extern FS_archive sdmcArchive;
//---------------------------------------------------------------------------
CWebRequest::CWebRequest()
{
#ifdef USE_CYASSL
	sk=-1;
	_buf=NULL;
	inet_scheme = inet_host = inet_path = NULL;
	bytesIn = 0;
	ctx = NULL;
	ssl = NULL;
#endif	
}
//---------------------------------------------------------------------------
CWebRequest::~CWebRequest()
{
	destroy();
}
//---------------------------------------------------------------------------
int CWebRequest::destroy()
{
#ifdef USE_CYASSL
	if(ssl != NULL){
		SSL_shutdown(ssl);
		SSL_free(ssl);   
		ssl = NULL;
	}
	if(ctx != NULL){
		SSL_CTX_free(ctx);
		ctx = NULL;
	}
	if(sk != -1){
		shutdown(sk,SHUT_RDWR);
		closesocket(sk);
		sk=-1;
	}
	if(_buf != NULL){
		free(_buf);
		_buf = NULL;
	}
	inet_scheme = inet_host = inet_path = NULL;
	headers.clear();
	response.clear();
	cookies.clear();
	bytesIn = 0;
#endif	
	return 0;
}	
//---------------------------------------------------------------------------
int CWebRequest::get_StatusCode()
{
	return -1;
}
//---------------------------------------------------------------------------
int CWebRequest::begin(char *url)
{
	char *p,*p1;
	int len,mode;
	
	if(!url || !url[0])
		return -1;
	destroy();
#ifdef USE_CYASSL	
	len = strlen(url);
	_buf = (char *)malloc(4096+(len+10)*2);
	if(_buf == NULL)
		return -2;
	inet_scheme = (char *)&_buf[4096];
	p1 = inet_scheme + len + 10;
	strcpy(p1,url);
	mode = 0;
	while((p = strtok(p1,"/")) != NULL){
		len = strlen(p);
		switch(mode){
			case 0:
				mode++;
				if(p[len-1]==':'){
					p[len-1]=0;
					strcpy(inet_scheme,p);
					inet_host = inet_scheme + len;
					len++;
				}
				else{
					inet_host = inet_scheme+1;
					inet_scheme[0] = 0;
					continue;
				}
			break;
			case 1:
				mode++;
				strcpy(inet_host,p);
				inet_path = inet_host+len+1;
				inet_path[0]='/';
				inet_path[1]=0;
			break;
			case 2:
				if(inet_path[1])
					strcat(inet_path,"/");
				strcat(inet_path,p);
			break;
		}
		p1 = p + len+1;
	}
	if(inet_path == NULL || !inet_path[0])
		return -3;
#endif		
	return 0;
}
//---------------------------------------------------------------------------
int CWebRequest::add_header(char *key,char *value)
{
#ifdef USE_CYASSL
	std::string s;
	
	if(!key || !key[0])
		return -1;
	if(!value || !value[0])
		return -2;	
	s = "";
	for(;*key != 0;key++)
		s += tolower(*key);		
	headers[s] = value;
	return 0;
#endif	
}
//---------------------------------------------------------------------------
int CWebRequest::send(int mode)
{
#ifdef USE_CYASSL
	struct hostent *h;
	struct sockaddr_in srv_addr;
	int res,i;
	
	if(inet_path == NULL || !inet_path[0] || _buf == NULL)
		return -1;
	res = -2;
	if((h = gethostbyname(inet_host)) == NULL)
		goto send_error;
	res--;
	sk = socket(PF_INET,SOCK_STREAM,0);
	if(sk == 0)
		goto send_error;
	res--;
	memset(&srv_addr,0,sizeof(srv_addr));
	srv_addr.sin_addr.s_addr = *((unsigned long *)h->h_addr_list[0]);
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(443);
	if(connect(sk,(struct sockaddr *)&srv_addr,sizeof(srv_addr)))
		goto send_error;
	res--;//-5
	method = CyaTLSv1_2_client_method();
	if(method == NULL)
		goto send_error;
	res--;
	ctx = SSL_CTX_new(method);
	if(ctx == NULL)
		goto send_error;
	res--;
	if(CyaSSL_CTX_load_verify_buffer(ctx,ssl_cert_bin,ssl_cert_bin_size,1) != SSL_SUCCESS)
		goto send_error;
	res--;
	CyaSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, myDateCb);
	res--;//-9
	ssl = CyaSSL_new(ctx);
	if(ssl == NULL)
		goto send_error;
	res--;
	CyaSSL_set_fd(ssl, sk);
	if(CyaSSL_connect(ssl) != SSL_SUCCESS)
		goto send_error;
	res--;
	if(request(mode))
		goto send_error;
	res--;
	i = SSL_write(ssl,_buf,strlen(_buf));
	if(!i)
		goto send_error;
	res--;
	i = SSL_read(ssl,_buf,4096);
	if(i < 1)
		goto send_error;
	{		
		Handle sram;
			
		Result res = FSUSER_OpenFile(NULL,&sram,sdmcArchive,FS_makePath(PATH_CHAR,"/lino.txt"),FS_OPEN_CREATE|FS_OPEN_WRITE,FS_ATTRIBUTE_NONE);
		if (res == 0){
			u32 byteswritten = 0;
			FSFILE_Write(sram, &byteswritten, 0, (u32*)_buf, i, FS_WRITE_FLUSH);
			FSFILE_Close(sram);
		}
	}
	bytesIn = i;
	res--;
	i = parse_response();
	if(i < 1)
		goto send_error;
	res--;
	return 0;	
send_error:
	destroy();
	return res;
#endif	
}
//---------------------------------------------------------------------------
int CWebRequest::parse_response()
{
	char *p,*p1,*p2;
	int len,parsed;

	if(!bytesIn || !_buf)
		return -1;
	p = _buf;
	parsed = 0;
	while((p1 = strtok(p,"\n")) != NULL){
		len = strlen(p1);
		if(p1[len-1] == '\r')
			p1[len-1] = 0;
		if(!p1[0])
           break;
		p = p1 + len + 1;
		p2 = NULL;
		if((p1 = strtok(p1,":")) != NULL)
			p2 = p1 + strlen(p1) + 1;
		if(!parsed){
			if(p2 && p2[0])
				return -2;
			response["status-code"] = trim(p2);
		}
		else{
			if(stricmp(p1,"set-cookie")==0){
				int i = 0;

				while((p1 = strtok(p2,";")) != NULL){
					char *p3;
	
					if(i)
						break;
					p2 = p1 + strlen(p1) + 1;
					i++;
					while((p3 = strtok(p1,"=")) != NULL){
						p3 = p1 + strlen(p1) + 1;
						cookies[trim(p1)] = trim(p3);
						break;
					}
				}
			}
			else
				response[trim(p1)] = trim(p2);
		}
		parsed += len+1;
	}
	return parsed;
}
//---------------------------------------------------------------------------
int CWebRequest::request(int mode)
{
	strcpy(_buf,"GET / HTTP/1.0\r\n");
	strcat(_buf,"Host: ");
	strcat(_buf,inet_host);
	strcat(_buf,"\r\n");
	strcat(_buf,"Connection: close\r\n");	
	strcat(_buf,"User-Agent: Opera/9.50 (Windows NT 5.1; U; it)\r\n");
	strcat(_buf,"Accept: */*\r\n");
	strcat(_buf,"Content-Length: 0\r\n");
	strcat(_buf,"\r\n\r\n");
	return 0;
}
//---------------------------------------------------------------------------
int CWebRequest::myDateCb(int preverify, CYASSL_X509_STORE_CTX* store)
{
	return 1;
}
//---------------------------------------------------------------------------
int CWebRequest::InitializeClient()
{
#ifdef USE_CYASSL
	if(!(CWebRequest::client&1)){
		u32 mem;
		int ret;
		
		mem = (u32)malloc(0x100000+0x1000);
		if(!mem)
			return -1;
		CWebRequest::mem = mem;
		mem = (mem + 0xfff) & ~0xfff;
		ret = SOC_Initialize((u32 *)mem,0x100000);
		if(ret){
			free((void *)CWebRequest::mem);
			CWebRequest::mem = 0;
			return -2;
		}		
		CWebRequest::client |= 1;
	}
	if(!(CWebRequest::client & 2)){
		CyaSSL_Init();
		CWebRequest::client |= 2;
	}
#endif
	return 0;
}
//---------------------------------------------------------------------------
int CWebRequest::DestroyClient()
{
#ifdef USE_CYASSL
	if(CWebRequest::client & 2){
		CyaSSL_Cleanup();
		CWebRequest::client &= ~2;
	}
	if((CWebRequest::client & 1)){
		SOC_Shutdown();
		if(CWebRequest::mem){
			free((void *)CWebRequest::mem);
			CWebRequest::mem = 0;
		}
		CWebRequest::client &= ~1;
	}
#endif
	return 0;
}