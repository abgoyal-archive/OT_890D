

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/wakelock.h>

#include <mach/irqs.h>
#include <mach/hardware.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_pll.h>
#include <mach/ccci_md.h>
#include <mach/ccci.h>

#include <asm/mach/time.h>
#include <asm/tcm.h>



typedef struct 
{
    void (*eint_func[EINT_MAX_CHANNEL])(void);
    kal_bool eint_active[EINT_MAX_CHANNEL];
    kal_bool eint_auto_umask[EINT_MAX_CHANNEL];
    kal_bool sensitive[EINT_MAX_CHANNEL];
    kal_bool pol[EINT_MAX_CHANNEL];
    kal_uint32 debounce_cn[EINT_MAX_CHANNEL];
    kal_bool debounce_en[EINT_MAX_CHANNEL];
} eint_func;



//SYSTEM_EXCHANGE_DATA SysExData;
static eint_func EINT_FUNC;
struct wake_lock EINT_suspend_lock;

#ifdef AP_MD_EINT_SHARE_DATA
struct core_eint_config eintdata[24];
static ccci_core_eint_config_setup config_setup_func = NULL;
#endif

extern UINT32 suspend_lock ;
extern void mt6516_pm_WakeReason(UINT32 u4IRQ);



void MT6516_IRQMask(unsigned int line)
{
    if (line <= 31)
        *MT6516_IRQ_MASK_SETL = (1 << line);
    else
        *MT6516_IRQ_MASK_SETH = (1 << (line - 32));
}

void MT6516_IRQUnmask(unsigned int line)
{

    if (line <= 31)
        *MT6516_IRQ_MASK_CLRL = (1 << line);
    else
        *MT6516_IRQ_MASK_CLRH = (1 << (line - 32));
}

void MT6516_IRQClearInt(unsigned int line)
{
    MT6516_IRQMask(line);
    *MT6516_IRQ_EOI2 = line;
    if (suspend_lock)
        mt6516_pm_WakeReason(line);
}

void MT6516_EINTIRQMask(unsigned int line)
{
    *MT6516_EINT_MASK_SET = 1 << line;
}

void MT6516_EINTIRQUnmask(unsigned int line)
{
    *MT6516_EINT_MASK_CLR = 1 << line;
}


void MT6516_EINTIRQClearInt(unsigned int line)
{
    MT6516_EINTIRQMask(line);
    *MT6516_EINT_INTACK = 1 << line;
    *MT6516_IRQ_EOI2 = MT6516_EIT_IRQ_LINE; // MT6516_EIT_CODE   
}


void MT6516_EINT_Set_HW_Debounce(kal_uint8 eintno, kal_uint32 ms)
{
    kal_uint32 cnt;
    
    /* calculate cnt value based on 32KHz clock cycles */
    cnt = ms << 5;
    
    /* clear original debounce value */
    EINTaddr(eintno) &= ~EINT_CON_DEBOUNCE;
    
    /* set new debounce value */
    EINTaddr(eintno) |= (cnt | EINT_CON_DEBOUNCE_EN);

    EINT_FUNC.debounce_cn[eintno] = cnt;

    EINT_FUNC.debounce_en[eintno] = KAL_TRUE;
}


void MT6516_EINT_Set_Polarity(kal_uint8 eintno, kal_bool ACT_Polarity)
{
    kal_uint32 count;
    
    /* Set external interrupt polarity */
    if (ACT_Polarity) 
        EINTaddr(eintno) |= EINT_CON_HIGHLEVEL;
    else
        EINTaddr(eintno) &= ~(EINT_CON_HIGHLEVEL);
    
    for (count=0; count<250; count++) ;  /*250:52M*/
    
    *MT6516_EINT_INTACK = (1 << (eintno));
        
    *MT6516_IRQ_EOI2 = eintno;

    EINT_FUNC.pol[eintno] = ACT_Polarity;
}

kal_uint32 MT6516_EINT_Set_Sensitivity(kal_uint8 eintno, kal_bool sens)
{

    if (sens == MT6516_EDGE_SENSITIVE) 
    {
        *MT6516_EINT_SENS &= ~(1 << eintno);
    } 
    else if (sens == MT6516_LEVEL_SENSITIVE) 
    {
        *MT6516_EINT_SENS |= (1 << eintno);
    }
    EINT_FUNC.sensitive[eintno] = sens;

   return 0;
}

static irqreturn_t MT6516_EINT_ISR(int irq, void *dev_id)
{
    kal_uint8 index;
    kal_uint32 status;

	//wake_lock_timeout(&EINT_suspend_lock, 5 * HZ);
	wake_lock_timeout(&EINT_suspend_lock, HZ / 2);

	
    status = *MT6516_EINT_STATUS;
    
    // for conventional external interrupt!
    for(index=0;index<EINT_MAX_CHANNEL;index++)
    {
        if (status & (1 << (index)) ) 
        {
            // disable interrupt
            MT6516_EINTIRQMask(index);
            //printk("EINT %d is triggered\n\r",index);
            if ( EINT_FUNC.eint_func[index] )
            {
                EINT_FUNC.eint_func[index]();                
            }
            if (EINT_FUNC.eint_auto_umask[index])
                MT6516_EINTIRQUnmask(index);
            *MT6516_EINT_INTACK = (1 << (index));
        }
    }
    return IRQ_HANDLED;
}

void MT6516_EINT_Registration
(
    kal_uint8 eintno, 
    kal_bool Dbounce_En, 
    kal_bool ACT_Polarity,
    void (EINT_FUNC_PTR)(void), 
    kal_bool auto_umask
)
{
    kal_uint16 count;
    kal_uint16 status;
    
    status =  request_irq(MT6516_EIT_IRQ_LINE, MT6516_EINT_ISR, 0, "MT3351_EIT", NULL);
    
    
    /* Set external interrupt polarity */
    if (ACT_Polarity) 
        EINTaddr(eintno) |= EINT_CON_HIGHLEVEL;
    else
        EINTaddr(eintno) &= ~(EINT_CON_HIGHLEVEL);
    
    if (eintno < EINT_MAX_CHANNEL) 
    {
        /* Set external interrupt debounce control enable or not */
        if (Dbounce_En) 
        {
            /* check if debounce is enabled */
            if (EINTaddr(eintno) & EINT_CON_DEBOUNCE_EN) 
            {
                /* debounce value is already set; NOP */
            } 
            else 
            {
                EINTaddr(eintno) |= (EINT_CON_DEBOUNCE | EINT_CON_DEBOUNCE_EN);
            }
        }
        else
        {
            /*disable debounce */
            EINTaddr(eintno) &= ~(EINT_CON_DEBOUNCE_EN);
        }
    }
    
    /* Register external interrupt's callback */
    EINT_FUNC.eint_func[eintno] = EINT_FUNC_PTR;
    EINT_FUNC.eint_active[eintno] = KAL_TRUE;
    EINT_FUNC.eint_auto_umask[eintno] = auto_umask;
    
    
    for (count=0; count<250; count++) ;  /*250:52M*/
    
    
    *MT6516_EINT_INTACK = 1 << eintno;
    
    *MT6516_EINT_MASK_CLR = 1 << eintno;

}


void MT6516_FIQMask(void)
{
    *MT6516_FIQ_CON |= 0x01;
}

void MT6516_FIQUnmask(void)
{
    *MT6516_FIQ_CON &= ~(0x01);
}

void MT6516_IRQSensitivity(unsigned char code, unsigned char sens)
{   
    if (sens == MT6516_EDGE_SENSITIVE) 
    { 
        if (code <= 31)
            *MT6516_IRQ_SENSL &= (~(1 << code));
        else
            *MT6516_IRQ_SENSH &= (~(1 << (code - 32)));
    } 
    else 
    {        /* level sensitive interrupt */
        if (code <= 31)
            *MT6516_IRQ_SENSL |= (1 << code);
        else
            *MT6516_IRQ_SENSH |= (1 << (code - 32));
    }
}

#ifdef AP_MD_EINT_SHARE_DATA
INT32 mt6516_EINT_SetMDShareInfo(void)
{
    UINT32 i = 0, counter= 0, u4Address= 0;
    for (i = 0; i< EINT_MAX_CHANNEL; i++)
    {
        if (EINT_FUNC.eint_active[i])
        {
            eintdata[counter].eint_no = i ;
            eintdata[counter].ACT_Polarity = EINT_FUNC.pol[i];
            eintdata[counter].Dbounce_En = EINT_FUNC.debounce_en[i];
            eintdata[counter].Dbounce_ms = (EINT_FUNC.debounce_cn[i]>>5 ) ;
            eintdata[counter].Sensitivity = EINT_FUNC.sensitive[i];
            counter++ ;
        }    
    }
    u4Address = (UINT32)virt_to_phys(&eintdata);

    if (config_setup_func)
        config_setup_func(counter, (void*)u4Address);

    return 0;
}    

void mt6516_EINT_register_config_setup_func(ccci_core_eint_config_setup func)
{
	printk("mt6516_EINT_register_config_setup_func\n");
	config_setup_func = func;
	mt6516_EINT_SetMDShareInfo();
}

void mt6516_EINT_unregister_config_setup_func(void)
{
	config_setup_func = NULL;
}
#endif

void MT6516_EINT_init(void)
{
    UINT32 i;
    for (i = 0; i< EINT_MAX_CHANNEL; i++)
    {
        EINT_FUNC.eint_func[i] = NULL;
        EINT_FUNC.eint_active[i] = KAL_FALSE;
        EINT_FUNC.eint_auto_umask[i] = KAL_FALSE;
        EINT_FUNC.sensitive[i] = MT6516_LEVEL_SENSITIVE;
        EINT_FUNC.pol[i] = MT6516_NEG_POL;
        EINT_FUNC.debounce_cn[i] = 0;
        EINT_FUNC.debounce_en[i] = KAL_FALSE;
    }    
	wake_lock_init(&EINT_suspend_lock, WAKE_LOCK_SUSPEND, "EINT wakelock");
    MT6516_IRQSensitivity(MT6516_EIT_IRQ_LINE, MT6516_LEVEL_SENSITIVE);

}    

#ifdef AP_MD_EINT_SHARE_DATA
EXPORT_SYMBOL(mt6516_EINT_register_config_setup_func);
EXPORT_SYMBOL(mt6516_EINT_unregister_config_setup_func);
#endif
EXPORT_SYMBOL(MT6516_IRQMask);
EXPORT_SYMBOL(MT6516_IRQUnmask);
EXPORT_SYMBOL(MT6516_IRQSensitivity);
