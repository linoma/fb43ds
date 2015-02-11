#include <3ds.h>
#include <vector>
#include "types.h"
#include "images.h"

#ifndef __FBH__
#define __FBH__

int fb_init(u32 arg0);

class CFBClient{
public:
	CFBClient();
	virtual ~CFBClient();
	static int Initialize();
	static int Destroy();
	static int onTouchEvent(touchPosition *p,u32 flags);
	static int onMainLoop();
	static int get_post_form_id(u32 arg0);
	int SetTimer(CTimer *p);
	int onTimersLoop();
	int set_Cookies(char *str);
	int get_Cookies(char *str,u32 size,u32 flags=0);
	int add_Cookie(char *key,char *value);
	int get_Cookie(char *key,char *str,u32 size);
	const char *get_Email(){return email.c_str();};
	const char *get_Password(){return pass.c_str();};
	int set_Email(char *str);
	int set_Password(char *str);
	int Main(u32 arg0);
	int onClicked(u32 id);
	int onTimers(u32 id);
	int on_get_post_form_id(u8 *data,u32 size);
protected:
	int SetTimer(LPDEFFUNC f,u64 val,u32 p);
	std::vector<CTimer *>timers;
	std::map<std::string,std::string>cookies;
	std::string email,pass;
	int mode;
};
extern CFBClient *fb;
#endif