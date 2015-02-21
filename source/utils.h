#include <3ds.h>

#ifndef __UTILSH__
#define __UTILSH__

char *trim(char *s);
char *strtolower(char *s);
char *strtoupper(char *s);
char *ucwords(char *s);
int urlencode(char *src,char *dst);
int printd(char *fmt,...);
u32 write_to_sdmc(const char *filename,u8 *_buf,u32 size);

class buffer{
public:
	buffer();
	virtual ~buffer();
	char *data(){return _buf;};
	u32 size(){return _size;};
	u32 len(){return _bytes;};
	int copy(char *data,u32 size,u32 flags = 0);
	u32 free(){return _size - _bytes;};
	int destroy();
	int alloc(u32 sz);
protected:
	u32 _size,_bytes;
	char *_buf;
};
#endif