

#ifndef __ARCH_ARM_UNCOMPRESS_H
#define __ARCH_ARM_UNCOMPRESS_H

//#define MT3351_UART0_DR  (*(volatile unsigned char *)0x2000c000)
//#define MT3351_UART0_ST  (*(volatile unsigned int *) 0x2000c04C)

#define MT3351_UART0_LSR (*(volatile unsigned char *)0x80025014)
#define MT3351_UART0_THR (*(volatile unsigned char *)0x80025000)


/*Koshi: Marked for porting */

#if 0
 
static void putstr(const char *s)
{
    while (*s) {
        /* wait for empty slot */
        while ((MT3351_UART0_ST & 0x1f00) == 0);

        MT3351_UART0_DR = *s;

        if (*s == '\n') {
            while ((MT3351_UART0_ST & 0x1f00) == 0);

            MT3351_UART0_DR = '\r';
        }
        s++;
    }
    //while (AMBA_UART_FR & (1 << 3));
}

#endif

#define MT3351_UART0_LCR (*(volatile unsigned char *)0x8002500c)
#define MT3351_UART0_DLL (*(volatile unsigned char *)0x80025000)
#define MT3351_UART0_DLH (*(volatile unsigned char *)0x80025004)
#define MT3351_UART0_FCR (*(volatile unsigned char *)0x80025008)
#define MT3351_UART0_MCR (*(volatile unsigned char *)0x80025010)

#define MT3351_UART0_PWR (*(volatile unsigned char *)0x80001340)

static void decomp_setup(void)
{
	u32 tmp = 1 << 6;
	
	MT3351_UART0_PWR = tmp; /* power on */

	MT3351_UART0_LCR = 0x3;
	tmp = MT3351_UART0_LCR;
	MT3351_UART0_LCR = (tmp | 0x80);
	MT3351_UART0_DLL = 16;
	MT3351_UART0_DLH = 0;
	MT3351_UART0_LCR = tmp;
	MT3351_UART0_FCR = 0x0047;
	MT3351_UART0_MCR = (0x1 | 0x2);
	MT3351_UART0_FCR = (0x10 | 0x80 | 0x07);
}

static inline void putc(char c)
{
    while (!(MT3351_UART0_LSR & 0x20));    
    MT3351_UART0_THR = c;        
}

static inline void flush(void)
{
}

#define arch_decomp_setup() decomp_setup()

#define arch_decomp_wdog()

#endif /* __ARCH_ARM_UNCOMPRESS_H */

