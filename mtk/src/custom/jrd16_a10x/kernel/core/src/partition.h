
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#define KB  (1024)
#define MB  (1024 * KB)
#define GB  (1024 * MB)

#define PART_SIZE_PRELOADER         (128*KB)
#define PART_SIZE_UBOOT  	    (128*3*KB)
#define PART_SIZE_SECURE  	    (128*KB)
#define PART_SIZE_LOGO   	    (3*MB)

//--------------------------------------------
// Android Boot Image
#define PART_SIZE_BOOTIMIG 	    (4*MB)
//--------------------------------------------

#define PART_SIZE_RECOVERY          (4*MB)
#define PART_SIZE_SECSTATIC         (1*MB+128*KB)
#define PART_SIZE_SYSIMG 	    (70*MB)
#define PART_SIZE_CACHE  	    (60*MB)
#define PART_SIZE_NVRAM  	    (3*MB)
#define PART_SIZE_MISC		    (512*KB-128*KB)
#define PART_SIZE_EXPDB   	    (640*KB)
/* modify by xuxian@jrdcom.com 20101015, add two partitions retrofit and trace at the end of flash --start */
#define PART_SIZE_USER		    (230*MB)
#define PART_SIZE_CUSTPACK	    (130*MB)
#define PART_SIZE_RETROFIT    (3*MB)
/* modify by xuxian@jrdcom.com 20101015, add two partitions retrofit and trace at the end of flash --end */

/*=======================================================================*/
/* NAND PARTITION Mapping                                                  */
/*=======================================================================*/
#ifdef CONFIG_MTD_PARTITIONS

static struct mtd_partition g_pasStatic_Partition[] = {
    { 
        .name   = "preloader",
        .offset = 0x0,
        .size   = PART_SIZE_PRELOADER,
        .mask_flags = MTD_WRITEABLE,
    },
    { 
        .name   = "nvram",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_NVRAM,
    },
    { 
        .name   = "seccnfg",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_SECURE,
        .mask_flags = MTD_WRITEABLE,
    },          
    { 
        .name   = "uboot",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_UBOOT,
        .mask_flags = MTD_WRITEABLE,
    },     
    { 
        .name   = "boot",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_BOOTIMIG,        
    },
    { 
        .name   = "recovery",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_RECOVERY,
    },
    { 
        .name   = "secstatic",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_SECSTATIC,
    },      
    { 
        .name   = "misc",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_MISC,
    },
    { 
        .name   = "system",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_SYSIMG,
    },
    { 
        .name   = "cache",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_CACHE,
    },    
    { 
        .name   = "logo",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_LOGO,
        .mask_flags = MTD_WRITEABLE,
    },    
    { 
        .name   = "expdb",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_EXPDB,
    },
/* add by xuxian@jrdcom.com 20101028, add custpack partition --start */    
    { 
        .name   = "custpack",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_CUSTPACK,
    },
/* add by xuxian@jrdcom.com 20101028, add custpack partition --end */  		
/* modify by xuxian@jrdcom.com 20101015, add two partitions retrofit and trace at the end of flash --start */
    { 
        .name   = "userdata",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_USER,
    }, 
     { 
        .name   = "retrofit",
        .offset = MTDPART_OFS_APPEND,
        .size   = PART_SIZE_RETROFIT,
    },   
     { 
        .name   = "trace",
        .offset = MTDPART_OFS_APPEND,
        .size   = MTDPART_SIZ_FULL,
    },  
/* modify by xuxian@jrdcom.com 20101015, add two partitions retrofit and trace at the end of flash --end */
};

#define NUM_PARTITIONS ARRAY_SIZE(g_pasStatic_Partition)
#endif
