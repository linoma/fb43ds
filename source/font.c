#include <3ds.h>
#include "font.h"
#include "font_bin.h" 
font_s fontDefault =
{
	font1Data,
	font1Desc,
	14,
	(u8[]){0xFF,0xFF,0xFF}
};

font_s fontBlack =
{
	font1Data,
	font1Desc,
	14,
	(u8[]){0x00,0x00,0x00}
};


font_s fontBlackHeader =
{
	RobotoData,
	RobotoDesc,
	24,
	(u8[]){0x00,0x00,0x00}
};

font_s fontBlackSubHeader =
{
	robotoSmallData,
	robotoSmallDesc,
	18,
	(u8[]){0x40,0x40,0x40}
};

font_s fontWhiteHeader =
{
	RobotoData,
	RobotoDesc,
	24,
	(u8[]){0xFF,0xFF,0xFF}
};