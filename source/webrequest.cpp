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
	_buf = _postdata = NULL;
	inet_scheme = inet_host = inet_path = NULL;
	bytesIn = _postdata_len = _postdata_size = 0;
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
		bytesIn = 0;
	}
	if(_postdata != NULL){
		free(_postdata);
		_postdata = NULL;
		_postdata_len = _postdata_size = 0;
	}
	inet_scheme = inet_host = inet_path = NULL;
	headers.clear();
	response.clear();
	bytesIn = 0;
#endif	
	return 0;
}
//---------------------------------------------------------------------------
int CWebRequest::close()
{
	return destroy();
}
//---------------------------------------------------------------------------
int CWebRequest::get_responseheader(char *key,char *buf,u32 size)
{
	int len;
	
	if(!key || !*key)
		return 0;
	if(response.count(key) == 0)
		return 0;
	std::string s = "";
	std::multimap<std::string,std::string>::iterator it;
    for (len=0,it=response.equal_range(key).first; it != response.equal_range(key).second; ++it,len++){
		if(len)
			s += "\r\n";
		s += (*it).second;
	}
	len = s.length();
	if(buf == NULL || size < len)
		return len;
	strcpy(buf,s.c_str());
	return len;
}
//---------------------------------------------------------------------------
int CWebRequest::get_statuscode(u32 *ret)
{
#ifdef USE_CYASSL
	char *p,*s,*p1;
	int mode;
	
	if(!ret)
		return -1;
	if(response.count("status-code") == 0)
		return -2;
	std::string c = response.find("status-code")->second;
	p = (char *)malloc(c.length()+2);
	if(!p)
		return -3;
	strcpy(p,c.c_str());
	s=p;
	mode=0;
	while((p1 = strtok(s," "))){
		mode++;
		if(mode == 2){
			*ret = atoi(trim(p1));
			break;
		}
		s = NULL;
	}
	free(p);
	return 0;
#endif
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
	_buf = (char *)malloc(4097+(len+10)*2);
	if(_buf == NULL)
		return -2;
	inet_scheme = (char *)&_buf[4097];
	memset(inet_scheme,0,(len+10)*2);
	p1 = inet_scheme + len + 10;
	strcpy(p1,url);
	mode = 0;
	while((p = strtok(p1,"/")) != NULL){
		len = strlen(p);
		switch(mode){
			case 0:
				mode++;
				if(p[len-1]==':'){
					p[len-1] = 0;
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
	if(!key || !key[0])
		return -1;
	if(!value || !value[0])
		return -2;			
	headers[key] = value;
	return 0;
#endif	
}
//---------------------------------------------------------------------------
int CWebRequest::add_postdata_raw(char *data,u32 size,u32 flags)
{
#ifdef USE_CYASSL
	if(!data || !size)
		return -1;
	if(flags & 1){
		if(_postdata != NULL){
			free(_postdata);
			_postdata_size = _postdata_len = 0;
			_postdata = NULL;
		}
	}
	u32 sz = _postdata_len + size;
	if(sz > _postdata_size){
		sz = ((sz >> 10) + 1) << 10;
		char *p = (char *)malloc(_postdata_size + sz);
		if(p == NULL)
			return -2;
		memcpy(p,_postdata,_postdata_len);
		free(_postdata);
		_postdata = p;
		_postdata_size += sz;
	}
	memcpy(&_postdata[_postdata_len],data,size);
	_postdata_len += size;
	return 0;
#endif		
}
//---------------------------------------------------------------------------
int CWebRequest::add_postdata(char *key,char *value,u32 flags)
{
#ifdef USE_CYASSL
	int len,l0,l1,ret;
	char *p,*p1;
	
	if(!key || !*key)
		return -1;
	if(!value || !*value)
		return -2;
	l0 = strlen(key);
	l1 = strlen(value);
	len = l0+l1+1;
	if(_postdata_len)
		len++;
	p = (char *)malloc(len+1);
	if(p == NULL)
		return -3;
	p1 = p;
	if(_postdata_len)
		*p1++ ='&';
	strcpy(p1,key);
	p1 += l0;
	*p1++ = '=';
	strcpy(p1,value);
	ret = add_postdata_raw(p,len,flags);
	free(p);
	return ret;
#endif
}
//---------------------------------------------------------------------------
int CWebRequest::download_data(char *buf,u32 size,u32 *pret)
{
#ifdef USE_CYASSL
	int err,r;
	u32 ret,sz;
	
	if(!buf || !size || !_buf)
		return -1;
	err=0;
	ret=0;
	while(size > 0 && !err){
		if(!bytesIn){
			while(bytesIn < 4096 && !err){
				r = SSL_read(ssl,_buf+bytesIn,4096-bytesIn);
				switch(SSL_get_error(ssl,r)){
					case SSL_ERROR_NONE:
						bytesIn += r;
					break;
					case SSL_ERROR_ZERO_RETURN:
						err=1;
					break;
					case SSL_ERROR_SYSCALL:
					default:
						printd("erro %d",r);
					break;
				}
			}
		}
		sz = size>bytesIn ? bytesIn : size;
		memcpy(buf,_buf,sz);
		bytesIn -= sz;
		size -= sz;
		buf += sz;
		ret += sz;
	}
	if(pret)
		*pret = ret;
	return 0;
#endif	
}
//---------------------------------------------------------------------------
int CWebRequest::send(int mode)
{
#ifdef USE_CYASSL
	struct hostent *h;
	struct sockaddr_in srv_addr;
	int res,i,err;
	
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
	res--;//-10
	CyaSSL_set_fd(ssl, sk);
	if(CyaSSL_connect(ssl) != SSL_SUCCESS)
		goto send_error;
	res--;//-11
	if(request(mode))
		goto send_error;
	res--;//-12
	i = strlen(_buf);
	if(_postdata_len && (mode & RQ_POST)){
		memcpy(&_buf[i],_postdata,_postdata_len);
		i += _postdata_len;
	}
	i = SSL_write(ssl,_buf,i);
	if(!i)
		goto send_error;
	res--;//-13
	err=0;
	while(bytesIn < 4096 && !err){
		i = SSL_read(ssl,_buf+bytesIn,4096-bytesIn);
		switch(SSL_get_error(ssl,i)){
			case SSL_ERROR_NONE:
				bytesIn += i;
			break;
			case SSL_ERROR_ZERO_RETURN:
				err=1;
			break;
			case SSL_ERROR_SYSCALL:
			default:
			break;
		}
		_buf[bytesIn]=0;
		if(strstr(_buf,"\r\n\r\n") != NULL)
			break;
	}
	if(bytesIn < 1)
		goto send_error;
#ifdef _DEBUG
		if(mode & RQ_DEBUG){
			Handle sram;
				
			Result res = FSUSER_OpenFile(NULL,&sram,sdmcArchive,FS_makePath(PATH_CHAR,"/lino.txt"),FS_OPEN_CREATE|FS_OPEN_WRITE,FS_ATTRIBUTE_NONE);
			if (res == 0){
				u32 byteswritten = 0;
				FSFILE_Write(sram, &byteswritten, 0, (u32*)_buf, bytesIn, FS_WRITE_FLUSH);
				FSFILE_Close(sram);
			}		
		}
#endif
	res--;//14
	i = parse_response();
	if(i < 1)
		goto send_error;
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
		if(!len)
           break;
		if(p1[len-1] == '\r')
			p1[len-1] = 0;
		if(!p1[0]){
           parsed += len+1;
           break;
		}
		p = p1+len+1;
		p2 = NULL;
		if((p1 = strtok(p1,":")) != NULL)
			p2 = p1 + strlen(p1) + 1;
		if(!parsed){
			if(p2 && p2[0])
				return -2;
			response.insert(std::pair<std::string,std::string>("status-code", trim(p1)));
		}
		else
			response.insert(std::pair<std::string,std::string>(trim(p1), trim(p2)));
		parsed += len+1;
	}
	return parsed;
}
//---------------------------------------------------------------------------
int CWebRequest::request(int mode)
{	
	if(!(mode & RQ_POST))
		strcpy(_buf,"GET ");
	else
		strcpy(_buf,"POST ");	
	strcat(_buf,inet_path),
	strcat(_buf," HTTP/1.0\r\n");
	strcat(_buf,"Host: ");
	strcat(_buf,inet_host);
	strcat(_buf,"\r\n");
	strcat(_buf,"Connection: ");	
	if(headers.count("Connection") == 0)
		strcat(_buf,"close");
	strcat(_buf,"\r\nUser-Agent: Opera/9.50 (Windows NT 5.1; U; it)\r\n");
	strcat(_buf,"Accept: */*\r\n");
	if(headers.count("Cookie")){
		strcat(_buf,"Cookie:");
		strcat(_buf,headers["Cookie"].c_str());
		strcat(_buf,"\r\n");
	}
	if(mode & RQ_POST){
		if(_postdata){
			char c[100];

			if(!(mode & RQ_FORMDATA))
				strcat(_buf,"Content-Type: application/x-www-form-urlencoded\r\n");
			sprintf(c,"Content-Length: %lu", _postdata_len);
			strcat(_buf,c);
		}
	}
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