


#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/miscdevice.h>

#include <asm/io.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>

#include <mach/mt6516.h>
#include <mach/mt6516_devs.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_nand.h>
#include "partition.h"

/* Added for TCM used */
#include <asm/tcm.h>

#define VERSION  	"v3.0"
#define MODULE_NAME	"# MT6516 NAND #"
#define PROCNAME    "driver/nand"

#if NAND_OTP_SUPPORT

#define SAMSUNG_OTP_SUPPORT     1
#define OTP_MAGIC_NUM           0x4E3AF28B
#define SAMSUNG_OTP_PAGE_NUM    6

static const unsigned int Samsung_OTP_Page[SAMSUNG_OTP_PAGE_NUM] = {0x15, 0x16, 0x17, 0x18, 0x19, 0x1b};

static struct mt6516_otp_config g_mt6516_otp_fuc;
static spinlock_t g_OTPLock;

#define OTP_MAGIC           'k'

/* NAND OTP IO control number */
#define OTP_GET_LENGTH 		_IOW(OTP_MAGIC, 1, int)
#define OTP_READ 	        _IOW(OTP_MAGIC, 2, int)
#define OTP_WRITE 			_IOW(OTP_MAGIC, 3, int)

#define FS_OTP_READ         0
#define FS_OTP_WRITE        1

/* NAND OTP Error codes */
#define OTP_SUCCESS                   0
#define OTP_ERROR_OVERSCOPE          -1
#define OTP_ERROR_TIMEOUT            -2
#define OTP_ERROR_BUSY               -3
#define OTP_ERROR_NOMEM              -4
#define OTP_ERROR_RESET              -5

#endif


#define NFI_SET_REG32(reg, value)   (DRV_WriteReg32(reg, DRV_Reg32(reg) | (value)))
#define NFI_SET_REG16(reg, value)   (DRV_WriteReg16(reg, DRV_Reg16(reg) | (value)))
#define NFI_CLN_REG32(reg, value)   (DRV_WriteReg32(reg, DRV_Reg32(reg) & (~(value))))
#define NFI_CLN_REG16(reg, value)   (DRV_WriteReg16(reg, DRV_Reg16(reg) & (~(value))))

#define NFI_WAIT_STATE_DONE(state) do{;}while (__raw_readl(NFI_STA_REG32) & state)
#define NFI_WAIT_TO_READY()  do{;}while (!(__raw_readl(NFI_STA_REG32) & STA_BUSY2READY))

#define __DEBUG_NAND		1			/* Debug information on/off */

/* Debug message event */
#define DBG_EVT_NONE		0x00000000	/* No event */
#define DBG_EVT_INIT		0x00000001	/* Initial related event */
#define DBG_EVT_VERIFY		0x00000002	/* Verify buffer related event */
#define DBG_EVT_PERFORMANCE	0x00000004	/* Performance related event */
#define DBG_EVT_READ		0x00000008	/* Read related event */
#define DBG_EVT_WRITE		0x00000010	/* Write related event */
#define DBG_EVT_ERASE		0x00000020	/* Erase related event */
#define DBG_EVT_BADBLOCK	0x00000040	/* Badblock related event */
#define DBG_EVT_POWERCTL	0x00000080	/* Suspend/Resume related event */
#define DBG_EVT_OTP         0x00000100   /* NAND OTP related event */

#define DBG_EVT_ALL			0xffffffff

#define DBG_EVT_MASK      	(DBG_EVT_INIT | DBG_EVT_OTP)

#if __DEBUG_NAND
#define MSG(evt, fmt, args...) \
do {	\
	if ((DBG_EVT_##evt) & DBG_EVT_MASK) { \
		printk(fmt, ##args); \
	} \
} while(0)

#define MSG_FUNC_ENTRY(f)	MSG(FUC, "<FUN_ENT>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{}while(0)
#define MSG_FUNC_ENTRY(f)	   do{}while(0)
#endif
#define NAND_SECTOR_SIZE (512)


#ifdef NAND_PFM
static suseconds_t g_PFM_R = 0;
static suseconds_t g_PFM_W = 0;
static suseconds_t g_PFM_E = 0;
static u32 g_PFM_RNum = 0;
static u32 g_PFM_RD = 0;
static u32 g_PFM_WD = 0;
static struct timeval g_now;

#define PFM_BEGIN(time) \
do_gettimeofday(&g_now); \
(time) = g_now;

#define PFM_END_R(time, n) \
do_gettimeofday(&g_now); \
g_PFM_R += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
g_PFM_RNum += 1; \
g_PFM_RD += n; \
MSG(PERFORMANCE, "%s - Read PFM: %lu, data: %d, ReadOOB: %d (%d, %d)\n", MODULE_NAME , g_PFM_R, g_PFM_RD, g_kCMD.pureReadOOB, g_kCMD.pureReadOOBNum, g_PFM_RNum);

#define PFM_END_W(time, n) \
do_gettimeofday(&g_now); \
g_PFM_W += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
g_PFM_WD += n; \
MSG(PERFORMANCE, "%s - Write PFM: %lu, data: %d\n", MODULE_NAME, g_PFM_W, g_PFM_WD);

#define PFM_END_E(time) \
do_gettimeofday(&g_now); \
g_PFM_E += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
MSG(PERFORMANCE, "%s - Erase PFM: %lu\n", MODULE_NAME, g_PFM_E);
#else
#define PFM_BEGIN(time)
#define PFM_END_R(time, n)
#define PFM_END_W(time, n)
#define PFM_END_E(time)
#endif

//-------------------------------------------------------------------------------
static struct completion g_comp_AHB_Done;
static struct mt6516_CMD g_kCMD;
static u32 g_u4ChipVer;
static bool g_bInitDone;
static int g_i4Interrupt;
static int g_page_size;

/* Modified for TCM used */
static __tcmfunc irqreturn_t mt6516_nand_irq_handler(int irqno, void *dev_id)
//static irqreturn_t mt6516_nand_irq_handler(int irqno, void *dev_id)
{
   u16 u16IntStatus = DRV_Reg16(NFI_INTR_REG16);
   	(void)irqno;

    if (u16IntStatus & (u16)INTR_AHB_DONE_EN)
	{
    	complete(&g_comp_AHB_Done);
    } 
    return IRQ_HANDLED;
}

static void ECC_Config(struct mt6516_nand_host_hw *hw)
{
	u32 u4ENCODESize;
	u32 u4DECODESize;

    DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
    do{;}while (!DRV_Reg16(ECC_DECIDLE_REG16));

    DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
    do{;}while (!DRV_Reg32(ECC_ENCIDLE_REG32));

	/* setup FDM register base */
	DRV_WriteReg32(ECC_FDMADDR_REG32, NFI_FDM0L_REG32);

    /* Sector + FDM */
    u4ENCODESize = (hw->nand_sec_size + 8) << 3;
    /* Sector + FDM + YAFFS2 meta data bits */
	u4DECODESize = ((hw->nand_sec_size + 8) << 3) + 4 * 13; 

	/* configure ECC decoder && encoder*/
	DRV_WriteReg32(ECC_DECCNFG_REG32,
		ECC_CNFG_ECC4|DEC_CNFG_NFI|DEC_CNFG_EMPTY_EN|
		(u4DECODESize << DEC_CNFG_CODE_SHIFT));

	DRV_WriteReg32(ECC_ENCCNFG_REG32, 
		ECC_CNFG_ECC4|ENC_CNFG_NFI|
		(u4ENCODESize << ENC_CNFG_MSG_SHIFT));

#if USE_AHB_MODE
	NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_CORRECT);
#else
	NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_EL);
#endif
}

static void ECC_Decode_Start(void)
{
   	/* wait for device returning idle */
	while(!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE));
	DRV_WriteReg16(ECC_DECCON_REG16, DEC_EN);
}

static void ECC_Decode_End(void)
{
   /* wait for device returning idle */
	while(!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE));
	DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
}

static void ECC_Encode_Start(void)
{
   /* wait for device returning idle */
	while(!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE));
	DRV_WriteReg16(ECC_ENCCON_REG16, ENC_EN);
}

static void ECC_Encode_End(void)
{
   /* wait for device returning idle */
	while(!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE));
	DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
}

static bool mt6516_nand_check_bch_error(
	struct mtd_info *mtd, u8* pDataBuf, u32 u4SecIndex, u32 u4PageAddr)
{
	bool bRet = true;
	u16 u2SectorDoneMask = 1 << u4SecIndex;
	u32 u4ErrorNumDebug, i, u4ErrNum;
	u32 timeout = 0xFFFF;
#if !USE_AHB_MODE
	u32 au4ErrBitLoc[6];
	u32 u4ErrByteLoc, u4BitOffset;
	u32 u4ErrBitLoc1th, u4ErrBitLoc2nd;
#endif

	//4 // Wait for Decode Done
	while (0 == (u2SectorDoneMask & DRV_Reg16(ECC_DECDONE_REG16)))
    {       
		timeout--;
		if (0 == timeout)
        {
			return false;
		}
	}
#if (USE_AHB_MODE)
	u4ErrorNumDebug = DRV_Reg32(ECC_DECENUM_REG32);
	if (0 != (u4ErrorNumDebug & 0xFFFF))
    {
		for (i = 0; i <= u4SecIndex; ++i)
        {
			u4ErrNum = DRV_Reg32(ECC_DECENUM_REG32) >> (i << 2);
			u4ErrNum &= 0xF;
            
			if (0xF == u4ErrNum)
            {
				mtd->ecc_stats.failed++;
				bRet = false;
				//printk(KERN_ERR"UnCorrectable at PageAddr=%d, Sector=%d\n", u4PageAddr, i);
			} 
            else 
            {
				mtd->ecc_stats.corrected++;
				//printk(KERN_ERR"Correct at PageAddr=%d, Sector=%d\n", u4ErrNum, u4PageAddr, i);
			}
		}
	}
#else
	memset(au4ErrBitLoc, 0x0, sizeof(au4ErrBitLoc));
	u4ErrorNumDebug = DRV_Reg32(ECC_DECENUM_REG32);
	u4ErrNum = DRV_Reg32(ECC_DECENUM_REG32) >> (u4SecIndex << 2);
	u4ErrNum &= 0xF;
    
	if (u4ErrNum)
    {
		if (0xF == u4ErrNum)
        {
			mtd->ecc_stats.failed++;
			bRet = false;
			//printk(KERN_ERR"UnCorrectable at PageAddr=%d\n", u4PageAddr);
		} 
        else 
        {
			for (i = 0; i < ((u4ErrNum+1)>>1); ++i)
            {
				au4ErrBitLoc[i] = DRV_Reg32(ECC_DECEL0_REG32 + i);
				u4ErrBitLoc1th = au4ErrBitLoc[i] & 0x1FFF;
                
				if (u4ErrBitLoc1th < 0x1000)
                {
					u4ErrByteLoc = u4ErrBitLoc1th/8;
					u4BitOffset = u4ErrBitLoc1th%8;
					pDataBuf[u4ErrByteLoc] = pDataBuf[u4ErrByteLoc]^(1<<u4BitOffset);
					mtd->ecc_stats.corrected++;
				} 
                else 
                {
					mtd->ecc_stats.failed++;
					//printk(KERN_ERR"UnCorrectable ErrLoc=%d\n", au4ErrBitLoc[i]);
				}
				u4ErrBitLoc2nd = (au4ErrBitLoc[i] >> 16) & 0x1FFF;
				if (0 != u4ErrBitLoc2nd)
                {
					if (u4ErrBitLoc2nd < 0x1000)
                    {
						u4ErrByteLoc = u4ErrBitLoc2nd/8;
						u4BitOffset = u4ErrBitLoc2nd%8;
						pDataBuf[u4ErrByteLoc] = pDataBuf[u4ErrByteLoc]^(1<<u4BitOffset);
						mtd->ecc_stats.corrected++;
					} 
                    else 
                    {
						mtd->ecc_stats.failed++;
						//printk(KERN_ERR"UnCorrectable High ErrLoc=%d\n", au4ErrBitLoc[i]);
					}
				}
			}
		}
		if (0 == (DRV_Reg16(ECC_DECFER_REG16) & (1 << u4SecIndex)))
        {
			bRet = false;
		}
	}
#endif
	return bRet;
}

static bool mt6516_nand_RFIFOValidSize(u16 u2Size)
{
	u32 timeout = 0xFFFF;
	while (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) < u2Size)
    {
		timeout--;
		if (0 == timeout){
			return false;
		}
	}
	if(u2Size==0)
	{
		while (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)))
		{
			timeout--;
			if (0 == timeout){
				return false;
			}
		}
	}	
	return true;
}

static bool mt6516_nand_WFIFOValidSize(u16 u2Size)
{
	u32 timeout = 0xFFFF;

	while (FIFO_WR_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) > u2Size)
    {
		timeout--;
		if (0 == timeout)
        {
			return false;
		}
	}
	if(u2Size==0)
	{
		while (FIFO_WR_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)))
		{
			timeout--;
			if (0 == timeout){

				return false;
			}
		}
	}	
	return true;
}

static bool mt6516_nand_status_ready(u32 u4Status)
{
	u32 timeout = 0xFFFF;
	while ((DRV_Reg32(NFI_STA_REG32) & u4Status) != 0)
    {
		timeout--;
		if (0 == timeout)
        {
			return false;
		}
	}
	return true;
}

static bool mt6516_nand_reset(void)
{
	/* issue reset operation */
	DRV_WriteReg16(NFI_CON_REG16, CON_FIFO_FLUSH | CON_NFI_RST);

	return mt6516_nand_status_ready(STA_NFI_FSM_MASK|STA_NAND_BUSY) &&
		   mt6516_nand_RFIFOValidSize(0) &&
		   mt6516_nand_WFIFOValidSize(0);
}

static void mt6516_nand_set_mode(u16 u2OpMode)
{
	u16 u2Mode = DRV_Reg16(NFI_CNFG_REG16);
	u2Mode &= ~CNFG_OP_MODE_MASK;
	u2Mode |= u2OpMode;
	DRV_WriteReg16(NFI_CNFG_REG16, u2Mode);
}

static void mt6516_nand_set_autoformat(bool bEnable)
{
	if (bEnable)
    {
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AUTO_FMT_EN);
	}
    else
    {
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AUTO_FMT_EN);
	}
}

static void mt6516_nand_configure_fdm(u16 u2FDMSize)
{
	NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_FDM_MASK | PAGEFMT_FDM_ECC_MASK);
	NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMSize << PAGEFMT_FDM_SHIFT);
	NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMSize << PAGEFMT_FDM_ECC_SHIFT);
}

static void mt6516_nand_configure_lock(void)
{
	u32 u4WriteColNOB = 2;
	u32 u4WriteRowNOB = 3;
	u32 u4EraseColNOB = 0;
	u32 u4EraseRowNOB = 3;
	DRV_WriteReg16(NFI_LOCKANOB_REG16, 
		(u4WriteColNOB << PROG_CADD_NOB_SHIFT)  |
		(u4WriteRowNOB << PROG_RADD_NOB_SHIFT)  |
		(u4EraseColNOB << ERASE_CADD_NOB_SHIFT) |
		(u4EraseRowNOB << ERASE_RADD_NOB_SHIFT));

	if (CHIPVER_ECO_1 == g_u4ChipVer)
    {
		int i;
		for (i = 0; i < 16; ++i)
        {
			DRV_WriteReg32(NFI_LOCK00ADD_REG32 + (i << 1), 0xFFFFFFFF);
			DRV_WriteReg32(NFI_LOCK00FMT_REG32 + (i << 1), 0xFFFFFFFF);
		}
		//DRV_WriteReg16(NFI_LOCKANOB_REG16, 0x0);
		DRV_WriteReg32(NFI_LOCKCON_REG32, 0xFFFFFFFF);
		DRV_WriteReg16(NFI_LOCK_REG16, NFI_LOCK_ON);
	}	
}

static bool mt6516_nand_set_command(u16 command)
{
	/* Write command to device */
	DRV_WriteReg16(NFI_CMD_REG16, command);	
	return mt6516_nand_status_ready(STA_CMD_STATE);
}

static bool mt6516_nand_set_address(u32 u4ColAddr, u32 u4RowAddr, u16 u2ColNOB, u16 u2RowNOB)
{
	/* fill cycle addr */
	DRV_WriteReg32(NFI_COLADDR_REG32, u4ColAddr);
	DRV_WriteReg32(NFI_ROWADDR_REG32, u4RowAddr);
	DRV_WriteReg16(NFI_ADDRNOB_REG16, u2ColNOB|(u2RowNOB << ADDR_ROW_NOB_SHIFT));
	return mt6516_nand_status_ready(STA_ADDR_STATE);
}

static bool mt6516_nand_check_RW_count(u16 u2WriteSize)
{
	u32 timeout = 0xFFFF;
	u16 u2SecNum = u2WriteSize >> 9;
    
	while (ADDRCNTR_CNTR(DRV_Reg16(NFI_ADDRCNTR_REG16)) < u2SecNum)
    {
		timeout--;
		if (0 == timeout)
        {
			return false;
		}
	}
	return true;
}

static bool mt6516_nand_ready_for_read(struct nand_chip *nand, u32 u4RowAddr, u8 *pDataBuf)
{
	/* Reset NFI HW internal state machine and flush NFI in/out FIFO */	
	bool bRet = false;

	u16 sec_num = 1 << (nand->page_shift - 9);
	
	if (!mt6516_nand_reset())
    {
		goto cleanup;
	}

	mt6516_nand_set_mode(CNFG_OP_READ);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
	DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

#if USE_AHB_MODE
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
	DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(pDataBuf));

#else
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif

	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	mt6516_nand_set_autoformat(true);
	ECC_Decode_Start();

	if (!mt6516_nand_set_command(NAND_CMD_READ0))
    {
		goto cleanup;
	}

	//1 FIXED ME: For Any Kind of AddrCycle
	if (!mt6516_nand_set_address(0, u4RowAddr, 2, 3))
    {
		goto cleanup;
	}

	if (!mt6516_nand_set_command(NAND_CMD_READSTART))
    {
		goto cleanup;
	}

	if (!mt6516_nand_status_ready(STA_NAND_BUSY))
    {
		goto cleanup;
	}

	bRet = true;
	
cleanup:
	return bRet;
}

static bool mt6516_nand_ready_for_write(
	struct nand_chip *nand, u32 u4RowAddr, u8 *pDataBuf)
{

	bool bRet = false;
	u16 sec_num = 1 << (nand->page_shift - 9);
	
	/* Reset NFI HW internal state machine and flush NFI in/out FIFO */	
	if (!mt6516_nand_reset())
    {
		return false;
	}

	mt6516_nand_set_mode(CNFG_OP_PRGM);
	
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
	
	DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

#if USE_AHB_MODE
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
	DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(pDataBuf));

#else
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif

	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	
	mt6516_nand_set_autoformat(true);

	ECC_Encode_Start();

	if (!mt6516_nand_set_command(NAND_CMD_SEQIN)){
		goto cleanup;
	}

	//1 FIXED ME: For Any Kind of AddrCycle
	if (!mt6516_nand_set_address(0, u4RowAddr, 2, 3)){
		goto cleanup;
	}

	if (!mt6516_nand_status_ready(STA_NAND_BUSY)){
		goto cleanup;
	}

	bRet = true;
cleanup:

	return bRet;
}

static bool mt6516_nand_read_page_data(u8* pDataBuf, u32 u4Size)
{
	int i4Interrupt = g_i4Interrupt;
	u32 timeout = 0xFFFF;

#if (USE_AHB_MODE)
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	// DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(pDataBuf));

    
	if (i4Interrupt) 
    {
	    init_completion(&g_comp_AHB_Done);
	    DRV_Reg16(NFI_INTR_REG16);
	    DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);
	}
	dmac_inv_range(pDataBuf, pDataBuf + u4Size);
	NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BRD);
    
	if (i4Interrupt) 
    {
	    wait_for_completion(&g_comp_AHB_Done);
	} 
    else 
    {
	    while (u4Size > DRV_Reg16(NFI_BYTELEN_REG16))
        {
		    timeout--;
		    if (0 == timeout)
            {
			    return false; //4  // AHB Mode Time Out!
			}
		}
	}
#else
	u32 i;
	u32* pBuf32;
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BRD);
	pBuf32 = (u32*)pDataBuf;
	for (i = 0; (i < (u4Size >> 2))&&(timeout > 0);)
    {
		if (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) >= 4)
        {
			*pBuf32++ = DRV_Reg32(NFI_DATAR_REG32);
			i++;
		} 
        else 
        {
			timeout--;
		}
		if (0 == timeout)
        {
			return false; //4 // MCU  Mode Time Out!
		}
	}
#endif
	return true;
}

static bool mt6516_nand_write_page_data(u8* pDataBuf, u32 u4Size)
{
	int i4Interrupt = g_i4Interrupt;
	u32 timeout = 0xFFFF;

#if (USE_AHB_MODE)
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    
	if (i4Interrupt) 
    {
		init_completion(&g_comp_AHB_Done);
		DRV_Reg16(NFI_INTR_REG16);
		DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);
	}    
	dmac_clean_range(pDataBuf, pDataBuf + u4Size);
	NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BWR);
    
	if (i4Interrupt) 
    {
		wait_for_completion(&g_comp_AHB_Done);
	} 
    else 
    {
		while (u4Size > DRV_Reg16(NFI_BYTELEN_REG16))
        {
			timeout--;
			if (0 == timeout)
            {
				return false; //4  // AHB Mode Time Out!
			}
		}
	}	
#else
	u32 i;	
	u32* pBuf32;
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BWR);
	pBuf32 = (u32*)pDataBuf;
	for (i = 0; (i < (u4Size >> 2))&&(timeout > 0);)
    {
		if (FIFO_WR_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) <= 12)
        {
			DRV_WriteReg32(NFI_DATAW_REG32, *pBuf32++);
			i++;
		} 
        else 
        {
			timeout--;
		}
		if (0 == timeout)
        {
			return false; //4 // MCU Mode Time Out!
		}
	}
#endif

	return true;
}

static void mt6516_nand_read_fdm_data(u8* pDataBuf, u32 u4SecNum)
{
	u32 i;
	u32* pBuf32 = (u32*)pDataBuf;
	for (i = 0; i < u4SecNum; ++i)
	{
		*pBuf32++ = DRV_Reg32(NFI_FDM0L_REG32 + (i<<1));
		*pBuf32++ = DRV_Reg32(NFI_FDM0M_REG32 + (i<<1));
		//*pBuf32++ = DRV_Reg32((u32)NFI_FDM0L_REG32 + (i<<3));
		//*pBuf32++ = DRV_Reg32((u32)NFI_FDM0M_REG32 + (i<<3));
	}
}

static void mt6516_nand_write_fdm_data(u8* pDataBuf, u32 u4SecNum)
{
	u32 i;
	u32* pBuf32 = (u32*)pDataBuf;
	for (i = 0; i < u4SecNum; ++i)
	{
		DRV_WriteReg32(NFI_FDM0L_REG32 + (i<<1), *pBuf32++);
		DRV_WriteReg32(NFI_FDM0M_REG32 + (i<<1), *pBuf32++);
		//DRV_WriteReg32((u32)NFI_FDM0L_REG32 + (i<<3), *pBuf32++);
		//DRV_WriteReg32((u32)NFI_FDM0M_REG32 + (i<<3), *pBuf32++);
	}
}

static void mt6516_nand_stop_read(void)
{
	NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BRD);
	ECC_Decode_End();
}

static void mt6516_nand_stop_write(void)
{
	NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BWR);
	ECC_Encode_End();
}

static bool mt6516_nand_exec_read_page(
	struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8* pPageBuf, u8* pFDMBuf)
{
	bool bRet = true;
	struct nand_chip *nand = mtd->priv;
	u32 u4SecNum = u4PageSize >> 9;
#ifdef NAND_PFM
	struct timeval pfm_time_read;
#endif
	PFM_BEGIN(pfm_time_read);
	if (mt6516_nand_ready_for_read(nand, u4RowAddr, pPageBuf))
	{
		if (!mt6516_nand_read_page_data(pPageBuf, u4PageSize))
		{
			bRet = false;
		}
        
		if (!mt6516_nand_status_ready(STA_NAND_BUSY))
		{
			bRet = false;
		}
        
		mt6516_nand_read_fdm_data(pFDMBuf, u4SecNum);
        
		if (!mt6516_nand_check_bch_error(mtd, pPageBuf, u4SecNum - 1, u4RowAddr))
		{
			bRet = false;
		}
		mt6516_nand_stop_read();
	}
	PFM_END_R(pfm_time_read, u4PageSize + 32);
	return bRet;
}

static void mt6516_nand_exec_write_page(
	struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8* pPageBuf, u8* pFDMBuf)
{
	struct nand_chip *nand = mtd->priv;
	u32 u4SecNum = u4PageSize >> 9;
#ifdef NAND_PFM
	struct timeval pfm_time_write;
#endif
	PFM_BEGIN(pfm_time_write);
	if (mt6516_nand_ready_for_write(nand, u4RowAddr, pPageBuf))
	{
		mt6516_nand_write_fdm_data(pFDMBuf, u4SecNum);
		(void)mt6516_nand_write_page_data(pPageBuf, u4PageSize);
		(void)mt6516_nand_check_RW_count(u4PageSize);
		mt6516_nand_stop_write();
        (void)mt6516_nand_set_command(NAND_CMD_PAGEPROG);
		while(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);		
	}
	PFM_END_W(pfm_time_write, u4PageSize + 32);
}
//-------------------------------------------------------------------------------

static void mt6516_nand_command_bp(struct mtd_info *mtd, unsigned int command,
			 int column, int page_addr)
{
	struct nand_chip* nand = mtd->priv;
#ifdef NAND_PFM
	struct timeval pfm_time_erase;
#endif
    switch (command) 
    {
        case NAND_CMD_SEQIN:
		    /* Reset g_kCMD */
		//if (g_kCMD.u4RowAddr != page_addr) {
			memset(g_kCMD.au1OOB, 0xFF, sizeof(g_kCMD.au1OOB));
			g_kCMD.pDataBuf = NULL;
        //}
		    g_kCMD.u4RowAddr = page_addr;
		    g_kCMD.u4ColAddr = column;
            break;

        case NAND_CMD_PAGEPROG:
           	if (g_kCMD.pDataBuf || (0xFF != g_kCMD.au1OOB[0])) 
    		{
           		u8* pDataBuf = g_kCMD.pDataBuf ? g_kCMD.pDataBuf : nand->buffers->databuf;
    			mt6516_nand_exec_write_page(mtd, g_kCMD.u4RowAddr, mtd->writesize, pDataBuf, g_kCMD.au1OOB);
    			g_kCMD.u4RowAddr = (u32)-1;
    			g_kCMD.u4OOBRowAddr = (u32)-1;
            }
            break;

        case NAND_CMD_READOOB:
    		g_kCMD.u4RowAddr = page_addr;        	
    		g_kCMD.u4ColAddr = column + mtd->writesize;
    		#ifdef NAND_PFM
    		g_kCMD.pureReadOOB = 1;
    		g_kCMD.pureReadOOBNum += 1;
    		#endif
			break;
			
        case NAND_CMD_READ0:
    		g_kCMD.u4RowAddr = page_addr;        	
    		g_kCMD.u4ColAddr = column;
    		#ifdef NAND_PFM
    		g_kCMD.pureReadOOB = 0;
    		#endif		
			break;

        case NAND_CMD_ERASE1:
    		PFM_BEGIN(pfm_time_erase);
    		(void)mt6516_nand_reset();
            mt6516_nand_set_mode(CNFG_OP_ERASE);
    		(void)mt6516_nand_set_command(NAND_CMD_ERASE1);
    		(void)mt6516_nand_set_address(0,page_addr,0,3);
            break;
            
        case NAND_CMD_ERASE2:
       	    (void)mt6516_nand_set_command(NAND_CMD_ERASE2);
			while(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);
		    PFM_END_E(pfm_time_erase);
            break;
            
        case NAND_CMD_STATUS:
        	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
		    (void)mt6516_nand_reset();        	
			mt6516_nand_set_mode(CNFG_OP_SRD);
		    (void)mt6516_nand_set_command(NAND_CMD_STATUS);
        	NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_NOB_MASK);
			DRV_WriteReg16(NFI_CON_REG16, CON_NFI_SRD|(1 << CON_NFI_NOB_SHIFT));
            break;
            
        case NAND_CMD_RESET:
       	    (void)mt6516_nand_reset();
			//mt6516_nand_exec_reset_device();
            break;

		case NAND_CMD_READID:
			NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN|CNFG_BYTE_RW);
		    (void)mt6516_nand_reset();
			mt6516_nand_set_mode(CNFG_OP_SRD);
		    (void)mt6516_nand_set_command(NAND_CMD_READID);
		    (void)mt6516_nand_set_address(0,0,1,0);
			DRV_WriteReg16(NFI_CON_REG16, CON_NFI_SRD);
			while(DRV_Reg32(NFI_STA_REG32) & STA_DATAR_STATE);
			break;
            
        default:
            BUG();        
            break;
    }
 }

static void mt6516_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if (chip == -1 && false == g_bInitDone)
	{
		struct nand_chip *nand = mtd->priv;
		/* Setup PageFormat */
	    if (2048 == mtd->writesize) 
		{
       		NFI_SET_REG16(NFI_PAGEFMT_REG16, (PAGEFMT_SPARE_16 << PAGEFMT_SPARE_SHIFT) | PAGEFMT_2K);
			nand->cmdfunc = mt6516_nand_command_bp;
        }/* else if (512 == mtd->writesize) {
       		NFI_SET_REG16(NFI_PAGEFMT_REG16, (PAGEFMT_SPARE_16 << PAGEFMT_SPARE_SHIFT) | PAGEFMT_512);
	       	nand->cmdfunc = mt6516_nand_command_sp;
    	}*/
		g_bInitDone = true;
	}
    switch(chip)
    {
	case -1:
		break;
	case 0: 
	case 1:
		DRV_WriteReg16(NFI_CSEL_REG16, chip);
		break;
    }
}

static uint8_t mt6516_nand_read_byte(struct mtd_info *mtd)
{
	while(0 == FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)));
	return DRV_Reg8(NFI_DATAR_REG32);
}

static void mt6516_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip* nand = (struct nand_chip*)mtd->priv;
	struct mt6516_CMD* pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
		
	if (u4ColAddr < u4PageSize) 
	{
		if ((u4ColAddr == 0) && (len >= u4PageSize)) 
		{
			mt6516_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, 
									   buf, pkCMD->au1OOB);
			if (len > u4PageSize) 
			{
				u32 u4Size = min(len - u4PageSize, sizeof(pkCMD->au1OOB));
				memcpy(buf + u4PageSize, pkCMD->au1OOB, u4Size);
			}
		} 
		else 
		{
			mt6516_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, 
									   nand->buffers->databuf, pkCMD->au1OOB);
			memcpy(buf, nand->buffers->databuf + u4ColAddr, len);
		}
		pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
	} 
	else 
	{
		u32 u4Offset = u4ColAddr - u4PageSize;
		u32 u4Size = min(len - u4Offset, sizeof(pkCMD->au1OOB));
		if (pkCMD->u4OOBRowAddr != pkCMD->u4RowAddr) 
		{
			mt6516_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize,
									   nand->buffers->databuf, pkCMD->au1OOB);
			pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
		}
		memcpy(buf, pkCMD->au1OOB + u4Offset, u4Size);
	}
	pkCMD->u4ColAddr += len;	
}

static void mt6516_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct mt6516_CMD* pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;

	if (u4ColAddr >= u4PageSize) 
    {
	    u32 u4Offset = u4ColAddr - u4PageSize;
		u8* pOOB = pkCMD->au1OOB + u4Offset;
		int i4Size = min(len, (int)sizeof(pkCMD->au1OOB) - u4Offset);
		u32 i;
        
		for (i = 0; i < i4Size; i++) 
        {
			pOOB[i] &= buf[i];
		}
	} 
    else 
    {
		pkCMD->pDataBuf = (u8*)buf;
    }
    
	pkCMD->u4ColAddr += len;	
}	

static void mt6516_nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	mt6516_nand_write_buf(mtd, buf, mtd->writesize);
	mt6516_nand_write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

static int mt6516_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf)
{
#if 0
	mt6516_nand_read_buf(mtd, buf, mtd->writesize);
	mt6516_nand_read_buf(mtd, chip->oob_poi, mtd->oobsize);
#else
	struct mt6516_CMD* pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
		
	if (u4ColAddr == 0) 
    {
        mt6516_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, buf, chip->oob_poi);
        pkCMD->u4ColAddr += u4PageSize + mtd->oobsize;
	}
#endif
	return 0;
}

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE

char gacBuf[2048 + 64];

static int mt6516_nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
#if 1
	struct nand_chip* chip = (struct nand_chip*)mtd->priv;
	struct mt6516_CMD* pkCMD = &g_kCMD;
	u32 u4PageSize = mtd->writesize;
	u32 *pSrc, *pDst;
	int i;

    mt6516_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, gacBuf, gacBuf + u4PageSize);

	pSrc = (u32*)buf;
	pDst = (u32*)gacBuf;
	len = len/sizeof(u32);
	for (i = 0; i < len; ++i) 
    {
		if (*pSrc != *pDst) 
        {
			MSG(VERIFY, "mt6516_nand_verify_buf page fail at page %d\n", pkCMD->u4RowAddr);
            return -1;
		}
		pSrc++;
		pDst++;
	}
    
	pSrc = (u32*)chip->oob_poi;
	pDst = (u32*)(gacBuf + u4PageSize);
    
	if ((pSrc[0] != pDst[0]) || (pSrc[1] != pDst[1]) ||
	    (pSrc[2] != pDst[2]) || (pSrc[3] != pDst[3]) ||
	    (pSrc[4] != pDst[4]) || (pSrc[5] != pDst[5]))
	    // TODO: Ask Designer Why?
	    //(pSrc[6] != pDst[6]) || (pSrc[7] != pDst[7])) 
    {
        MSG(VERIFY, "mt6516_nand_verify_buf oob fail at page %d\n", pkCMD->u4RowAddr);
		MSG(VERIFY, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
		    pSrc[0], pSrc[1], pSrc[2], pSrc[3], pSrc[4], pSrc[5], pSrc[6], pSrc[7]);
		MSG(VERIFY, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
		    pDst[0], pDst[1], pDst[2], pDst[3], pDst[4], pDst[5], pDst[6], pDst[7]);
		return -1;		
    }
	/*
	for (i = 0; i < len; ++i) {
		if (*pSrc != *pDst) {
			printk(KERN_ERR"mt6516_nand_verify_buf oob fail at page %d\n", g_kCMD.u4RowAddr);
			return -1;
		}
		pSrc++;
		pDst++;
	}
	*/
	//printk(KERN_INFO"mt6516_nand_verify_buf OK at page %d\n", g_kCMD.u4RowAddr);
	
	return 0;
#else
    return 0;
#endif
}
#endif

static void mt6516_nand_init_hw(struct mt6516_nand_host *host)
{
	struct mt6516_nand_host_hw *hw = host->hw;
	
	/* Power on NFI HW component. */
	(void)hwEnableClock(MT6516_PDN_PERI_NFI, "NAND");

	g_bInitDone = false;
    /* Get the HW_VER */
    g_u4ChipVer = DRV_Reg32(CONFIG_BASE);
	g_kCMD.u4OOBRowAddr  = (u32)-1;

    /* Set default NFI access timing control */
	DRV_WriteReg32(NFI_ACCCON_REG32, hw->nfi_access_timing);
	DRV_WriteReg16(NFI_CNFG_REG16, 0);
	DRV_WriteReg16(NFI_PAGEFMT_REG16, 0);	
	
    /* Reset the state machine and data FIFO, because flushing FIFO */
	(void)mt6516_nand_reset();
	
    /* Set the ECC engine */
    if(hw->nand_ecc_mode == NAND_ECC_HW)
	{
		MSG(INIT, "%s : Use HW ECC\n", MODULE_NAME);
		NFI_SET_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		ECC_Config(host->hw);
   		mt6516_nand_configure_fdm(8);
		mt6516_nand_configure_lock();
	}

	/* Initilize interrupt. Clear interrupt, read clear. */
    DRV_Reg16(NFI_INTR_REG16);
	
    /* Interrupt arise when read data or program data to/from AHB is done. */
	DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
}

//-------------------------------------------------------------------------------
static int mt6516_nand_dev_ready(struct mtd_info *mtd)
{	
    return !(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);
}

static int mt6516_nand_proc_read(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	if (off > 0) 
    {
		return 0;
	}
	return sprintf(page, "Interrupt-Scheme is %d\n", g_i4Interrupt);
}

static int mt6516_nand_proc_write(struct file* file, const char* buffer,
	unsigned long count, void *data)
{
	char buf[16];
	int n, len = count;

	if (len >= sizeof(buf)) 
    {
		len = sizeof(buf) - 1;
	}

	if (copy_from_user(buf, buffer, len)) 
    {
		return -EFAULT;
	}

	buf[len] = '\0';

	if (buf[0] == 'I') 
    {
		n = simple_strtol(buf+1, NULL, 10);
        g_i4Interrupt = n > 0 ? 1 : 0;	
        MSG(INIT, "Interrupt is %d\n", g_i4Interrupt);
    } 
	
#ifdef NAND_PFM
	if (buf[0] == 'P') 
    {
        /* Reset values */
		g_PFM_R = 0;
		g_PFM_W = 0;
		g_PFM_E = 0;
		g_PFM_RD = 0;
		g_PFM_WD = 0;
		g_kCMD.pureReadOOBNum = 0;
	}
#endif

	return len;
}

static int mt6516_nand_probe(struct platform_device *pdev)
{
	struct mt6516_nand_host *host;
	struct mt6516_nand_host_hw *hw;	
	struct mtd_info *mtd;
	struct nand_chip *nand_chip;
    struct resource *res = pdev->resource;	
	int err = 0;
   
    hw = (struct mt6516_nand_host_hw*)pdev->dev.platform_data;
    BUG_ON(!hw);

	if (pdev->num_resources != 4 ||
	    res[0].flags != IORESOURCE_MEM || 
	    res[1].flags != IORESOURCE_MEM ||
	    res[2].flags != IORESOURCE_IRQ ||
   	    res[3].flags != IORESOURCE_IRQ)
   	{
		MSG(INIT, "%s: invalid resource type\n", __FUNCTION__);
		return -ENODEV;
	}

	/* Request IO memory */
	if (!request_mem_region(res[0].start,
				            res[0].end - res[0].start + 1, 
				            pdev->name)) 
	{
		return -EBUSY;
	}
	if (!request_mem_region(res[1].start,
				            res[1].end - res[1].start + 1, 
				            pdev->name)) 
	{
		return -EBUSY;
	}

	/* Allocate memory for the device structure (and zero it) */
	host = kzalloc(sizeof(struct mt6516_nand_host), GFP_KERNEL);	
	if (!host) 
	{
		MSG(INIT, "mt6516_nand: failed to allocate device structure.\n");
		return -ENOMEM;
	}

    host->hw = hw;

	/* init mtd data structure */
	nand_chip  = &host->nand_chip;
	nand_chip->priv = host;		/* link the private data structures */
	
	mtd        = &host->mtd;	
	mtd->priv  = nand_chip;
	mtd->owner = THIS_MODULE;
	mtd->name  = "MT6516-Nand";

	/* Set address of NAND IO lines */
	nand_chip->IO_ADDR_R 	    = (void __iomem*)NFI_DATAR_REG32;
	nand_chip->IO_ADDR_W 	    = (void __iomem*)NFI_DATAW_REG32;
	nand_chip->chip_delay 	    = 20;			/* 20us command delay time */
	nand_chip->ecc.mode 	    = hw->nand_ecc_mode;	/* enable ECC */

	nand_chip->read_byte        = mt6516_nand_read_byte;
	nand_chip->read_buf		    = mt6516_nand_read_buf;
	nand_chip->write_buf	    = mt6516_nand_write_buf;
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE	
	nand_chip->verify_buf       = mt6516_nand_verify_buf;
#endif
    nand_chip->select_chip      = mt6516_nand_select_chip;
    nand_chip->dev_ready 	    = mt6516_nand_dev_ready;
	nand_chip->cmdfunc 		    = mt6516_nand_command_bp;	
   	nand_chip->ecc.read_page    = mt6516_nand_read_page_hwecc;
	nand_chip->ecc.write_page   = mt6516_nand_write_page_hwecc;

    nand_chip->ecc.layout	    = &mt6516_nand_oob;
    nand_chip->ecc.size		    = hw->nand_ecc_size;	//2048
    nand_chip->ecc.bytes	    = hw->nand_ecc_bytes;	//32
	//nand_chip->options		    = NAND_USE_FLASH_BBT;
	nand_chip->options		 = NAND_SKIP_BBTSCAN;
	//nand_chip->options		 = NAND_USE_FLASH_BBT | NAND_NO_AUTOINCR;
								/*
							   BBT_AUTO_REFRESH      | 
		                       NAND_NO_SUBPAGE_WRITE | 
		                       NAND_NO_AUTOINCR;
		                       */
	mt6516_nand_init_hw(host);

	/* 16-bit bus width */
	if (hw->nfi_bus_width == 16)
	{
	    MSG(INIT, "%s : Set the 16-bit I/O settings!\n", MODULE_NAME);
		nand_chip->options |= NAND_BUSWIDTH_16;
	}

    /*  register NFI IRQ handler. */
    err = request_irq(MT6516_NFI_IRQ_LINE, mt6516_nand_irq_handler, 0, 
                     "mt6516-nand", NULL);
    if (0 != err) 
	{
        MSG(INIT, "%s : Request IRQ fail: err = %d\n", MODULE_NAME, err);
        goto out;
    }

	/* Scan to find existance of the device */
	if (nand_scan(mtd, hw->nfi_cs_num)) 
	{
		MSG(INIT, "%s : nand_scan fail.\n", MODULE_NAME);
		err = -ENXIO;
		goto out;
	}

	g_page_size = mtd->writesize;
	platform_set_drvdata(pdev, host);
    
    if (hw->nfi_bus_width == 16)
	{
		NFI_SET_REG16(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);
	}


#ifdef CONFIG_MTD_PARTITIONS

    err = add_mtd_partitions(mtd, g_pasStatic_Partition, NUM_PARTITIONS);

#ifdef CONFIG_MTD_NAND_NVRAM

#endif

#else

	err = add_mtd_device(mtd);

#endif

	/* Successfully!! */
	if (!err)
	{
        MSG(INIT, "[mt6516_nand] probe successfully!\n");
		return err;
	}

	/* Fail!! */
out:
	MSG(INIT, "[NFI] mt6516_nand_probe fail, err = %d!\n", err);
	
	nand_release(mtd);
	
	platform_set_drvdata(pdev, NULL);
	
	kfree(host);

	(void)hwDisableClock(MT6516_PDN_PERI_NFI,"NAND");
	
	return err;
}
static int mt6516_nand_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY)
	{
		MSG(POWERCTL, "[NFI] Busy, Suspend Fail !\n");		
		return 1; // BUSY
	}	

	MSG(POWERCTL, "[NFI] Suspend !\n");
	(void)hwDisableClock(MT6516_PDN_PERI_NFI,"NAND");
    return 0;
}
static int mt6516_nand_resume(struct platform_device *pdev)
{
	(void)hwEnableClock(MT6516_PDN_PERI_NFI,"NAND");
	MSG(POWERCTL, "[NFI] Resume !\n");
    return 0;
}

static int __devexit mt6516_nand_remove(struct platform_device *pdev)
{
	struct mt6516_nand_host *host = platform_get_drvdata(pdev);
	struct mtd_info *mtd = &host->mtd;

	nand_release(mtd);

	kfree(host);

	(void)hwDisableClock(MT6516_PDN_PERI_NFI,"NAND");
	
	return 0;
}

static struct platform_driver mt6516_nand_driver = {
	.probe		= mt6516_nand_probe,
	.remove		= mt6516_nand_remove,
	.suspend	= mt6516_nand_suspend,
	.resume	    = mt6516_nand_resume,
	.driver		= {
		.name	= "mt6516-nand",
		.owner	= THIS_MODULE,
	},
};

#if (NAND_OTP_SUPPORT && SAMSUNG_OTP_SUPPORT)
unsigned int samsung_OTPQueryLength(unsigned int *QLength)
{
    *QLength = SAMSUNG_OTP_PAGE_NUM * g_page_size;
	return 0;
}

unsigned int samsung_OTPRead(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;

    if(PageAddr >= SAMSUNG_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
    rowaddr = Samsung_OTP_Page[PageAddr];

    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);

    /* Power on NFI HW component. */
	(void)hwEnableClock(MT6516_PDN_PERI_NFI, "NAND");

	mt6516_nand_reset();
    (void)mt6516_nand_set_command(0x30);
    mt6516_nand_reset();
    (void)mt6516_nand_set_command(0x65);

    MSG(OTP, "[%s]: Start to read data from OTP area\n", __func__);

    if (!mt6516_nand_reset())
    {
        bRet = OTP_ERROR_RESET;
        goto cleanup;
    }

    mt6516_nand_set_mode(CNFG_OP_READ);
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
    DRV_WriteReg16(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);

#if (USE_AHB_MODE)
    DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr));
#endif

    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    mt6516_nand_set_autoformat(true);
    ECC_Decode_Start();

    if (!mt6516_nand_set_command(NAND_CMD_READ0))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6516_nand_set_address(coladdr, rowaddr, 2, 3))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6516_nand_set_command(NAND_CMD_READSTART))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6516_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6516_nand_read_page_data(BufferPtr, u4Size))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6516_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    mt6516_nand_read_fdm_data(SparePtr, 4);

    mt6516_nand_stop_read();

    MSG(OTP, "[%s]: End to read data from OTP area\n", __func__);

    bRet = OTP_SUCCESS;

cleanup:

	mt6516_nand_reset();
    (void)mt6516_nand_set_command(0xFF);

	return bRet;
}

unsigned int samsung_OTPWrite(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;

    if(PageAddr >= SAMSUNG_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
    rowaddr = Samsung_OTP_Page[PageAddr];

    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);

	mt6516_nand_reset();
    (void)mt6516_nand_set_command(0x30);
    mt6516_nand_reset();
    (void)mt6516_nand_set_command(0x65);

    MSG(OTP, "[%s]: Start to write data to OTP area\n", __func__);

    /* Power on NFI HW component. */
	(void)hwEnableClock(MT6516_PDN_PERI_NFI, "NAND");

    if (!mt6516_nand_reset())
    {
		bRet = OTP_ERROR_RESET;
        goto cleanup;
	}

	mt6516_nand_set_mode(CNFG_OP_PRGM);

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

	DRV_WriteReg16(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);

#if (USE_AHB_MODE)
      DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr));
#endif

	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

	mt6516_nand_set_autoformat(true);

	ECC_Encode_Start();

	if (!mt6516_nand_set_command(NAND_CMD_SEQIN))
    {
        bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mt6516_nand_set_address(coladdr, rowaddr, 2, 3))
    {
        bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mt6516_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	mt6516_nand_write_fdm_data(BufferPtr, 4);
	(void)mt6516_nand_write_page_data(BufferPtr, u4Size);
	if(!mt6516_nand_check_RW_count(u4Size))
    {
        MSG(OTP, "[%s]: Check RW count timeout !\n", __func__);
        bRet = OTP_ERROR_TIMEOUT;
        goto cleanup;
    }

	mt6516_nand_stop_write();
    (void)mt6516_nand_set_command(NAND_CMD_PAGEPROG);
	while(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);

    bRet = OTP_SUCCESS;

    MSG(OTP, "[%s]: End to write data to OTP area\n", __func__);

cleanup:
    mt6516_nand_reset();
    (void)mt6516_nand_set_command(0xFF);

    return bRet;
}
#endif

#if NAND_OTP_SUPPORT
static int mt_otp_open(struct inode *inode, struct file *filp)
{
	MSG(OTP, "[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	filp->private_data = (int*)OTP_MAGIC_NUM;
	return 0;
}

static int mt_otp_release(struct inode *inode, struct file *filp)
{
	MSG(OTP, "[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static int mt_otp_access(unsigned int access_type, unsigned int offset, void *buff_ptr, unsigned int length, unsigned int *status)
{
    unsigned int i = 0, ret = 0;
    char *BufAddr = (char *)buff_ptr;
    unsigned int PageAddr, AccessLength=0;
    int Status = 0;

    static char *p_D_Buff = NULL;
    char S_Buff[64];

    if (!(p_D_Buff = kmalloc(g_page_size, GFP_KERNEL)))
    {
        ret = -ENOMEM;
        *status = OTP_ERROR_NOMEM;
        goto exit;
    }

    MSG(OTP, "[%s]: %s (0x%x) length:(%d bytes) !\n", __func__, access_type?"WRITE":"READ", offset, length);

    while(1)
    {
        PageAddr = offset/g_page_size;
        if(FS_OTP_READ == access_type)
        {
            memset(p_D_Buff, 0xff, g_page_size);
            memset(S_Buff, 0xff, (sizeof(char)*64));

            MSG(OTP, "[%s]: Read Access of page (%d)\n",__func__, PageAddr);

            Status = g_mt6516_otp_fuc.OTPRead(PageAddr, p_D_Buff, &S_Buff);
            *status = Status;

            if( OTP_SUCCESS != Status)
            {
                MSG(OTP, "[%s]: Read status (%d)\n", __func__, Status);
                break;
            }

            AccessLength = g_page_size - (offset % g_page_size);

            if(length >= AccessLength)
            {
                memcpy(BufAddr, (p_D_Buff+(offset % g_page_size)), AccessLength);
            }
            else
            {
                //last time
                memcpy(BufAddr, (p_D_Buff+(offset % g_page_size)), length);
            }
        }
        else if(FS_OTP_WRITE == access_type)
        {
            AccessLength = g_page_size - (offset % g_page_size);
            memset(p_D_Buff, 0xff, g_page_size);
            memset(S_Buff, 0xff, (sizeof(char)*64));

            if(length >= AccessLength)
            {
                memcpy((p_D_Buff+(offset % g_page_size)), BufAddr, AccessLength);
            }
            else
            {
                //last time
                memcpy((p_D_Buff+(offset % g_page_size)), BufAddr, length);
            }

            Status = g_mt6516_otp_fuc.OTPWrite(PageAddr, p_D_Buff, &S_Buff);
            *status = Status;

            if( OTP_SUCCESS != Status)
            {
                MSG(OTP, "[%s]: Write status (%d)\n",__func__, Status);
                break;
            }
        }
        else
        {
            MSG(OTP, "[%s]: Error, not either read nor write operations !\n",__func__);
            break;
        }

        offset += AccessLength;
        BufAddr += AccessLength;
        if(length <= AccessLength)
        {
            length = 0;
            break;
        }
        else
        {
            length -= AccessLength;
            MSG(OTP, "[%s]: Remaining %s (%d) !\n",__func__, access_type?"WRITE":"READ", length);
        }
    }
error:
    kfree(p_D_Buff);
exit:
    return ret;
}

static long mt_otp_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0, i=0;
	static char *pbuf = NULL;

	void __user *uarg = (void __user *)arg;
    struct otp_ctl otpctl;

    /* Lock */
    spin_lock(&g_OTPLock);

	if (copy_from_user(&otpctl, uarg, sizeof(struct otp_ctl)))
	{
        ret = -EFAULT;
        goto exit;
    }

    if(false == g_bInitDone)
    {
        MSG(OTP, "ERROR: NAND Flash Not initialized !!\n");
        ret = -EFAULT;
        goto exit;
    }

    if (!(pbuf = kmalloc(sizeof(char)*otpctl.Length, GFP_KERNEL)))
    {
        ret = -ENOMEM;
        goto exit;
    }

	switch (cmd)
    {
	case OTP_GET_LENGTH:
        MSG(OTP, "OTP IOCTL: OTP_GET_LENGTH\n");
        g_mt6516_otp_fuc.OTPQueryLength(&otpctl.QLength);
        otpctl.status = OTP_SUCCESS;
        MSG(OTP, "OTP IOCTL: The Length is %d\n", otpctl.QLength);
        break;
    case OTP_READ:
        MSG(OTP, "OTP IOCTL: OTP_READ Offset(0x%x), Length(0x%x) \n", otpctl.Offset, otpctl.Length);
        memset(pbuf, 0xff, sizeof(char)*otpctl.Length);

        mt_otp_access(FS_OTP_READ, otpctl.Offset, pbuf, otpctl.Length, &otpctl.status);

        if (copy_to_user(otpctl.BufferPtr, pbuf, (sizeof(char)*otpctl.Length)))
        {
            MSG(OTP, "OTP IOCTL: Copy to user buffer Error !\n");
            goto error;
        }
        break;
    case OTP_WRITE:
        MSG(OTP, "OTP IOCTL: OTP_WRITE Offset(0x%x), Length(0x%x) \n", otpctl.Offset, otpctl.Length);
        if (copy_from_user(pbuf, otpctl.BufferPtr, (sizeof(char)*otpctl.Length)))
        {
            MSG(OTP, "OTP IOCTL: Copy from user buffer Error !\n");
            goto error;
        }
        mt_otp_access(FS_OTP_WRITE, otpctl.Offset , pbuf, otpctl.Length, &otpctl.status);
        break;
	default:
		ret = -EINVAL;
	}

    ret = copy_to_user(uarg, &otpctl, sizeof(struct otp_ctl));

error:
    kfree(pbuf);
exit:
    spin_unlock(&g_OTPLock);
    return ret;
}
#endif
#if NAND_OTP_SUPPORT
static struct file_operations nand_otp_fops = {
    .owner=      THIS_MODULE,
    .ioctl=      mt_otp_ioctl,
    .open=       mt_otp_open,
    .release=    mt_otp_release,
};

static struct miscdevice nand_otp_dev = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = "otp",
    .fops    = &nand_otp_fops,
};
#endif

static int __init mt6516_nand_init(void)
{
	struct proc_dir_entry *entry;
#if NAND_OTP_SUPPORT
    int err = 0;
#endif
	
#ifdef CONFIG_MTD_NAND_MT6516_INTERRUPT_SCHEME	
	g_i4Interrupt = 1;
#else
	g_i4Interrupt = 0;
#endif

#if NAND_OTP_SUPPORT
    MSG(OTP, "OTP: register NAND OTP device ...\n");
	err = misc_register(&nand_otp_dev);
	if (unlikely(err))
    {
		MSG(OTP, "OTP: failed to register NAND OTP device!\n");
		return err;
	}
	spin_lock_init(&g_OTPLock);
#endif

#if (NAND_OTP_SUPPORT && SAMSUNG_OTP_SUPPORT)
    g_mt6516_otp_fuc.OTPQueryLength = samsung_OTPQueryLength;
    g_mt6516_otp_fuc.OTPRead = samsung_OTPRead;
    g_mt6516_otp_fuc.OTPWrite = samsung_OTPWrite;
#endif

	entry = create_proc_entry(PROCNAME, 0666, NULL);
	if (entry == NULL) 
	{
		MSG(INIT, "MT6516 Nand : unable to create /proc entry\n");
		return -ENOMEM;
	}
	entry->read_proc = mt6516_nand_proc_read;
	entry->write_proc = mt6516_nand_proc_write;	
	entry->owner = THIS_MODULE;

	MSG(INIT, "MediaTek MT6516 Nand driver init, version %s\n", VERSION);

	return platform_driver_register(&mt6516_nand_driver);
}

static void __exit mt6516_nand_exit(void)
{
	MSG(INIT, "MediaTek MT6516 Nand driver exit, version %s\n", VERSION);
#if NAND_OTP_SUPPORT
	misc_deregister(&nand_otp_dev);
#endif

#ifdef SAMSUNG_OTP_SUPPORT
    g_mt6516_otp_fuc.OTPQueryLength = NULL;
    g_mt6516_otp_fuc.OTPRead = NULL;
    g_mt6516_otp_fuc.OTPWrite = NULL;
#endif
	platform_driver_unregister(&mt6516_nand_driver);
	remove_proc_entry(PROCNAME, NULL);
}

module_init(mt6516_nand_init);
module_exit(mt6516_nand_exit);
MODULE_LICENSE("GPL");
