
 
#ifndef _MEMORY_DUMMY_H_
#define _MEMORY_DUMMY_H_

#define MEM_DUMMY_VA2PA (0x1)

typedef struct VAtoPA
{
    unsigned long   va;
    unsigned long   pa;
} VAtoPA;

extern unsigned long gDummyFlag;
#define M3D_DEBUG_DISABLE_LCD               (0x00000040)

#endif //_MEMORY_DUMMY_H_

