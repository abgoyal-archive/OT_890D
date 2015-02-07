


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <mach/irqs.h>
#include <mach/mt6516_gpio.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/jiffies.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_pll.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/wakelock.h>

#include <linux/pmic6326_sw.h>
#include <asm/tcm.h>

/*   below is Yusu sound driver   */
#include "mt6516_ana_regs.h"
#include "mt6516_asm_regs.h"
#include "yusu_audio_stream.h"
#include "Yusu_ioctl.h"
#include "yusu_asm.h"
#include "yusu_ana.h"
#include "yusu_android_headset.h"
#include "yusu_android_speaker.h"

// #define for internal sram usage
#define ASM_SRAM_START (0x40015000)
#define ASM_SRAM_END     (0x40017FFF)
#define ASM_SRAM_LENGTH (0x3000)
#define EDI_LENGTH_MAX (0x5000)

//#define CONFIG_DEBUG_MSG
#ifdef CONFIG_DEBUG_MSG
#define PRINTK(format, args...) printk( KERN_EMERG format,##args )
#else
#define PRINTK(format, args...)
#endif

#define ASM_WAIT_TIMEOUT (100)
#define EDI_WAIT_TIMEOUT (500)

typedef struct
{
    volatile UINT32 Suspend_ASM_IBUF_BASE0;
    volatile UINT32 Suspend_ASM_IBUF_END0;
    volatile UINT32 Suspend_ASM_SET0;
    volatile UINT32 Suspend_ASM_GAIN0;
    volatile UINT32 Suspend_ASM_IR_CNT0;

    volatile UINT32 Suspend_ASM_IBUF_BASE1;
    volatile UINT32 Suspend_ASM_IBUF_END1;
    volatile UINT32 Suspend_ASM_SET1;
    volatile UINT32 Suspend_ASM_GAIN1;
    volatile UINT32 Suspend_ASM_IR_CNT1;

    volatile UINT32 Suspend_ASM_IBUF_BASE2;
    volatile UINT32 Suspend_ASM_IBUF_END2;
    volatile UINT32 Suspend_ASM_SET2;
    volatile UINT32 Suspend_ASM_GAIN2;
    volatile UINT32 Suspend_ASM_IR_CNT2;

    volatile UINT32 Suspend_ASM_CON0;
    volatile UINT32 Suspend_ASM_CON1;
    volatile UINT32 Suspend_ASM_SIN_CON;
    volatile UINT32 Suspend_ASM_AFE_CON0 ;
    volatile UINT32 Suspend_ASM_AFE_CON1;
    volatile UINT32 Suspend_ASM_AFE_CON2;
    volatile UINT32 Suspend_AFE_EDI_CON;
    volatile UINT32 Suspend_ASM_EDI_BASE;
    volatile UINT32 Suspend_ASM_EDI_END;
    volatile UINT32 Suspend_ASM_EDI_CON1;
    volatile UINT32 Suspend_AFE_AAG_CON;
    volatile UINT32 Suspend_AFE_AAC_CON;
    volatile UINT32 Suspend_AFE_AAPDB_CON;
    volatile UINT32 Suspend_AFE_AAC_NEW_CON;

}_Suspend_reg;


static void magic_tasklet(unsigned long data );
DECLARE_TASKLET(magic_tasklet_handle, magic_tasklet, 0L);
DECLARE_WAIT_QUEUE_HEAD(Sound_Wait_Queue);
DECLARE_WAIT_QUEUE_HEAD(Edi_Wait_Queue);

static int Yusu_Sound_fasync(int fd, struct file *flip, int mode);
static int Yusu_Sound_remap_mmap(struct file *flip, struct vm_area_struct *vma);
static int Audio_Read_Procmem(char *buf,char **start, off_t offset, int count , int *eof, void *data);

extern void Yusu_Sound_AMP_Switch(BOOL enable);
extern void pmic_asw_bsel(asw_bsel_enum sel);
extern void pmic_spkr_enable(kal_bool enable);
extern void pmic_spkl_enable(kal_bool enable);
extern void pmic_spkl_vol(kal_uint8 val);
extern void pmic_spkr_vol(kal_uint8 val);
extern void MT6516_EINTIRQMask(unsigned int line);
extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void copy_to_user_fake( char* Read_Data_Ptr , int read_size);




static char sound_name[] = "Yusu_Sound_Driver";
static u64 Yusu_sound_dmamask =  0xffffffffUL;
static kal_int32 Mmap_index =-1;
static volatile kal_uint32 u4_Asm_Stat =0;
static kal_uint32 wait_queue_flag =0;
static kal_uint32 Edi_wait_queue_flag =0;
static spinlock_t sound_lock = SPIN_LOCK_UNLOCKED;
static ASM_CONTROL_T  Asm_Control_context;  // main control of asm
static ASM_CONTROL_T *Asm_Control = &Asm_Control_context;  // main control of asm

static EDI_CONTROL_T  Edi_Control_context;  // main control of asm
static EDI_CONTROL_T *Edi_Control = &Edi_Control_context;  // main control of asm

static kal_bool Asm_Amp_Flag = false; // asm_Amp_Flag means amp status , default to false
static _Suspend_reg Suspend_reg;
static kal_bool Suspen_Mode = false;
static int Asm_Memory_Count = 0;
static int Audio_Flush_Count =0;
static struct fasync_struct *Yusu_Sound_async = NULL;
static int Clock_Count =0;
static Audio_Control_T Audio_Control_State;
struct wake_lock Audio_suspend_lock;


struct platform_device Yusu_device_sound = {
	.name		  =sound_name,
	.id		  = 0,
	.dev              = {
		.dma_mask = &Yusu_sound_dmamask,
		.coherent_dma_mask =  0xffffffffUL
	}
};


static void Yusu_sound_Store_reg(void);
static void Yusu_sound_Recover_reg (void);
void Yusu_Sound_Power_On(void);
void Yusu_Sound_Power_Off(void);
void Yusu_Sound_Suspend_Power_Off(void);
void Yusu_Sound_Suspend_Power_On(void);
void Yusu_DeInit_InputStream(void);


static int Audio_Read_Procmem(char *buf,char **start, off_t offset, int count , int *eof, void *data)
{
    int len = 0;
    len += sprintf(buf+ len, "ASM_IBUF_BASE0 = %x\n",Asm_Get(ASM_IBUF_BASE0));
    len += sprintf(buf+ len, "ASM_IBUF_END0 = %x\n",Asm_Get(ASM_IBUF_END0));
    len += sprintf(buf+ len, "ASM_IBUF_CUR0 = %x\n",Asm_Get(ASM_IBUF_CUR0));
    len += sprintf(buf+ len, "ASM_SET0 = %x\n",Asm_Get(ASM_SET0));
    len += sprintf(buf+ len, "ASM_GAIN0 = %x\n",Asm_Get(ASM_GAIN0));
    len += sprintf(buf+ len, "ASM_IR_CNT0 = %x\n",Asm_Get(ASM_IR_CNT0));

    len += sprintf(buf+ len, "ASM_IBUF_BASE1 = %x\n",Asm_Get(ASM_IBUF_BASE1));
    len += sprintf(buf+ len, "ASM_IBUF_END1 = %x\n",Asm_Get(ASM_IBUF_END1));
    len += sprintf(buf+ len, "ASM_IBUF_CUR1 = %x\n",Asm_Get(ASM_IBUF_CUR1));
    len += sprintf(buf+ len, "ASM_SET1= %x\n",Asm_Get(ASM_SET1));
    len += sprintf(buf+ len, "ASM_GAIN1 = %x\n",Asm_Get(ASM_GAIN1));
    len += sprintf(buf+ len, "ASM_IR_CNT1 = %x\n",Asm_Get(ASM_IR_CNT1));

    len += sprintf(buf+ len, "ASM_IBUF_BASE2 = %x\n",Asm_Get(ASM_IBUF_BASE2));
    len += sprintf(buf+ len, "ASM_IBUF_END2 = %x\n",Asm_Get(ASM_IBUF_END2));
    len += sprintf(buf+ len, "ASM_IBUF_CUR2 = %x\n",Asm_Get(ASM_IBUF_CUR2));
    len += sprintf(buf+ len, "ASM_SET2 = %x\n",Asm_Get(ASM_SET2));
    len += sprintf(buf+ len, "ASM_GAIN2 = %x\n",Asm_Get(ASM_GAIN2));
    len += sprintf(buf+ len, "ASM_IR_CNT2 = %x\n",Asm_Get(ASM_IR_CNT2));

    len += sprintf(buf+ len, "ASM_CON0 = %x\n",Asm_Get(ASM_CON0));
    len += sprintf(buf+ len, "ASM_CON1 = %x\n",Asm_Get(ASM_CON1));
    len += sprintf(buf+ len, "ASM_SIN_CON = %x\n",Asm_Get(ASM_SIN_CON));
    len += sprintf(buf+ len, "ASM_AFE_CON0 = %x\n",Asm_Get(ASM_AFE_CON0));
    len += sprintf(buf+ len, "ASM_AFE_CON1 = %x\n",Asm_Get(ASM_AFE_CON1));
    len += sprintf(buf+ len, "ASM_AFE_CON2 = %x\n",Asm_Get(ASM_AFE_CON2));
    len += sprintf(buf+ len, "AFE_EDI_CON = %x\n",Asm_Get(AFE_EDI_CON));
    len += sprintf(buf+ len, "ASM_EDI_BASE = %x\n",Asm_Get(ASM_EDI_BASE));
    len += sprintf(buf+ len, "ASM_EDI_END = %x\n",Asm_Get(ASM_EDI_END));
    len += sprintf(buf+ len, "ASM_EDI_CON1 = %x\n",Asm_Get(ASM_EDI_CON1));
    len += sprintf(buf+ len, "ASM_EDI_IR_CNT = %x\n",Asm_Get(ASM_EDI_IR_CNT));
    len += sprintf(buf+ len, "ASM_EBIT_RDATA = %x\n",Asm_Get(ASM_EBIT_RDATA));
    len += sprintf(buf+ len, "ASM_EDI_OUTPUT_MONITOR = %x\n",Asm_Get(ASM_EDI_OUTPUT_MONITOR));
    len += sprintf(buf+ len, "ASM_EDI_INPUT_CURSU = %x\n",Asm_Get(ASM_EDI_INPUT_CURSU));

    len += sprintf(buf+ len, "AFE_AAG_CON = %x\n",Ana_Get(AFE_AAG_CON));
    len += sprintf(buf+ len, "AFE_AAC_CON = %x\n",Ana_Get(AFE_AAC_CON));
    len += sprintf(buf+ len, "AFE_AAPDB_CON = %x\n",Ana_Get(AFE_AAPDB_CON));
    len += sprintf(buf+ len, "AFE_AAC_NEW_CON = %x\n",Ana_Get(AFE_AAC_NEW_CON));

    // also print ASM_MEMORY_CONTROL
    volatile UINT32 *ASM_REG =(volatile UINT32 *) (ASM_MEMORY_CONTROL);
    len += sprintf(buf+ len, "ASM_MEMORY_CONTROL = %x\n",*ASM_REG);

    len += sprintf(buf+ len, "Asm_Amp_Flag = %d\n",Asm_Amp_Flag);
    len += sprintf(buf+ len, "Asm_Memory_Count = %d\n",Asm_Memory_Count);
    len += sprintf(buf+ len, "Clock_Count = %d\n",Clock_Count);


    return len;
}

int LouderSPKSound(kal_uint32 time)
{
    int u4vlaue =1;
    printk("LouderSPKSound time = %d ",time);
    //start SinWave
    {
        Yusu_sound_Store_reg();
        spin_lock_bh(&sound_lock);
        Asm_Enable_Memory_Power ();	//disable memory buffer
        Yusu_Sound_Power_On();
        // setting asm register
        Asm_Set(ASM_AFE_CON2,0x0059,ASM_MASK_ALL);
        Asm_Set(ASM_AFE_CON1,0x0140,0x0fff);
        Asm_Set(ASM_AFE_CON0,1,ASM_MASK_ALL);
        Asm_Set(ASM_CON1, 2,ASM_MASK_ALL);
        // enable ASM interrupt
        Asm_Set(ASM_IR_CLR,(1<<0),(1<<0));
        Asm_Set(ASM_CON0,u4vlaue,0x1);
        // open analog dac
        Ana_Set(AFE_AAPDB_CON,0x001c,0x001c);
        Ana_Set(AFE_AAPDB_CON,0x0013,0x0013);
        // setting analog gain
        Ana_Set(AFE_AAG_CON,  (9 | (9<< 4)),  0xFFFFFFFF);
        //asm ummute
        Asm_Set(ASM_AFE_CON1,0x000,0xC0C);
        //enable asm sinwave
        Asm_Set(ASM_SIN_CON, 0x2401, ASM_MASK_ALL);
        spin_unlock_bh(&sound_lock);
        // turn on speaker
        Sound_Speaker_Turnon(Channel_Stereo);
        msleep(time); // sleep
    }
    // stop SineWave
    {
    Asm_Disable_Memory_Power ();	//disable memory buffer
        Yusu_sound_Recover_reg();
        Yusu_Sound_Power_Off();
        if(Asm_Amp_Flag== false){
            Sound_Speaker_Turnoff(Channel_Stereo);
        }
    }
    return true;
}

void Yusu_Init_Stream(kal_uint32  Asm_Buffer_Length)
{
    int index =0 ;
    kal_uint32 sram_addr = (kal_uint32)ASM_SRAM_START;
    if(Asm_Buffer_Length  >  ASM_SRAM_LENGTH){
        printk("Yusu_Init_Stream with Length  =%d\n",Asm_Buffer_Length);
        Asm_Buffer_Length = ASM_SRAM_LENGTH;
    }
    PRINTK("Yusu_Init_Stream Asm_Buffer_Length = %x\n",Asm_Buffer_Length);
    Asm_Control->u4BufferSize = Asm_Buffer_Length;
    // init stream
    for(index =0;index < ASM_SRC_BLOCK_MAX ; index ++)
    {
        Asm_Control->rSRCBlock[index].u4BufferSize = Asm_Buffer_Length;
        Asm_Control->rSRCBlock[index].pucVirtBufAddr = (kal_uint8*)ioremap( (unsigned long)sram_addr , (unsigned long)ASM_SRAM_LENGTH );  // use ioremap to map sram
        Asm_Control->rSRCBlock[index].pucPhysBufAddr = sram_addr;
        Asm_Control->rSRCBlock[index].u4InterruptLine = index;
        Asm_Control->rSRCBlock[index].u4SampleNumMask = 0x000f;  // 16 byte align
        Asm_Control->rSRCBlock[index].u4WriteIdx	       = 0;
        Asm_Control->rSRCBlock[index].u4DMAReadIdx     = 0;
        Asm_Control->rSRCBlock[index].u4DataRemained   =  0;
        memset((void*)Asm_Control->rSRCBlock[index].pucVirtBufAddr,0,Asm_Buffer_Length);
        Asm_Set(ASM_IBUF_BASE0 + (ASM_BLOCK_LENGTH * index)  ,Asm_Control->rSRCBlock[index].pucPhysBufAddr,ASM_MASK_ALL);
        Asm_Set(ASM_IBUF_END0 +   (ASM_BLOCK_LENGTH * index) ,Asm_Control->rSRCBlock[index].pucPhysBufAddr +Asm_Buffer_Length -1 ,ASM_MASK_ALL);
        PRINTK("Yusu_Init_Stream  pucVirtBufAddr  =%p  pucPhysBufAddr = %x\n",
            Asm_Control->rSRCBlock[index].pucVirtBufAddr, Asm_Control->rSRCBlock[index].pucPhysBufAddr);
     }
}

void Yusu_Clear_Stream_Buf(void)
{
    int index =0 ;
    // clear stream
    for(index =0;index < ASM_SRC_BLOCK_MAX ; index ++)
    {
        memset((void*)Asm_Control->rSRCBlock[index].pucVirtBufAddr,0,Asm_Control->rSRCBlock[index].u4BufferSize);
     }
}


void Yusu_Init_InputStream(kal_uint32  Edi_Buffer_Length)
{
    int index =0 ;
    if(Edi_Buffer_Length  >  EDI_LENGTH_MAX){
        printk("Yusu_Init_InputStream with Length  =%d\n",Edi_Buffer_Length);
        Edi_Buffer_Length = Edi_Buffer_Length;
    }
    PRINTK("Yusu_Init_Stream Asm_Buffer_Length = %x\n",Edi_Buffer_Length);
    Edi_Control->u4BufferSize = Edi_Buffer_Length;
    // init stream
    for(index =0;index < EDI_BLOCK_MAX ; index ++)
    {
        Edi_Control->rSRCBlock[index].u4BufferSize = Edi_Buffer_Length;
        Edi_Control->rSRCBlock[index].pucVirtBufAddr = (kal_uint8*)kmalloc(Edi_Buffer_Length , GFP_KERNEL);  // use ioremap to map sram
        Edi_Control->rSRCBlock[index].pucPhysBufAddr = virt_to_phys((void*)(Edi_Control->rSRCBlock[index].pucVirtBufAddr));
        Edi_Control->rSRCBlock[index].u4InterruptLine = index;
        Edi_Control->rSRCBlock[index].u4SampleNumMask = 0x000f;  // 16 byte align
        Edi_Control->rSRCBlock[index].u4WriteIdx	       = 0;
        Edi_Control->rSRCBlock[index].u4DMAReadIdx     = 0;
        Edi_Control->rSRCBlock[index].u4DataRemained   =  0;
        memset((void*)Edi_Control->rSRCBlock[index].pucVirtBufAddr,0,Edi_Buffer_Length);
        Asm_Set(ASM_EDI_BASE ,Edi_Control->rSRCBlock[index].pucPhysBufAddr,ASM_MASK_ALL);
        Asm_Set(ASM_EDI_END  ,Edi_Control->rSRCBlock[index].pucPhysBufAddr +Edi_Buffer_Length -1 ,ASM_MASK_ALL);
        PRINTK("Yusu_Init_InputStream  pucVirtBufAddr  =%p  pucPhysBufAddr = %x\n",
            Edi_Control->rSRCBlock[index].pucVirtBufAddr, Edi_Control->rSRCBlock[index].pucPhysBufAddr);
     }
}

void Yusu_DeInit_InputStream(void)
{
    int index =0 ;
    // clear EDI register
    Asm_Set(AFE_EDI_CON ,0,ASM_MASK_ALL);
    Asm_Set(ASM_EDI_CON1 ,0,ASM_MASK_ALL);
    Asm_Set(ASM_EDI_IR_CNT ,0,ASM_MASK_ALL);

    for(index =0;index < EDI_BLOCK_MAX ; index ++)
    {
        if(Edi_Control->rSRCBlock[index].pucVirtBufAddr != NULL){
            kfree((void*)Edi_Control->rSRCBlock[index].pucVirtBufAddr);
            Edi_Control->rSRCBlock[index].pucVirtBufAddr = NULL;
        }
        Edi_Control->rSRCBlock[index].u4BufferSize = 0;
        Edi_Control->rSRCBlock[index].pucPhysBufAddr = 0;
        Edi_Control->rSRCBlock[index].u4InterruptLine = 0;
        Edi_Control->rSRCBlock[index].u4SampleNumMask = 0;  // 16 byte align
        Edi_Control->rSRCBlock[index].u4WriteIdx	       = 0;
        Edi_Control->rSRCBlock[index].u4DMAReadIdx     = 0;
        Edi_Control->rSRCBlock[index].u4DataRemained   =  0;
        PRINTK("Yusu_DeInit_InputStream");
     }
}

void Yusu_Depop_On(void )
{
    Ana_Set(AFE_AAC_NEW_CON, 0x0002, 0x0002);   //set_AAC_NEW_CON to old VCM mode
    Ana_Set(AFE_AAC_CON,0x1000,0x1000);   // ABI  + 0x0204 depop on
}

void Yusu_Depop_Off(void )
{
    Ana_Set(AFE_AAC_NEW_CON, 0x0000, 0x0002);   //set_AAC_NEW_CON to old VCM mode
    Ana_Set(AFE_AAC_CON,000000,0x1000);   // ABI  + 0x0204 depop on
}


int Yusu_Sound_Allocate_Stream(struct file *fp)  // find a free asm block
{
    int index = 0;
    for(index =0; index < ASM_SRC_BLOCK_MAX ; index++){
        if(Asm_Control->rSRCBlock[index].flip == NULL)
        	    return index;
    }
    return -1;
}

int Yusu_Sound_Allocate_InputStream(struct file *fp)  // find a free asm block
{
    int index = 0;
    for(index =0; index < EDI_BLOCK_MAX ; index++){
        if(Edi_Control->rSRCBlock[index].flip == NULL)
        	    return index;
    }
    return -1;
}

bool	Yusu_Sound_Deallocate_InputStream(struct file *fp)  // find a free asm block
{
    int index = 0;
    for(index =0; index < ASM_SRC_BLOCK_MAX ; index++){
        if(Asm_Control->rSRCBlock[index].flip == fp){
        	    Asm_Control->rSRCBlock[index].flip = NULL;
        	    return TRUE;
        	}
    }
    return FALSE;
}

bool	Yusu_Sound_Deallocate_Stream(struct file *fp)  // find a free asm block
{
    int index = 0;
    for(index =0; index < ASM_SRC_BLOCK_MAX ; index++){
        if(Asm_Control->rSRCBlock[index].flip == fp){
        	    Asm_Control->rSRCBlock[index].flip = NULL;
        	        return TRUE;
        	}
    }
    return FALSE;
}

kal_uint32 Yusu_Sound_Find_Stream(struct file *fp)  // find match block
{
    int index;
    for(index =0; index < ASM_SRC_BLOCK_MAX ; index++){
        if(Asm_Control->rSRCBlock[index].flip == fp)
        	    return index;
    }
    return FALSE;
}

kal_uint32 Yusu_Sound_Find_InputStream(struct file *fp)  // find match block
{
    int index;
    for(index =0; index < ASM_SRC_BLOCK_MAX ; index++){
        if(Edi_Control->rSRCBlock[index].flip == fp)
        	    return index;
    }
    return FALSE;
}

void Yusu_Sound_Standby(struct file *fp,unsigned long arg)
{
    int index = Yusu_Sound_Find_Stream(fp);
    PRINTK("Yusu_Sound_Standby\n");
    if(index < 0){
        PRINTK("can't find standby stream\n");
        return;
    }
    memset(Asm_Control->rSRCBlock[index].pucVirtBufAddr,0,Asm_Control->rSRCBlock[index].u4BufferSize);
    spin_lock(&sound_lock);
    Asm_Control->rSRCBlock[index].u4WriteIdx         = 0;
    Asm_Control->rSRCBlock[index].u4DMAReadIdx   = 0;
    Asm_Control->rSRCBlock[index].u4DataRemained = 0;
    Asm_Control->rSRCBlock[index].u4fsyncflag = false;
    spin_unlock(&sound_lock);
}

void Yusu_Sound_EDI_Standby(struct file *fp,unsigned long arg)
{
    int index = Yusu_Sound_Find_InputStream(fp);
    PRINTK("Yusu_Sound_Standby\n");
    if(index < 0){
        PRINTK("can't find standby stream\n");
        return;
    }
    memset(Edi_Control->rSRCBlock[index].pucVirtBufAddr,0,Edi_Control->rSRCBlock[index].u4BufferSize);
    Edi_Control->rSRCBlock[index].u4WriteIdx	  = 0;
    Edi_Control->rSRCBlock[index].u4DMAReadIdx	  = 0;
    Edi_Control->rSRCBlock[index].u4DataRemained = 0;
}

static int Yusu_Sound_fasync(int fd, struct file *flip, int mode)
{
    PRINTK("Yusu_Sound_fasync  \n");
    return fasync_helper(fd,flip,mode,&Yusu_Sound_async);
}


static int Yusu_Sound_Reset()
{
    int index =0;
    for(index =0 ; index <ASM_SRC_BLOCK_MAX ; index++ ){
        memset(Asm_Control->rSRCBlock[index].pucVirtBufAddr,0,Asm_Control->rSRCBlock[index].u4BufferSize);
        Asm_Control->rSRCBlock[index].flip                              = NULL;
        Asm_Control->rSRCBlock[index].u4WriteIdx		  = 0;
        Asm_Control->rSRCBlock[index].u4DMAReadIdx	  = 0;
        Asm_Control->rSRCBlock[index].u4DataRemained      = 0;
        Asm_Control->rSRCBlock[index].u4fsyncflag = false;
    }
    Yusu_DeInit_InputStream();
    return true;
}

static int Yusu_Sound_Flush_Reset()
{
    // flush will set all register to default
    Asm_Amp_Flag = false;
    Asm_Memory_Count = 0;
    Clock_Count =0;
    Yusu_Sound_Reset();
    Asm_Disable_Memory_Power ();	//disable memory buffer
    Yusu_Sound_Suspend_Power_Off();  // turn off asm afe clock
    Yusu_Sound_Power_On();  // turn off asm afe clock
    printk("Yusu_Sound_Flush_Reset done \n");
}


static int Yusu_Sound_flush(struct file *flip)
{
    Audio_Flush_Count ++;
    printk("Yusu_Sound_flush Audio_Flush_Count = %d \n",Audio_Flush_Count);
    return 0;
}


void  Sound_do_Edi_Task(void)
{
    kal_int32 Edi_get_bytes=0;
    kal_int32 Current_Idx=0;
    ASM_SRC_BLOCK_T *Src_Block =&(Edi_Control->rSRCBlock[0]);

    if(Src_Block == NULL)
    	return;

    Current_Idx = Asm_Get(ASM_EDI_OUTPUT_MONITOR);
    Edi_get_bytes = (Current_Idx  - Src_Block->pucPhysBufAddr )- Src_Block->u4DMAReadIdx;
    if(Edi_get_bytes <0){
    	Edi_get_bytes += Src_Block->u4BufferSize;
    }

    Edi_get_bytes &= (~Src_Block->u4SampleNumMask);
    /*
    PRINTK("Edi_require_bytes is %d Current_Idx = %d Src_Block->pucPhysBufAddr=%u Src_Block->u4DMAReadIdx = %d\n ",
        Edi_get_bytes,Current_Idx,Src_Block->pucPhysBufAddr,Src_Block->u4DMAReadIdx);
        */

    if(Edi_get_bytes ==0)
        return;

    spin_lock_bh(&sound_lock);
    Src_Block->u4DataRemained += Edi_get_bytes;
    Src_Block->u4WriteIdx+= Edi_get_bytes;
    Src_Block->u4WriteIdx  %= Src_Block->u4BufferSize;
   // buffer overflow
    if(Src_Block->u4DataRemained > Src_Block->u4BufferSize){
        Src_Block->u4DataRemained= 0;
        Src_Block->u4DMAReadIdx = Src_Block->u4WriteIdx;
        if( Src_Block->u4DMAReadIdx <0){
             Src_Block->u4DMAReadIdx+=Src_Block->u4BufferSize;
        }
        spin_unlock_bh(&sound_lock);
        memset((void*)Src_Block->pucVirtBufAddr,0,Src_Block->u4BufferSize);
        printk("Sound_do_Edi_Task buffer overflow u4DMAReadIdx  = %d ,u4DataRemained = %d u4WriteIdx = %d\n",
        Src_Block->u4DMAReadIdx,Src_Block->u4DataRemained,Src_Block->u4WriteIdx);
        spin_lock_bh(&sound_lock);
    }
    spin_unlock_bh(&sound_lock);
    /*
    PRINTK("Sound_do_Edi_Task normal u4DMAReadIdx  = %d ,u4DataRemained = %d u4WriteIdx = %d\n",
        Src_Block->u4DMAReadIdx,Src_Block->u4DataRemained,Src_Block->u4WriteIdx);
        */

    Edi_wait_queue_flag =1;
    wake_up_interruptible(&Edi_Wait_Queue);
}

void Sound_do_Tasklet(void)  // interrupt tasklet
{
    kal_int32 index = -1;
    kal_int32 Asm_require_bytes=0;
    kal_int32 Current_Idx=0;
    ASM_SRC_BLOCK_T *Src_Block = NULL;
    //PRINTK("u4_Asm_Stat = %x\n" , u4_Asm_Stat);
    if(u4_Asm_Stat == 0 ){
        PRINTK("u4_Asm_Stat  = 0\n");
        return;
    }

do_more:
    // block 0 interrupt
    if (u4_Asm_Stat & ASM_BLOCK_0){
        u4_Asm_Stat &=(~ASM_BLOCK_0);
        index =0;
    }
    else if(u4_Asm_Stat & ASM_BLOCK_1){
        u4_Asm_Stat &=(~ASM_BLOCK_1);
        index =1;
    }
    else if(u4_Asm_Stat & ASM_BLOCK_2){
        u4_Asm_Stat &=(~ASM_BLOCK_2);
        index =2;
    }
    else if(u4_Asm_Stat & EDI_BLOCK){
        u4_Asm_Stat &=(~EDI_BLOCK);
        Sound_do_Edi_Task();
        return;
    }

    Src_Block = &(Asm_Control->rSRCBlock[index]);
    Current_Idx = Asm_Get(ASM_IBUF_CUR0 + (0x20 * index));
    Current_Idx = Current_Idx - Src_Block->pucPhysBufAddr;
    Asm_require_bytes = Current_Idx - Src_Block->u4DMAReadIdx;
    if(Asm_require_bytes <0)
    	Asm_require_bytes += Src_Block->u4BufferSize;

    Asm_require_bytes &= (~Src_Block->u4SampleNumMask);
    PRINTK("Asm_require_bytes is %d\n",Asm_require_bytes);

    // clean used buffer
    if((Src_Block->u4DMAReadIdx + Asm_require_bytes) <  (Src_Block->u4BufferSize))
    	memset(Src_Block->pucVirtBufAddr + Src_Block->u4DMAReadIdx , 0 , Asm_require_bytes);
    else{
         kal_uint32 size_1 = Src_Block->u4BufferSize - Src_Block->u4DMAReadIdx;
         kal_uint32 size_2 = Asm_require_bytes - size_1;
    	memset(Src_Block->pucVirtBufAddr + Src_Block->u4DMAReadIdx , 0 , size_1);
   	memset(Src_Block->pucVirtBufAddr  , 0 , size_2);
    }

    if( Src_Block->u4DataRemained <= Asm_require_bytes ){
        memset(Src_Block->pucVirtBufAddr,0,Src_Block->u4BufferSize);
        PRINTK("data underflow u4DMAReadIdx  = %d ,u4DataRemained = %d u4WriteIdx = %d Asm_require_bytes = %d\n",
            Src_Block->u4DMAReadIdx,Src_Block->u4DataRemained,Src_Block->u4WriteIdx,Asm_require_bytes);
        spin_lock_bh(&sound_lock);
        Src_Block->u4DMAReadIdx += Asm_require_bytes;
        Src_Block->u4DMAReadIdx %= Src_Block->u4BufferSize;
        Src_Block->u4WriteIdx  =  Src_Block->u4DMAReadIdx  ;
        Src_Block->u4DataRemained =Asm_Control->rSRCBlock[index].u4BufferSize;
        spin_unlock_bh(&sound_lock);
        if(Src_Block->u4fsyncflag == false){

            kill_fasync(&Yusu_Sound_async,SIGIO, POLL_IN);
            spin_lock_bh(&sound_lock);
            Src_Block->u4fsyncflag = true;
            spin_unlock_bh(&sound_lock);
        }
        PRINTK("data underflow u4DMAReadIdx  = %d ,u4DataRemained = %d u4WriteIdx = %d\n",
           Src_Block->u4DMAReadIdx,Src_Block->u4DataRemained,Src_Block->u4WriteIdx);
    }
    else{
        spin_lock_bh(&sound_lock);
        Src_Block->u4DataRemained -= Asm_require_bytes;
        Src_Block->u4DMAReadIdx += Asm_require_bytes;
        Src_Block->u4DMAReadIdx  %= Src_Block->u4BufferSize;
        spin_unlock_bh(&sound_lock);
        PRINTK("normal u4DMAReadIdx  = %d ,u4DataRemained = %d u4WriteIdx = %d\n",
            Src_Block->u4DMAReadIdx,Src_Block->u4DataRemained,Src_Block->u4WriteIdx);
    }
    if(u4_Asm_Stat != 0)
    	goto do_more;

    wait_queue_flag =1;
    wake_up_interruptible(&Sound_Wait_Queue);
    return ;
}

static irqreturn_t asm_irq_handler(int irq, void *dev_id)
{
    u4_Asm_Stat |=  Asm_Get(ASM_IR_STATUS);
    Asm_Set(ASM_IR_CLR,u4_Asm_Stat,0xffff);
    tasklet_schedule(&magic_tasklet_handle);
    return IRQ_HANDLED;
}

static void magic_tasklet(unsigned long data)
{
    	Sound_do_Tasklet();
}

void Yusu_Sound_Power_On(void)
{
    spin_lock(&sound_lock);
    if(Clock_Count == 0 ){
    // get ASM , AFE , clock
        //PRINTK("Enable Audio clock Clock_Count = %d\n",Clock_Count);
        if(hwEnableClock(MT6516_PDN_MM_AFE,"SOUND")== false)
            PRINTK("hwEnableClock MT6516_PDN_MM_AFE fail");
        if(hwEnableClock(MT6516_PDN_MM_ASM,"SOUND")==false)
            PRINTK("hwEnableClock MT6516_PDN_MM_ASM fail");
    }
    Clock_Count ++;
    spin_unlock(&sound_lock);
    //PRINTK("Yusu_Sound_Power_On Clock_Count = %d\n",Clock_Count);
}

void Yusu_Sound_Power_Off(void)
{
    //printk("Yusu_Sound_Power_Off Clock_Count = %d\n",Clock_Count);
    spin_lock(&sound_lock);
    Clock_Count --;
    spin_unlock(&sound_lock);
    if(Clock_Count == 0 ){
    // release  ASM , AFE , clock
        //PRINTK("disable Audio clock Clock_Count = %d\n",Clock_Count);
        if(hwDisableClock(MT6516_PDN_MM_AFE,"SOUND")==false)
            PRINTK("hwDisableClock MT6516_PDN_MM_AFE fail");
        if(hwDisableClock(MT6516_PDN_MM_ASM,"SOUND")==false)
            PRINTK("hwDisableClock MT6516_PDN_MM_ASM fail");
    }
    if(Clock_Count <0){
        Clock_Count =0;
    }
}

void Yusu_Sound_Suspend_Power_Off(void)
{
    printk("Yusu_Sound_Power_Off Clock_Count = %d\n",Clock_Count);
    if(hwDisableClock(MT6516_PDN_MM_AFE,"SOUND")==false)
        PRINTK("hwDisableClock MT6516_PDN_MM_AFE fail");
    if(hwDisableClock(MT6516_PDN_MM_ASM,"SOUND")==false)
        PRINTK("hwDisableClock MT6516_PDN_MM_ASM fail");
}

void Yusu_Sound_Suspend_Power_On(void)
{
    printk("Yusu_Sound_Power_Off Clock_Count = %d\n",Clock_Count);
    if(hwEnableClock(MT6516_PDN_MM_AFE,"SOUND")==false)
        PRINTK("hwDisableClock MT6516_PDN_MM_AFE fail");
    if(hwEnableClock(MT6516_PDN_MM_ASM,"SOUND")==false)
        PRINTK("hwDisableClock MT6516_PDN_MM_ASM fail");
}


static int Yusu_Sound_open(struct inode *inode, struct file *fp)
{
	PRINTK("Yusu_Sound_open inode = %p , file = %p \n ", inode , fp);
	return 0;
}

static int Yusu_Sound_release(struct inode *inode, struct file *fp)
{
	PRINTK("Yusu_Sound_release inode = %p , file = %p", inode , fp);
         Yusu_Sound_fasync(-1,fp,0);
	if (!(fp->f_mode & FMODE_WRITE || fp->f_mode & FMODE_READ))
		return -ENODEV;
	return 0;
}

static ssize_t Yusu_Sound_write(struct file *fp, const char __user *data, size_t count, loff_t *offset)
{
    kal_uint32 Stream_index =0;
    int ret =0;
    ASM_SRC_BLOCK_T  *Asm_Block = NULL;
    ssize_t written_size = count;
    char *Wirte_Data_Ptr = (char*)data;
    PRINTK("Yusu_Sound_write count = %d \n", count);  // how much data can copy

    Stream_index = Yusu_Sound_Find_Stream(fp);
    if(Stream_index >= ASM_SRC_BLOCK_MAX){
        PRINTK("Yusu_Sound_write can't find stream\n");
        return 0;
    }
    Asm_Block = &(Asm_Control->rSRCBlock[Stream_index]);

    while(count)
    {
       kal_int32 copy_size = Asm_Block->u4BufferSize - Asm_Block->u4DataRemained;  // how many buffer can copy
       if(count <= (kal_uint32)copy_size){
           copy_size = count;
           PRINTK("copy_size = %d \n", copy_size);  // how much data can copy
       }

       if(Asm_Block->u4WriteIdx + copy_size  <   Asm_Block->u4BufferSize ){
           if(copy_from_user( (Asm_Block->pucVirtBufAddr + Asm_Block->u4WriteIdx) , Wirte_Data_Ptr,copy_size) ){
               PRINTK("can't copy from user");
               return -1;
           }
           spin_lock_bh(&sound_lock);
           Asm_Block->u4DataRemained += copy_size;
           Asm_Block->u4WriteIdx += copy_size;
           Asm_Block->u4WriteIdx %= Asm_Block->u4BufferSize;
           spin_unlock_bh(&sound_lock);
           Wirte_Data_Ptr += copy_size;
           count -= copy_size;
           PRINTK("finish copy data user to input data once , copy size is %d, u4WriteIdx  is %d u4DataRemained is %d \r\n",copy_size, Asm_Block->u4WriteIdx,  Asm_Block->u4DataRemained );
       }
       else{
           kal_uint32 size_1 = Asm_Block->u4BufferSize - Asm_Block->u4WriteIdx;
	   kal_uint32 size_2 = copy_size - size_1;
	   if ((copy_from_user( (Asm_Block->pucVirtBufAddr + Asm_Block->u4WriteIdx), Wirte_Data_Ptr , size_1)) ){
	       PRINTK("can't copy from user");
	       return -1;
	   }
	   spin_lock_bh(&sound_lock);
	   Asm_Block->u4DataRemained += size_1;
	   Asm_Block->u4WriteIdx += size_1;
	   Asm_Block->u4WriteIdx %= Asm_Block->u4BufferSize;
	   spin_unlock_bh(&sound_lock);
            if ((copy_from_user( (Asm_Block->pucVirtBufAddr + Asm_Block->u4WriteIdx),( Wirte_Data_Ptr + size_1 ), size_2))){
	        PRINTK("can't copy from user");
	        return -1;
	    }
            spin_lock_bh(&sound_lock);
            Asm_Block->u4DataRemained += size_2;
            Asm_Block->u4WriteIdx += size_2;
            spin_unlock_bh(&sound_lock);
            count -= copy_size;
	   Wirte_Data_Ptr += copy_size;
	   PRINTK("finish copy data user to input data twice , copy size is %d, u4WriteIdx  is %d u4DataRemained is %d \r\n",copy_size, Asm_Block->u4WriteIdx,  Asm_Block->u4DataRemained );
       }
       if(count != 0){
           PRINTK("wait for interrupt signal\n");
           wait_queue_flag =0;
           ret = wait_event_interruptible_timeout(Sound_Wait_Queue, wait_queue_flag,ASM_WAIT_TIMEOUT);
           if(ret <= 0 ){
               printk("Yusu_Sound_write wait_event_interruptible_timeout error!\n");
               return written_size;
           }
       }
    }
    return written_size;
}

static ssize_t Yusu_Sound_read(struct file *fp,  char __user *data, size_t count, loff_t *offset)
{
     // this is use for I2S read data
     int Stream_index =-1;
     ASM_SRC_BLOCK_T  *Asm_Block = NULL;
     ssize_t read_size = 0;
     ssize_t read_count = 0;
     PRINTK("Yusu_Sound_read count = %d fp = %p\n", count,fp);  // how much data can copy
     char *Read_Data_Ptr = (char*)data;
     Stream_index = Yusu_Sound_Find_InputStream(fp);
     if(Stream_index >= EDI_BLOCK_MAX){
         printk("Yusu_Sound_read can't find stream\n");
         return 0;
     }

     Asm_Block = &(Edi_Control->rSRCBlock[Stream_index]);
     while(count ){
         if(Yusu_Sound_Find_InputStream(fp) < 0){
             printk("Yusu_Sound_read fp is not exist!!\n");
             break;
         }

         spin_lock_bh(&sound_lock);
         if(count >  Asm_Block->u4DataRemained){
             read_size = Asm_Block->u4DataRemained;
         }
         else{
             read_size = count;
         }
         spin_unlock_bh(&sound_lock);
         read_count+= read_size;

	if(Asm_Block->u4DMAReadIdx + read_size  <   Asm_Block->u4BufferSize ){
	  #ifndef SOUND_FAKE_READ
             if(copy_to_user( (void __user *)Read_Data_Ptr, (Asm_Block->pucVirtBufAddr + Asm_Block->u4DMAReadIdx) ,read_size) ){
                 PRINTK("can't copy to user");
                 return -1;
             }
           #else
               copy_to_user_fake(Read_Data_Ptr ,read_size);
           #endif
             spin_lock_bh(&sound_lock);
             Asm_Block->u4DataRemained -= read_size;
             Asm_Block->u4DMAReadIdx += read_size;
             Asm_Block->u4DMAReadIdx %= Asm_Block->u4BufferSize;
             spin_unlock_bh(&sound_lock);
             Read_Data_Ptr += read_size;
             count -= read_size;
             PRINTK("finish read_size data user to input data once , copy size is %d, u4DMAReadIdx  is %d u4DataRemained is %d \r\n",read_size, Asm_Block->u4DMAReadIdx,  Asm_Block->u4DataRemained );
         }

         else{
             kal_uint32 size_1 = Asm_Block->u4BufferSize - Asm_Block->u4DMAReadIdx;
             kal_uint32 size_2 = read_size - size_1;
       	  #ifndef SOUND_FAKE_READ
             if (copy_to_user( (void __user *)Read_Data_Ptr ,(Asm_Block->pucVirtBufAddr + Asm_Block->u4DMAReadIdx) , size_1)){
	       PRINTK("can't copy to user");
	       return -1;
	   }
	   #else
	       copy_to_user_fake(Read_Data_Ptr,size_1);
	   #endif
	   spin_lock_bh(&sound_lock);
	   Asm_Block->u4DataRemained -= size_1;
	   Asm_Block->u4DMAReadIdx += size_1;
	   Asm_Block->u4DMAReadIdx %= Asm_Block->u4BufferSize;
	   spin_unlock_bh(&sound_lock);
       	  #ifndef SOUND_FAKE_READ
            if (copy_to_user(  (void __user *)(Read_Data_Ptr+size_1),(Asm_Block->pucVirtBufAddr + Asm_Block->u4DMAReadIdx), size_2)){
	        PRINTK("can't copy to user");
	        return -1;
	    }
	  #else
	       copy_to_user_fake((Read_Data_Ptr+size_1),size_2);
	  #endif
            spin_lock_bh(&sound_lock);
            Asm_Block->u4DataRemained -= size_2;
            Asm_Block->u4DMAReadIdx += size_2;
            spin_unlock_bh(&sound_lock);
            count -= read_size;
	   Read_Data_Ptr += read_size;
	   PRINTK("finish copy data user to input data twice , copy size is %d, u4WriteIdx  is %d u4DataRemained is %d \r\n",read_size, Asm_Block->u4WriteIdx,  Asm_Block->u4DataRemained );
	}
         if(count != 0){
           //PRINTK("read wait for interrupt signal\n");
           Edi_wait_queue_flag =0;
           wait_event_interruptible_timeout(Edi_Wait_Queue, Edi_wait_queue_flag,EDI_WAIT_TIMEOUT);
         }
     }
     return read_count;
}

static int Yusu_Sound_ioctl(struct inode *inode, struct file *fp, unsigned int cmd, unsigned long arg)
{
         int ret =0;
   	_Reg_Data Reg_Data;
	Yusu_Sound_Power_On();

	switch(cmd)
	{
	    case YUSU_SET_ASM_REG:
	    {
	    	PRINTK("YUSU_SET_ASM_REG\n");
	    	if(copy_from_user((void *)(&Reg_Data), (const void __user *)( arg), sizeof(Reg_Data))){
	    	    Yusu_Sound_Power_Off();
	    	    return -EFAULT;
	    	}
	    	spin_lock_bh(&sound_lock);
	    	Asm_Set(Reg_Data.offset,Reg_Data.value,Reg_Data.mask);
	    	spin_unlock_bh(&sound_lock);
	    	break;
	    }
	    case YUSU_GET_ASM_REG:
	    {
		PRINTK("YUSU_GET_ASM_REG\n");
	    	if(copy_from_user((void *)(&Reg_Data), (const void __user *)( arg), sizeof(Reg_Data))){
	    	    Yusu_Sound_Power_Off();
	    	    return -EFAULT;
	    	}
	    	spin_lock_bh(&sound_lock);
	    	Reg_Data.value = Asm_Get(Reg_Data.offset);
	    	spin_unlock_bh(&sound_lock);
	    	if(copy_to_user((void __user *)( arg),(void *)(&Reg_Data), sizeof(Reg_Data))){
	    	    Yusu_Sound_Power_Off();
	    	    return -EFAULT;
	    	}
	    	break;
	    }
	    case YUSU_SET_ANA_REG:
	    {
	    	PRINTK("YUSU_SET_ANA_REG\n");
	    	if(copy_from_user((void *)(&Reg_Data), (const void __user *)( arg), sizeof(Reg_Data))){
	    	    Yusu_Sound_Power_Off();
	    	    return -EFAULT;
	    	}
	    	spin_lock_bh(&sound_lock);
	    	Ana_Set(Reg_Data.offset,Reg_Data.value,Reg_Data.mask);
	    	spin_unlock_bh(&sound_lock);
	    	break;
	    }
	    case YUSU_GET_ANA_REG:
	    {
	    	PRINTK("YUSU_GET_ANA_REG\n");
	    	if(copy_from_user((void *)(&Reg_Data), (const void __user *)( arg), sizeof(Reg_Data))){
	    	    Yusu_Sound_Power_Off();
	    	    return -EFAULT;
	    	}
	    	spin_lock_bh(&sound_lock);
	    	Reg_Data.value = Ana_Get(Reg_Data.offset);
	    	spin_unlock_bh(&sound_lock);
	    	if(copy_to_user((void __user *)( arg),(void *)(&Reg_Data), sizeof(Reg_Data))){
	    	    Yusu_Sound_Power_Off();
	    	    return -EFAULT;
	    	}
	    	break;
	    }
	    case YUSU_SET_MMAP_INDEX:
	    {
	    	PRINTK("YUSU_SET_MMAP_INDEX\n");
	    	Mmap_index = arg;  // save mmap index
	    	break;
	    }
	    case YUSU_GET_ASM_BUFFER_SIZE:
	    {
	         ret = Asm_Control->u4BufferSize;
	    	PRINTK("YUSU_GET_ASM_BUFFER_SIZE\n");
	    	break;
	    }
	     case YUSU_STOP_IOCTL: // this ioctl is only use for debugging
	    {
	    	printk("YUSU_STOP_IOCTL arg = %ld \n",arg);
	    	LouderSPKSound(300);
	        	break;
	    }
	     case YUSU_Set_2IN1_SPEAKER:
	    {
	    #ifdef CONFIG_MT6516_GEMINI_BOARD
	    	PRINTK("CONFIG_MT6516_GEMINI_BOARD YUSU_Set_2IN1_SPEAKER arg = %ld \n",arg);
		msleep(5);
		if(arg == 0 ){
		    pmic_asw_bsel(HI_Z);
		}
		else{
		    pmic_asw_bsel(RECEIVER);
		}
	    #endif
	        	break;
	    }
	     case YUSU_OPEN_OUTPUT_STREAM:
	     {
	       	PRINTK("YUSU_OPEN_OUTPUT_STREAM\n");
	       	ret = Yusu_Sound_Allocate_Stream(fp);
	       	if(ret >=0)
	       	    Asm_Control->u4SRCBlockOccupiedNum ++;
	       	break;  // allocate this fp with one asm block
	     }
	     case YUSU_CLOSE_OUTPUT_STREAM:
	     {
	       	PRINTK("YUSU_CLOSE_OUTPUT_STREAM\n");
	       	ret = Yusu_Sound_Deallocate_Stream(fp);
	       	if(ret >=0)
	       	    Asm_Control->u4SRCBlockOccupiedNum --;
	       	break;
	     }
	     case YUSU_SET_OUTPUT_ATTRIBUTE:
	     {
	        	PRINTK("YUSU_CLOSE_OUTPUT_STREAM\n");
	        	break;
	     }
	     case YUSU_OPEN_INPUT_STREAM:
	     {
	       	PRINTK("YUSU_OPEN_INPUT_STREAM\n");
	       	ret = Yusu_Sound_Allocate_InputStream(fp);
	       	if(ret >=0){
	       	    Yusu_Init_InputStream(arg);
	       	    Edi_Control->u4SRCBlockOccupiedNum ++;
	       	}
	       	break;
	     }
	     case YUSU_CLOSE_INPUT_STREAM:
	     {
	       	PRINTK("YUSU_CLOSE_INPUT_STREAM\n");
	       	ret = Yusu_Sound_Deallocate_InputStream(fp);
	       	if(ret >=0){
	       	    Yusu_DeInit_InputStream();
	       	    Edi_Control->u4SRCBlockOccupiedNum --;
	       	}
	       	PRINTK("YUSU_CLOSE_INPUT_STREAM\n");
	       	break;
	     }
	     case YUSU_OUPUT_STREAM_START:
	     {
	     	PRINTK("YUSU_OUPUT_STREAM_START arg = %ld\n",arg);
	    	spin_lock_bh(&sound_lock);
		Asm_Memory_Count ++;
	    	spin_unlock_bh(&sound_lock);
		if(Asm_Memory_Count == 1 ){
		    wake_lock(&Audio_suspend_lock);
		    printk("YUSU_OUPUT_STREAM_START Asm_Memory_Count = %d\n",Asm_Memory_Count);
		    Yusu_Sound_Power_On();
		    Asm_Enable_Memory_Power ();	//enable memory buffer
		    Yusu_Clear_Stream_Buf();
		}
		else{
		    printk("YUSU_OUPUT_STREAM_START Asm_Memory_Count >=1 =  %d\n",Asm_Memory_Count);
		}
	       	break;
	     }
	     case YUSU_OUPUT_STREAM_STANDBY:
	     {
	     	Yusu_Sound_Standby(fp,arg);
	    	spin_lock_bh(&sound_lock);
		Asm_Memory_Count--;
		spin_unlock_bh(&sound_lock);
		if(Asm_Memory_Count == 0 ){
		    printk("YUSU_OUPUT_STREAM_STANDBY Asm_Memory_Count = %ld\n",Asm_Memory_Count);
		    Yusu_Clear_Stream_Buf();
		    Asm_Disable_Memory_Power ();	//disable memory buffer
		    Yusu_Sound_Power_Off();
		    wake_unlock(&Audio_suspend_lock);
		}
		else if(Asm_Memory_Count <0){
		    printk("YUSU_OUPUT_STREAM_STANDBY Asm_Memory_Count = %d set to 0\n",Asm_Memory_Count);
		    spin_lock_bh(&sound_lock);
		    Asm_Memory_Count = 0;
		    spin_unlock_bh(&sound_lock);
		    wake_unlock(&Audio_suspend_lock);
		}
	       	break;
	     }
	     case YUSU_INPUT_STREAM_START:
	     {
	     	PRINTK("YUSU_INPUT_STREAM_START Asm_Memory_Count = %ld\n",Asm_Memory_Count);
	    	spin_lock_bh(&sound_lock);
		Asm_Memory_Count ++;
		spin_unlock_bh(&sound_lock);
		if(Asm_Memory_Count == 1  ){
		    printk("YUSU_INPUT_STREAM_START Asm_Memory_Count = %d\n ",Asm_Memory_Count);
		    Yusu_Sound_Power_On();
		    Asm_Enable_Memory_Power();	//enable memory buffer
		}
	       	break;
	     }
	     case YUSU_INPUT_STREAM_STANDBY:
	     {
	     	PRINTK("YUSU_INPUT_STREAM_STANDBY Asm_Memory_Count = %ld\n",Asm_Memory_Count);
	     	Yusu_Sound_EDI_Standby(fp,arg);
	    	spin_lock_bh(&sound_lock);
		Asm_Memory_Count--;
		spin_unlock_bh(&sound_lock);
		if(Asm_Memory_Count == 0 ){
		    printk("YUSU_OUPUT_STREAM_STANDBY Asm_Memory_Count = %d\n",Asm_Memory_Count);
		    Asm_Disable_Memory_Power ();	//disable memory buffer
		    Yusu_Sound_Power_Off();
		}
		else if(Asm_Memory_Count < 0){
		    printk("YUSU_OUPUT_STREAM_STANDBY Asm_Memory_Count = %d set to 0\n",Asm_Memory_Count);
		    spin_lock_bh(&sound_lock);
		    Asm_Memory_Count = 0;
		    spin_unlock_bh(&sound_lock);
		}
	       	break;
	     }
	     case YUSU_SET_SPEAKER_VOL:
	     {
	         PRINTK("  YUSU_SET_SPEAKER_VOL level = %u\n",arg);
	         Sound_Speaker_SetVolLevel(arg);
	         break;
	     }
	     case YUSU_SET_SPEAKER_ON:
	     {
	        spin_lock_bh(&sound_lock);
		if(Asm_Amp_Flag != true){
	 	     Asm_Amp_Flag =true;
	 	     spin_unlock_bh(&sound_lock);
		     printk("YUSU_SET_SPEAKER_ON arg= %u\n",arg);
		     Sound_Speaker_Turnon(arg);
		 }
		 else{
		     spin_unlock_bh(&sound_lock);
		 }
		 break;
	     }
	     case YUSU_SET_SPEAKER_OFF:
	     {
	         spin_lock_bh(&sound_lock);
	         if(Asm_Amp_Flag !=false){
	             Asm_Amp_Flag = false;
	             spin_unlock_bh(&sound_lock);
	             printk("YUSU_SET_SPEAKER_OFF arg= %u\n",arg);
	             Sound_Speaker_Turnoff(arg);
	         }
	         else{
	             spin_unlock_bh(&sound_lock);
	         }
	         break;
	     }
	     case YUSU_SET_HEADSET:
	     {
	         spin_lock_bh(&sound_lock);
	         PRINTK("YUSU_SET_HEADSET arg= %u\n",arg);
	         if(arg){
	             Sound_Headset_Turnon();
	         }
	         else{
	             Sound_Headset_Turnoff();
	         }
	         spin_unlock_bh(&sound_lock);
	         break;
	     }
	    case YUSU_INIT_STREAM:
	    {
            	PRINTK("YUSU_INIT_STREAM command buffer size = %x\n",arg);

            	if(Audio_Flush_Count){
            	    printk("YUSU_INIT_STREAM Audio_Flush_Count = %d\n",Audio_Flush_Count);
            	    Audio_Flush_Count=0;
            	    Yusu_Sound_Flush_Reset();
            	}
		//init stream
		Yusu_Init_Stream(arg);
		Sound_Speaker_Turnoff(Channel_Stereo);
		break;
	    }
	    case YUSU_Set_AUDIO_STATE:
	    {
	        printk("YUSU_Set_AUDIO_STATE ");
	    	if(copy_from_user((void *)(&Audio_Control_State), (const void __user *)( arg), sizeof(Audio_Control_T))){
	    	    return -EFAULT;
	    	}
	    	printk("bBgsFlag=%d, bRecordFlag=%d, bSpeechFlag=%d, bTtyFlag=%d \n",
	    	Audio_Control_State.bBgsFlag,Audio_Control_State.bRecordFlag,Audio_Control_State.bSpeechFlag,Audio_Control_State.bTtyFlag);
	    	break;
	    }
	    case YUSU_Get_AUDIO_STATE:
	    {
	        printk("YUSU_Get_AUDIO_STATE ");
	    	if(copy_to_user((void __user *)arg,(void *)&Audio_Control_State, sizeof(Audio_Control_T))){
	    	    return -EFAULT;
	    	}
	    	printk("bBgsFlag=%d, bRecordFlag=%d, bSpeechFlag=%d, bTtyFlag=%d \n",
	    	Audio_Control_State.bBgsFlag,Audio_Control_State.bRecordFlag,Audio_Control_State.bSpeechFlag,Audio_Control_State.bTtyFlag);
	    	break;
	    }
	    case YUSU_ASSERT_IOCTL:
	    {
		PRINTK("YUSU_ASSERT_IOCTL\n");
		BUG_ON(1);
		break;
	    }
	    default:
	    {
	    	PRINTK("can't find a IOCTL command\r\n");
	    	break;
	    }
	}
	Yusu_Sound_Power_Off();
	return ret;
}

void Yusu_Sound_vma_open(struct vm_area_struct *vma)
{
    PRINTK("Yusu_Sound_vma_open virt %lx, phys %lx length = %lx \n",vma->vm_start, vma->vm_pgoff<<PAGE_SHIFT,vma->vm_end - vma->vm_start);
}

void Yusu_Sound_vma_close(struct vm_area_struct *vma)
{
    PRINTK("Yusu_Sound_vma_close virt");
}

static struct vm_operations_struct Yusu_Sound_remap_vm_ops =
{
    .open = Yusu_Sound_vma_open,
    .close = Yusu_Sound_vma_close
};

static struct file_operations Yusu_Sound_fops = {
	.owner      = THIS_MODULE,
	.open        = Yusu_Sound_open,
	.release    = Yusu_Sound_release,
	.ioctl         = Yusu_Sound_ioctl,
	.write        = Yusu_Sound_write,
	.read   	= Yusu_Sound_read,
	.flush        = Yusu_Sound_flush,
	.fasync	    = Yusu_Sound_fasync,
	.mmap     = Yusu_Sound_remap_mmap
};

static struct miscdevice Yusu_audio_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "eac",
	.fops = &Yusu_Sound_fops,
};

static int Yusu_sound_probe(struct platform_device *dev)
{
	int ret = 0;
    // register MISC device
	if((ret = misc_register(&Yusu_audio_device))){
		PRINTK("misc_register returned %d in goldfish_audio_init\n", ret);
		return 1;
	}
	ret = request_irq(MT6516_ASM_IRQ_LINE, asm_irq_handler, 0, "Asm_isr_handle", dev);
	if(ret < 0 ){
		PRINTK(" Unable to request_irq \n");
	}
	return 0;
}

static int Yusu_sound_remove(struct platform_device *dev)
{
	tasklet_kill(&magic_tasklet_handle);
	spin_lock_bh(&sound_lock);
	Yusu_Sound_Power_Off();  // turn off asm afe clock
	spin_unlock_bh(&sound_lock);
	PRINTK("Unregistering Yusu_sound_remove\n");
	return 0;
}

static void Yusu_sound_shutdown(struct platform_device *dev)
{
	PRINTK("sound_shutdown \n");
	spin_lock_bh(&sound_lock);
	Yusu_Sound_Power_Off();  // turn off asm afe clock
	spin_unlock_bh(&sound_lock);
}

static void Yusu_sound_Store_reg(void)
{
    Yusu_Sound_Power_On();
    Suspend_reg.Suspend_ASM_IBUF_BASE0 = Asm_Get(ASM_IBUF_BASE0);
    Suspend_reg.Suspend_ASM_IBUF_END0 = Asm_Get(ASM_IBUF_END0);
    Suspend_reg.Suspend_ASM_SET0 = Asm_Get(ASM_SET0);
    Suspend_reg.Suspend_ASM_GAIN0 = Asm_Get(ASM_GAIN0);
    Suspend_reg.Suspend_ASM_IR_CNT0 = Asm_Get(ASM_IR_CNT0);

    Suspend_reg.Suspend_ASM_IBUF_BASE1 = Asm_Get(ASM_IBUF_BASE1);
    Suspend_reg.Suspend_ASM_IBUF_END1 = Asm_Get(ASM_IBUF_END1);
    Suspend_reg.Suspend_ASM_SET1 = Asm_Get(ASM_SET1);
    Suspend_reg.Suspend_ASM_GAIN1 = Asm_Get(ASM_GAIN1);
    Suspend_reg.Suspend_ASM_IR_CNT1 = Asm_Get(ASM_IR_CNT1);

    Suspend_reg.Suspend_ASM_IBUF_BASE2 = Asm_Get(ASM_IBUF_BASE2);
    Suspend_reg.Suspend_ASM_IBUF_END2 = Asm_Get(ASM_IBUF_END2);
    Suspend_reg.Suspend_ASM_SET2 = Asm_Get(ASM_SET2);
    Suspend_reg.Suspend_ASM_GAIN2 = Asm_Get(ASM_GAIN2);
    Suspend_reg.Suspend_ASM_IR_CNT2 = Asm_Get(ASM_IR_CNT2);

    Suspend_reg.Suspend_ASM_CON0 = Asm_Get(ASM_CON0);
    Suspend_reg.Suspend_ASM_CON1 = Asm_Get(ASM_CON1);
    Suspend_reg.Suspend_ASM_SIN_CON = Asm_Get(ASM_SIN_CON);
    Suspend_reg.Suspend_ASM_AFE_CON0 = Asm_Get(ASM_AFE_CON0);
    Suspend_reg.Suspend_ASM_AFE_CON1 = Asm_Get(ASM_AFE_CON1);
    Suspend_reg.Suspend_ASM_AFE_CON2 = Asm_Get(ASM_AFE_CON2);
    Suspend_reg.Suspend_AFE_EDI_CON = Asm_Get(AFE_EDI_CON);
    Suspend_reg.Suspend_ASM_EDI_BASE = Asm_Get(ASM_EDI_BASE);
    Suspend_reg.Suspend_ASM_EDI_END = Asm_Get(ASM_EDI_END);
    Suspend_reg.Suspend_ASM_EDI_CON1 = Asm_Get(ASM_EDI_CON1);

    Suspend_reg.Suspend_AFE_AAG_CON = Ana_Get(AFE_AAG_CON);
    Suspend_reg.Suspend_AFE_AAC_CON = Ana_Get(AFE_AAC_CON);
    Suspend_reg.Suspend_AFE_AAPDB_CON = Ana_Get(AFE_AAPDB_CON);
    Suspend_reg.Suspend_AFE_AAC_NEW_CON = Ana_Get(AFE_AAC_NEW_CON);

    Ana_Set(AFE_AAPDB_CON,0x0000, 0xffff);
    Yusu_Depop_Off();

    Asm_Set(ASM_CON0,0x0,ASM_MASK_ALL);
    Asm_Set(ASM_AFE_CON0,0x0,ASM_MASK_ALL);
    Asm_Set(ASM_IR_CLR,0x000f,ASM_MASK_ALL);  // clear interrupt
    mdelay(5);  // wait for a shor latency
    Asm_Set(ASM_IR_CLR,0x000f,ASM_MASK_ALL);  // clear interrupt
    Yusu_Sound_Power_Off();

}

static void Yusu_sound_Recover_reg(void)
{
    Yusu_Sound_Power_On();
    Yusu_Depop_On();

    Asm_Set(ASM_IBUF_BASE0,Suspend_reg.Suspend_ASM_IBUF_BASE0, ASM_MASK_ALL);
    Asm_Set(ASM_IBUF_END0,Suspend_reg.Suspend_ASM_IBUF_END0, ASM_MASK_ALL);
    Asm_Set(ASM_SET0,Suspend_reg.Suspend_ASM_SET0, ASM_MASK_ALL);
    Asm_Set(ASM_GAIN0,Suspend_reg.Suspend_ASM_GAIN0, ASM_MASK_ALL);
    Asm_Set(ASM_IR_CNT0,Suspend_reg.Suspend_ASM_IR_CNT0, ASM_MASK_ALL);

    Asm_Set(ASM_IBUF_BASE1,Suspend_reg.Suspend_ASM_IBUF_BASE1, ASM_MASK_ALL);
    Asm_Set(ASM_IBUF_END1,Suspend_reg.Suspend_ASM_IBUF_END1, ASM_MASK_ALL);
    Asm_Set(ASM_SET1,Suspend_reg.Suspend_ASM_SET1, ASM_MASK_ALL);
    Asm_Set(ASM_GAIN1,Suspend_reg.Suspend_ASM_GAIN1, ASM_MASK_ALL);
    Asm_Set(ASM_IR_CNT1,Suspend_reg.Suspend_ASM_IR_CNT1, ASM_MASK_ALL);

    Asm_Set(ASM_IBUF_BASE2,Suspend_reg.Suspend_ASM_IBUF_BASE2, ASM_MASK_ALL);
    Asm_Set(ASM_IBUF_END2,Suspend_reg.Suspend_ASM_IBUF_END2, ASM_MASK_ALL);
    Asm_Set(ASM_SET2,Suspend_reg.Suspend_ASM_SET2, ASM_MASK_ALL);
    Asm_Set(ASM_GAIN2,Suspend_reg.Suspend_ASM_GAIN2, ASM_MASK_ALL);
    Asm_Set(ASM_IR_CNT2,Suspend_reg.Suspend_ASM_IR_CNT2, ASM_MASK_ALL);

    Asm_Set(ASM_CON0,Suspend_reg.Suspend_ASM_CON0, ASM_MASK_ALL);
    Asm_Set(ASM_CON1,Suspend_reg.Suspend_ASM_CON1, ASM_MASK_ALL);
    Asm_Set(ASM_SIN_CON,Suspend_reg.Suspend_ASM_SIN_CON, ASM_MASK_ALL);
    Asm_Set(ASM_AFE_CON0,Suspend_reg.Suspend_ASM_AFE_CON0, ASM_MASK_ALL);
    Asm_Set(ASM_AFE_CON1,Suspend_reg.Suspend_ASM_AFE_CON1, ASM_MASK_ALL);
    Asm_Set(ASM_AFE_CON2,Suspend_reg.Suspend_ASM_AFE_CON2, ASM_MASK_ALL);
    Asm_Set(AFE_EDI_CON,Suspend_reg.Suspend_AFE_EDI_CON, ASM_MASK_ALL);
    Asm_Set(ASM_EDI_BASE,Suspend_reg.Suspend_ASM_EDI_BASE, ASM_MASK_ALL);
    Asm_Set(ASM_EDI_END,Suspend_reg.Suspend_ASM_EDI_END, ASM_MASK_ALL);
    Asm_Set(ASM_EDI_CON1,Suspend_reg.Suspend_ASM_EDI_CON1, ANA_MASK_ALL);

    Ana_Set(AFE_AAC_CON,Suspend_reg.Suspend_AFE_AAC_CON, ANA_MASK_ALL);
    Ana_Set(AFE_AAC_NEW_CON,Suspend_reg.Suspend_AFE_AAC_NEW_CON, ANA_MASK_ALL);
    Ana_Set(AFE_AAG_CON,Suspend_reg.Suspend_AFE_AAG_CON, ANA_MASK_ALL);
    Ana_Set(AFE_AAPDB_CON,Suspend_reg.Suspend_AFE_AAPDB_CON, 0x1c);
    mdelay(1);
    Ana_Set(AFE_AAPDB_CON,Suspend_reg.Suspend_AFE_AAPDB_CON, 0x1f);
    Yusu_Sound_Power_Off();
}

static int Yusu_sound_suspend(struct platform_device *dev, pm_message_t state)  // only one suspend mode
{
    printk("[Audio]sound_suspend \n");
    if(Suspen_Mode == false){

       // when suspend , if is not in speech mode , close amp.
        if(Asm_Amp_Flag ==true && Audio_Control_State.bSpeechFlag== false ){
            Sound_Speaker_Turnoff(Channel_Stereo);//turn off speaker
            mdelay(1);
        }

        spin_lock_bh(&sound_lock);

        /*
        // remove this with CR , this should let headset driver control.
        MT6516_EINTIRQMask(7);
        MT6516_EINTIRQMask(6);
        Sound_Headset_Unset_Gpio();
        */

        Yusu_Clear_Stream_Buf();
        Yusu_sound_Store_reg();
        tasklet_disable(&magic_tasklet_handle);  // disable tasklet

        printk("Yusu_sound_suspend Asm_Memory_Count = %d Clock_Count = %d\n",Asm_Memory_Count,Clock_Count);
        Asm_Disable_Memory_Power ();	//disable memory buffer
        Yusu_Sound_Suspend_Power_Off();  // turn off asm afe clock

        /*
            *MT6516_EINT_MASK_SET = (1<<7);
        */

        Suspen_Mode = true;// set suspend mode to true
        spin_unlock_bh(&sound_lock);
    }
    return 0;
}

static int Yusu_sound_resume(struct platform_device *dev) // wake up
{
    PRINTK("[Audio] sound_resume\n");
    if(Suspen_Mode == true){
        Yusu_Clear_Stream_Buf();
        /*
        // remove this with CR , this should let headset driver control.
        MT6516_EINTIRQUnmask(7);
        MT6516_EINTIRQUnmask(6);
        Sound_Headset_Set_Gpio();
        */

        printk("Yusu_sound_resume Asm_Memory_Count = %d Clock_Count = %d\n",Asm_Memory_Count,Clock_Count);
        if(Asm_Memory_Count){
            printk("Yusu_sound_suspend Asm_Memory_Count = %d Clock_Count = %d\n",Asm_Memory_Count,Clock_Count);
            Asm_Enable_Memory_Power();	//enable memory bufferClass
            Yusu_Sound_Suspend_Power_On();
            Yusu_Clear_Stream_Buf();
        }
        spin_lock_bh(&sound_lock);
        tasklet_enable(&magic_tasklet_handle); // enable tasklet
        Yusu_sound_Recover_reg();

        /*
        *MT6516_EINT_INTACK = (1 << 7);
        *MT6516_EINT_MASK_CLR = (1 << 7);
        */

        Suspen_Mode = false;  // set suspend mode to false
        spin_unlock_bh(&sound_lock);

        // when resume , if amp is closed , reopen it.
        if(Asm_Amp_Flag ==true && Audio_Control_State.bSpeechFlag== false){
           Sound_Speaker_Turnon(Channel_Stereo);
        }
    }
    return 0;
}

static int Yusu_Sound_remap_mmap(struct file *flip, struct vm_area_struct *vma)
{
	PRINTK("Yusu_Sound_remap_mmap Mmap_index = %d \n",Mmap_index);
	if(Mmap_index>=0 && Mmap_index<3) // mmap
	{
	    PRINTK("Yusu_Sound_remap_mmap MMAP ASM BLOCK %d \n",Mmap_index);
        	    vma->vm_pgoff =( Asm_Control->rSRCBlock[Mmap_index].pucPhysBufAddr)>>PAGE_SHIFT;
       	    if(remap_pfn_range(vma , vma->vm_start , vma->vm_pgoff ,
       	     	vma->vm_end - vma->vm_start , vma->vm_page_prot) < 0){
       	     	PRINTK("remap_pfn_range failed\n");
		return -EIO;
		}
   	    vma->vm_ops = &Yusu_Sound_remap_vm_ops;
        	    Yusu_Sound_vma_open(vma);
        	    return 0;
	}
	else  // no mmap
	{
	     PRINTK("Yusu_Sound_remap_mmap No mmap \n");
	     return -1;
	}
}

static struct platform_driver sound_driver = {
	.probe		= Yusu_sound_probe,
	.remove		= Yusu_sound_remove,
	.shutdown  	= Yusu_sound_shutdown,
	.suspend	         = Yusu_sound_suspend,
	.resume		= Yusu_sound_resume,
	.driver     = {
	.name       = "Yusu_Sound_Driver",
	},
};

static int  sound_mod_init(void)
{
	int ret;
	//power on
	Yusu_Sound_Power_On();
	//depop-on
	Yusu_Depop_On();
	Speaker_Init();

	PRINTK("Yusu sound engine registering driver\n");
	// below register Sound platform and driver
	ret = platform_device_register(&Yusu_device_sound);
	if (ret) {
		PRINTK("Unable to device register(%d)\n", ret);
		Yusu_Sound_Power_Off();
		return ret;
	}
	ret = platform_driver_register(&sound_driver);
	if (ret) {
		PRINTK("Unable to register driver (%d)\n", ret);
		Yusu_Sound_Power_Off();
		return ret;
	}
	create_proc_read_entry("Audio",
		0,
		NULL,
		Audio_Read_Procmem,
		NULL
		);
	PRINTK("Yusu sound engine registering driver Done\n");
	wake_lock_init(&Audio_suspend_lock, WAKE_LOCK_SUSPEND, "Audio wakelock");
	PRINTK("Audio_suspend_lock wakelock init\n");
        Yusu_Sound_Power_Off();
	return 0;
}

static void  sound_mod_exit(void)
{
	PRINTK("Unregistering driver\n");
	platform_driver_unregister(&sound_driver);
	PRINTK("Done\n");
}

module_init(sound_mod_init);
module_exit(sound_mod_exit);

MODULE_DESCRIPTION("Yusu_sound_driver");
MODULE_AUTHOR("ChiPeng <ChiPeng.Chang@mediatek.com>");
MODULE_LICENSE("GPL");


