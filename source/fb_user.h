#include <3ds.h>
#include <string>
#include "jpeg-image.h"
#include "widgets.h"

#ifndef __FBUSERH__
#define __FBUSERH__

class CUser : public CImageJpeg{
public:
   CUser(const char *cid);
   virtual ~CUser();
   int set_Active(int val);
   int get_info();
   static int get_info_user(u32 arg0);
   int is_Ready(){return (status & 2) != 0;};
   const char *get_ID(){return id;};
   const char *get_Name(){return name;};
   int draw(LPRECT prc,u8 *screen);
protected:
   unsigned long status;
   char id[31],*name,*thumbSrc;
   CWindow *win;
};

#endif