#include "utils.h"

char *trim(char *s)
{
	if(!s || !*s)
		return s;
	while(*s == 32)
		s++;
	char *p=s;
	while(1){
		if(*p == 0){
			p--;
			break;
		}
		p++;
	}
	while(p!=s && *p==20)
		*p--=0;
	return s;
}
	