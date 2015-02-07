

#include <linux/autoconf.h>
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/hardware.h>

#define MT3351_DVFS_ITCM_PA	 0xF0400000
#define MT3351_EMI_CONN_PA	 0xF0020068
#define MT3351_CON_REGS_PA	 0xF0001208
#define MT3351_PMU_REGS_PA	 0xF0061300

	.text

ENTRY(DVFS_CLKSwitch_4x)  
		stmdb   r13!,{r4-r7,r14}

//	pITCM_Base = OALPAtoVA(0x50000000, FALSE)//
		ldr     r4,=MT3351_DVFS_ITCM_PA

//	pEMI_CONN =  OALPAtoVA(MT3351_EMI_CONN_PA, FALSE)//
		ldr     r5,=MT3351_EMI_CONN_PA

//	pCONFIG_REGS =  OALPAtoVA(MT3351_CON_REGS_PA, FALSE)//
		ldr     r6,=MT3351_CON_REGS_PA

//	pPMU_REGS =  OALPAtoVA(MT3351_PMU_REGS_PA, FALSE)//
		ldr     r7,=MT3351_PMU_REGS_PA

//	copy suspend code to ITCM
		mov     r0,r4					// destination address
		ldr     r1,=clk_4x_switch_start
		mov     r2,#0x300				// copy 300 byte length 
		bl      memcpy
		mov     pc, r4					// set Program counter to ITCM0
		nop

clk_4x_switch_start:
//	*pEMI_CONN |= 0x20//				//// 2. set DRAM into self refresh mode
		ldr     r3,[r5]
		orr     r3,r3,#0x20
		str     r3,[r5]

//									//// 3. count over 32 bus clk cycle
		mov		r0,#0x40
dvfs_delay1_4x:
		sub		r0,r0,#0x1
		cmp     r0,#0x0
		bgt     dvfs_delay1_4x


// 441  : 	s_pConforDVFSRegs->CLKCON &= 0xFFFCFFFF//

        ldr     r3, [r6, #0]        //// 4. switch clock frequency
        mvn     r4, #3, 16
        and     r3, r3, r4
        str		r3,[r6,#0]
        
// s_pConforDVFSRegs->CLKCON |= ( (ARM_DIV_1 << 16) ) //
// no assembly code

//									
		mov		r6,#0x10            //// 5. count over 8 clk cycle
dvfs_delay2_4x:
		sub		r6,r6,#0x1
		cmp     r6,#0x0
		bgt     dvfs_delay2_4x

		ldr     r3,[r5]             //// 6. disable DRAM 
		bic     r3,r3,#0x1
		str     r3,[r5]

//	*pEMI_CONN &= ~0x20//			//// 7. leave self refresh mode 
		ldr     r3,[r5]
		bic     r3,r3,#0x20
		str     r3,[r5]

		mov		r6,#0xFF            //// 8. count over 200 clk cycle
dvfs_delay3_4x:
		sub		r6,r6,#0x1
		cmp     r6,#0x0
		bgt     dvfs_delay3_4x

		ldr     r3,[r5]             //// 9. enable DRAM 
		orr     r3,r3,#0x1
		str     r3,[r5]


		ldmia   r13!,{r4-r7,r14}
		bx      r14



ENTRY(DVFS_CLKSwitch_2x)  

		stmdb   r13!,{r4-r7,r14}

//	pITCM_Base = OALPAtoVA(0x50000000, FALSE)//
		ldr     r4,=MT3351_DVFS_ITCM_PA

//	pEMI_CONN =  OALPAtoVA(MT3351_EMI_CONN_PA, FALSE)//
		ldr     r5,=MT3351_EMI_CONN_PA

//	pCONFIG_REGS =  OALPAtoVA(MT3351_CON_REGS_PA, FALSE)//
		ldr     r6,=MT3351_CON_REGS_PA

//	pPMU_REGS =  OALPAtoVA(MT3351_PMU_REGS_PA, FALSE)//
		ldr     r7,=MT3351_PMU_REGS_PA

//	copy suspend code to ITCM
		mov     r0,r4					// destination address
		ldr     r1,=clk_2x_switch_start
		mov     r2,#0x300				// copy 300 byte length 
		bl      memcpy
		mov     pc, r4					// set Program counter to ITCM0
		nop

clk_2x_switch_start:
//	*pEMI_CONN |= 0x20//				//// 2. set DRAM into self refresh mode
		ldr     r3,[r5]
		orr     r3,r3,#0x20
		str     r3,[r5]

//									//// 3. count over 32 bus clk cycle
		mov		r0,#0x40
dvfs_delay1_2x:
		sub		r0,r0,#0x1
		cmp     r0,#0x0
		bgt     dvfs_delay1_2x


// 441  : 	s_pConforDVFSRegs->CLKCON &= 0xFFFCFFFF//

        ldr     r3, [r6, #0]        //// 4. switch clock frequency
        mvn     r4, #3, 16
        and     r3, r3, r4
        orr     r3, r3, #1, 16
        str		r3,[r6,#0]
        
// s_pConforDVFSRegs->CLKCON |= ( (ARM_DIV_2 << 16) ) //

		mov		r6,#0x10            //// 5. count over 8 clk cycle
dvfs_delay2_2x:
		sub		r6,r6,#0x1
		cmp     r6,#0x0
		bgt     dvfs_delay2_2x

		ldr     r3,[r5]             //// 6. disable DRAM 
		bic     r3,r3,#0x1
		str     r3,[r5]		

//	*pEMI_CONN &= ~0x20//			//// 7. leave self refresh mode 
		ldr     r3,[r5]
		bic     r3,r3,#0x20
		str     r3,[r5]

		mov		r6,#0xFF            //// 8. count over 200 clk cycle
dvfs_delay3_2x:
		sub		r6,r6,#0x1
		cmp     r6,#0x0
		bgt     dvfs_delay3_2x

		ldr     r3,[r5]             //// 9. enable DRAM 
		orr     r3,r3,#0x1
		str     r3,[r5]


		ldmia   r13!,{r4-r7,r14}
		bx      r14



ENTRY(DVFS_CLKSwitch_1x)  

		stmdb   r13!,{r4-r7,r14}

//	pITCM_Base = OALPAtoVA(0x50000000, FALSE)//
		ldr     r4,=MT3351_DVFS_ITCM_PA

//	pEMI_CONN =  OALPAtoVA(MT3351_EMI_CONN_PA, FALSE)//
		ldr     r5,=MT3351_EMI_CONN_PA

//	pCONFIG_REGS =  OALPAtoVA(MT3351_CON_REGS_PA, FALSE)//
		ldr     r6,=MT3351_CON_REGS_PA

//	pPMU_REGS =  OALPAtoVA(MT3351_PMU_REGS_PA, FALSE)//
		ldr     r7,=MT3351_PMU_REGS_PA

//	copy suspend code to ITCM
		mov     r0,r4					// destination address
		ldr     r1,=clk_1x_switch_start
		mov     r2,#0x300				// copy 300 byte length 
		bl      memcpy
		mov     pc, r4					// set Program counter to ITCM0
		nop

clk_1x_switch_start:
//	*pEMI_CONN |= 0x20//				//// 2. set DRAM into self refresh mode
		ldr     r3,[r5]
		orr     r3,r3,#0x20
		str     r3,[r5]

//									//// 3. count over 32 bus clk cycle
		mov		r0,#0x40
dvfs_delay1_1x:
		sub		r0,r0,#0x1
		cmp     r0,#0x0
		bgt     dvfs_delay1_1x


// 441  : 	s_pConforDVFSRegs->CLKCON &= 0xFFFCFFFF//
// s_pConforDVFSRegs->CLKCON |= ( (ARM_DIV_2 << 16) ) //
        ldr     r3, [r6, #0]        //// 4. switch clock frequency
        mvn     r4, #3, 16
        and     r3, r3, r4
        orr     r3, r3, #3, 16
        str		r3,[r6,#0]
        
		mov		r6,#0xFF            //// 5. count over 8 clk cycle
dvfs_delay2_1x:
		sub		r6,r6,#0x1
		cmp     r6,#0x0
		bgt     dvfs_delay2_1x

		ldr     r3,[r5]             //// 6. disable DRAM 
		bic     r3,r3,#0x1
		str     r3,[r5]

//	*pEMI_CONN &= ~0x20//			//// 7. leave self refresh mode 
		ldr     r3,[r5]
		bic     r3,r3,#0x20
		str     r3,[r5]

		mov		r6,#0xFF            //// 8. count over 200 clk cycle
dvfs_delay3_1x:
		sub		r6,r6,#0x1
		cmp     r6,#0x0
		bgt     dvfs_delay3_1x

		ldr     r3,[r5]             //// 9. enable DRAM 
		orr     r3,r3,#0x1
		str     r3,[r5]


		ldmia   r13!,{r4-r7,r14}
		bx      r14


