

#ifndef __MT6516_META__
#define __MT6516_META__

#include    <mach/mt6516_typedefs.h>
#include    <mach/mt6516_reg_base.h>




typedef enum
{
    META_OTHER		=	0,
    META_AP,
    META_MODEM,
    META_CAMERA,
    //------------------------
    // add new meta module here
    // .....
    // .....
    //------------------------
    META_MODULE_COUNT
} meta_module;

typedef bool (*meta_callback)(void); // type-definition: 'meta_callback' now can be used as type



typedef struct
{	char name[20];
	meta_callback callback; // point to the test function
    BOOL enable; //turn on / off this unit test
} meta_unit_test;



BOOL MT6516_META_Register(meta_module module,meta_callback func,const char* name);
BOOL MT6516_META_Unregister(meta_module module,meta_callback func,const char* name);

#endif /* __MT6516_META__ */


