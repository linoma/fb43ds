#include <3ds.h>
#include <string>

#ifndef __FBUSERH__
#define __FBUSERH__

class CUser{
public:
   CUser(const char *cid);
   virtual ~CUser();
   int set_Active(int val);
   int get_info();
   static int get_info_user(u32 arg0);
   int is_Ready(){return (status & 2) != 0;};
protected:
   unsigned long status;
   char id[31];
   std::string name;
};

#endif