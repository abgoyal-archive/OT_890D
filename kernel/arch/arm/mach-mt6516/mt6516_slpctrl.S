

#include <linux/autoconf.h>
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/hardware.h>

#define Mode_USR    0x10
#define Mode_FIQ    0x11
#define Mode_IRQ    0x12
#define Mode_SVC    0x13
#define Mode_ABT    0x17
#define Mode_UNDEF  0x1B
#define Mode_SYS    0x1F

#define MODE_MASK1    0x1F
#define I_Bit       0x80
#define F_Bit       0x40
#define C9_ENABLE   0x1



#define MASK_MMU_CTL  (0x3FFF0000)
#define MASK_MMU_TTB  (0x00003FFF)
#define TCM_START_PA  (0xF0400000)

	.text

ENTRY(MT6516_CPUIdle)
    // CPU enter IDLE state
    mov r3, #0
    // AHB
    cmp r0, #0
    beq 10f
    // Gate AHB just before CPU enter IDLE state
    ldrh r2, [r1]
    orr r2, r2, #0x2
    strh r2, [r1] 
10:
    mcr p15, 0, r3, c7, c0, 4
    mov pc, lr


ENTRY(MT6516_EnableITCM)
    mrc p15,0,r1,c9,c1,1
    orr r0, r0,#C9_ENABLE 
    orr r0, r0, r1
    mcr p15,0,r0,c9,c1,1 
    mov pc, lr


ENTRY(MT6516_CPUSuspend)
    	stmfd sp!, {r4-r12, lr} 

    // r0 = MT6516_CPUSuspend physical address,  
    // r1 = TCM address
	    mov r4, r0    
	    mov r9, r1 

    // Set SVC mode
    	mrs r0, cpsr
    	bic r0, r0, #MODE_MASK1
    	orr r1, r0, #Mode_SVC
    // Set I/F bit, disable IRQ and FIQ
    	orr r1, r1, #I_Bit|F_Bit
    // Update CPSR
    	msr cpsr_cxsf, r1 

    // calculate the physical address of instruction after disable MMU
    	ldr r0, =PhysicalPart 
    	ldr r1, =MT6516_CPUSuspend 
    	sub r0, r0, r1 
    	mov r1, r4
    // Now r0 is the physical address of PhysicalPart
    	add r0, r0, r1

    // Read control register c1, put at r6
    	mrc p15, 0, r6, c1, c0, 0 
    // mask unused bits, 0x3FFF0000
    	ldr r1, =MASK_MMU_CTL 
    	bic r6, r6, r1

    // Read translation table base register c2, put at r7    
    	mrc p15, 0, r7, c2, c0, 0 
    // mask unused bits, 0x00003FFF
    	ldr r1, =MASK_MMU_TTB 
    	bic r7, r7, r1

    // Read Domain Access Control register c2, put at r8    
    	mrc p15, 0, r8, c3, c0, 0 

10:   
    // Clean and invalid DCache
    	mrc p15, 0, r15, c7, c14, 3 
    	bne 10b

    // Clean write buffer       
    	mov r1, #0
    	mcr p15, 0, r1, c7, c10, 4

    // Invalidate Entire I-Cache        
    	mov r1, #0
    	mcr p15, 0, r1, c7, c5, 0
	// Read Control register  MMU
    	mrc p15, 0, r1, c1, c0, 0        
    // 0x1000 : Disable I-Cache
    	bic r1, r1, #0x1000
    // 0x8 : Reserved by ARM 9 TRM
    	bic r1, r1, #0x8
    // 0x4 : Disable D-Cache
    	bic r1, r1, #0x4
    // 0x2 : Disable Alignment fault checking 
    	bic r1, r1, #0x2
    // 0x1 : Disable MMU    
    	//bic r1, r1, #0x1        
    	cmp r0, #0x0        
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Test jump to physical address @@@@@@@@@@@@@@@@@@@@@@
    	mcr p15, 0, r1, c1, c0, 0        
    // Jump to PhysicalPart
	    // mov pc, r0
	    nop
	    nop
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    
    // The following code will run in physical address, only physical address is available. 
PhysicalPart:
	    nop
	    nop
        
    // Flush the TLB
	    mov r0, #0 
	    mcr p15, 0, r0, c8, c7, 0
    
    // Power down Cache and MMU, MCU_MEM_PDN
	    ldr r0, =0xF0001308
	    ldr r1, [r0]
	    // ldr r1, =0xFFFFFFFF
	    orr r1, r1, #0x0F
    	str r1, [r0]
    
    
    // STEP1: Set AP SLEEP (IRQ CODE: 0x36) to level sensitive on CIRQ. 
    // already done when system start.
    
    // STEP2: Unmask AP SLEEP CTRL interrupt.
    // already done at mt6516_pm_Maskinterrupt.
    
    // STEP3: EOI AP SLEEP interrupt.
    // already done at mt6516_pm_Maskinterrupt.
        
    // STEP4: Read clear AP_SM_STA (OxF003C21C). 
    // already done at mt6516_pm_Maskinterrupt.
        
    // STEP5: Set AP_SM_PAUSE_M(0x8003C200) and AP_SM_PAUSE_L(0x8003C204) for sleep duration. 16 seconds as default
    ldr r1, =0xF003C200
    ldr r2, =0x7 
    ldr r3, =0xFFFF
    str r2, [r1]
    str r3, [r1, #0x4] 
    
    // STEP6: Set AP_SM_CLK_SETTLE(0xF003C208) to 0x64. Must over 3ms
    ldr r3, =0x64
    str r3, [r1, #0x8] 
    
    // STEP7: Set AP_SM_CNF(DxF003C22C) to wanted wake-up source. (TP, GPT, MSDC, RTC, EINT, KP or SM) 
    // already done at mt6516_pm_SuspendEnter    

    // STEP8: Set AP_SM_CON[1]:PAUSE_START to 1 to enable AP sleep controller. 
    ldr r1, =0xF003C218
    mov r2, #0x2 
    str r2, [r1]
    
    // STEP9: Execute the CP15 command(MCR p15, 0, r0, c7, c0, 4), 
    // then ARM9 MCU enters low power state 
    // and STANDBYWFI signal becomes HIGH. CLOCK_OFF signal is issued to Clock Management Unit, 
    // and then AP MCU Sub-system clock is gated and VCXO OFF signal is issued to AP Sleep Controller.
    mov r0, #0
    mcr p15, 0, r0,c7,c0,4

    // wait till interrupt occurs
    // polling AP_SM_STA
    mov r2, #0
    mov r3, #0x10
25:      
    //ldr r0, =0xF003c21c
    //ldr r1, [r0]
// test
    //cmp r1, #0xC0
    //beq 15f
    //cmp r1, #0xA0
    //beq 15f
    //add r2, r2, #1
    //cmp r2, r3
    //beq 15f
    //b 25b

15:    
    //mov r10, r1
        

    // Power up Cache and MMU
    ldr r0, =0xF0001308
    ldr r1, [r0]
    bic r1, r1, #0x0F
    // ldr r1, =0x0    
    str r1, [r0]

    //delay 1 us for 416M, and about 4 us for 104M
    mov r0, #0x1A0
35:      
    sub r0, r0, #0x1
    cmp r0, #0x0
    bne 35b
        
    // Clean and invalid DCache
    // mrc p15, 0, r15, c7, c14, 3 
    // bne 10b
    mov r1, #0 
    mcr p15, 0, r1, c7, c6, 0

    // Invalidate instruction cache
    mov r1, #0 
    mcr p15, 0, r1, c7, c5, 0
    ldr r1, VirtualPart
    ldr r2, =MT6516_CPUSuspend
    sub r1, r1, r2

    // TCM_START_UA saved in r9
    mov r2, r9 
    add r1, r1, r2

    // make sure no stall on ¡§mov pc,r0¡¨ below
    cmp r1, #0 
    
    // restore MMU
    mov r4, #0

    // access domain 0
    mcr p15, 0, r8, c3, c0, 0 
    mov r4, #1
    nop
    nop

    // TTB    
    mcr p15, 0, r7, c2, c0, 0 
    mov r4, #2
    nop
    mov r0, #0

    // flush TLBs    
    mcr p15, 0, r0, c8, c7, 0 
    nop
    nop

    // Turn on MMU

//test
    mcr p15, 0, r6, c1, c0, 0
    //mov pc, r1
    nop
    nop

VirtualPart:

    nop
    nop

    mov r0, r10
    ldmia sp!, {r4-r12, lr}

    mov pc, lr
    nop
    
    
    
    

        



