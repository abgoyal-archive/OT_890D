

#ifndef _CUST_KPD_H_
#define _CUST_KPD_H_
#include <linux/input.h>
#include <cust_eint.h>
#define KPD_YES		1
#define KPD_NO		0
/* available keys (Linux keycodes) */
#define KEY_CALL	KEY_SEND
#define KEY_ENDCALL	KEY_END
#undef KEY_OK
#define KEY_OK		KEY_REPLY	/* DPAD_CENTER */
#define KEY_FOCUS	KEY_HP
#define KEY_AT		KEY_EMAIL
#define KEY_POUND	228	//KEY_KBDILLUMTOGGLE
#define KEY_STAR	227	//KEY_SWITCHVIDEOMODE
#define KEY_DEL 	KEY_BACKSPACE
#define KEY_SYM		KEY_COMPOSE
/* KEY_HOME */
/* KEY_BACK */
/* KEY_VOLUMEDOWN */
/* KEY_VOLUMEUP */
/* KEY_MUTE */
/* KEY_MENU */
/* KEY_UP */
/* KEY_DOWN */
/* KEY_LEFT */
/* KEY_RIGHT */
/* KEY_CAMERA */
/* KEY_POWER */
/* KEY_TAB */
/* KEY_ENTER */
/* KEY_LEFTSHIFT */
/* KEY_COMMA */
/* KEY_DOT */		/* PERIOD */
/* KEY_SLASH */
/* KEY_LEFTALT */
/* KEY_RIGHTALT */
/* KEY_SPACE */
/* KEY_SEARCH */
/* KEY_0 ~ KEY_9 */
/* KEY_A ~ KEY_Z */


#define KPD_KEY_DEBOUNCE  1024      /* (val / 32) ms */
#define KPD_PWRKEY_MAP    KEY_POWER

#define KPD_PWRKEY_USE_EINT       KPD_YES
#define KPD_PWRKEY_EINT           CUST_EINT_KPD_PWRKEY_NUM
#define KPD_PWRKEY_DEBOUNCE       CUST_EINT_KPD_PWRKEY_DEBOUNCE_CN
#define KPD_PWRKEY_POLARITY       CUST_EINT_KPD_PWRKEY_POLARITY
#define KPD_PWRKEY_SENSITIVE      CUST_EINT_KPD_PWRKEY_SENSITIVE

/* HW keycode [0 ~ 71] -> Linux keycode */
#define KPD_INIT_KEYMAP()	\
{	\
	[10] = KEY_VOLUMEUP,		\
	[11] = KEY_CALL,		\
	[19] = KEY_VOLUMEDOWN,		\
	[20] = KEY_HOME,		\
	[27] = KEY_OK,		\
	[30] = KEY_CAMERA,		\
	[36] = KEY_1,		\
	[37] = KEY_2,		\
	[38] = KEY_3,		\
	[39] = KEY_CALL,		\
	[40] = KEY_BACK,		\
	[41] = KEY_MENU,		\
	[42] = KEY_HOME,		\
	[45] = KEY_4,		\
	[46] = KEY_5,		\
	[47] = KEY_6,		\
	[48] = KEY_ENDCALL,		\
	[50] = KEY_UP,		\
	[54] = KEY_7,		\
	[55] = KEY_8,		\
	[56] = KEY_9,		\
	[58] = KEY_LEFT,		\
	[59] = KEY_OK,		\
	[60] = KEY_RIGHT,		\
	[63] = KEY_STAR,		\
	[64] = KEY_0,		\
	[65] = KEY_POUND,		\
	[68] = KEY_DOWN,		\
}	 
#endif


