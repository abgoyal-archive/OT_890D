


#ifndef __MT6516_REG_BASE_H__
#define __REG_BASE_H__

// 0xF0000000
#define EFUSE_BASE      	(0xF0000000)
#define CONFIG_BASE      	(0xF0001000)     
#define GPIO_BASE        	(0xF0002000)     
#define RGU_BASE         	(0xF0003000)

// 0xF0020000
#define EMI_BASE         	(0xF0020000)     
#define CIRQ_BASE        	(0xF0021000)     
#define DMA_BASE         	(0xF0022000)     
#define UART1_BASE       	(0xF0023000)     
#define UART2_BASE       	(0xF0024000)     
#define UART3_BASE       	(0xF0025000)     
#define GPT_BASE         	(0xF0026000)     
#define HDQ_BASE         	(0xF0027000)     
#define KP_BASE         	(0xF0028000)     
#define PWM_BASE         	(0xF0029000)     
#define UART4_BASE       	(0xF002B000)     
#define RTC_BASE         	(0xF002C000)     
#define SEJ_base         	(0xF002D000)     
#define I2C3_BASE        	(0xF002E000)     
#define IRDA_BASE       	(0xF002F000)     

// 0xF0030000
#define I2C_BASE        	(0xF0030000)
#define MSDC1_BASE       	(0xF0031000)
#define NFI_BASE 	      	(0xF0032000)
#define SIM_BASE 	      	(0xF0033000)
#define MSDC2_BASE       	(0xF0034000)
#define I2C2_BASE        	(0xF0035000)
#define CCIF_BASE        	(0xF0036000)
#define NFIECC_BASE      	(0xF0038000)
#define AMCONFG_BASE     	(0xF0039000)
#define AP2MD_BASE	     	(0xF003A000)
#define APVFE_BASE	     	(0xF003B000)
#define APSLP_BASE	     	(0xF003C000)
#define AUXADC_BASE	     	(0xF003D000)
#define APXGPT_BASE	     	(0xF003E000)
#define MSDC3_BASE       	(0xF003F000)

// 0xF0040000
#define CSDBG_BASE				(0xF0040000)

// 0xF0060000
#define PLL_BASE				(0xF0060000)
#define DSI_PHY_BASE            (0xF0060B00)

// 0xF0080000
#define GMC1_BASE				(0xF0080000)
#define G2D_BASE				(0xF0081000)
#define GCMQ_BASE				(0xF0082000)
#define GIFDEC_BASE				(0xF0083000)
#define IMGDMA_BASE				(0xF0084000)
#define PNGDEC_BASE				(0xF0085000)
#define MTVSPI_BASE				(0xF0087000)
#define TVCON_BASE				(0xF0088000)
#define TVENC_BASE				(0xF0089000)
#define CAM_BASE				(0xF008A000)
#define CAM_ISP_BASE				(0xF008B000)
#define BLS_BASE				(0xF008C000)
#define CRZ_BASE				(0xF008D000)
#define DRZ_BASE				(0xF008E000)
#define ASM_BASE				(0xF008F000)

// 0xF0090000
#define WT_BASE					(0xF0090000)
#define IMG_BASE				(0xF0091000)
#define GRAPH1SYS_CONFG_BASE			(0xF0092000)

// 0xF00A0000
#define GMC2_BASE				(0xF00A0000)
#define JPEG_BASE				(0xF00A1000)
#define M3D_BASE				(0xF00A2000)
#define PRZ_BASE				(0xF00A3000)
#define IMGDMA1_BASE				(0xF00A4000)
#define MP4_DEBLK_BASE		    		(0xF00A5000)
#define FAKE_ENG2_BASE				(0xF00A6000)
#define GRAPH2SYS_BASE		    		(0xF00A7000)

// 0xF00C0000
#define MP4_BASE				(0xF00C0000)
#define H264_BASE				(0xF00C1000)

// 0xF0100000
#define USB_BASE            			(0xF0100000)

// 0xF0120000
#define LCD_BASE            			(0xF0120000)

// 0xF0130000
#define DPI_BASE            			(0xF0130000)


#define CEVA_BASE        			(0xF1000000)

#define dbg_print
#endif /*ifndef __REG_BASE_H__ */
