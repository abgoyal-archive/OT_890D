


#include "boot.h"

#define MAX_8042_LOOPS	100000

static int empty_8042(void)
{
	u8 status;
	int loops = MAX_8042_LOOPS;

	while (loops--) {
		io_delay();

		status = inb(0x64);
		if (status & 1) {
			/* Read and discard input data */
			io_delay();
			(void)inb(0x60);
		} else if (!(status & 2)) {
			/* Buffers empty, finished! */
			return 0;
		}
	}

	return -1;
}


#define A20_TEST_ADDR	(4*0x80)
#define A20_TEST_SHORT  32
#define A20_TEST_LONG	2097152	/* 2^21 */

static int a20_test(int loops)
{
	int ok = 0;
	int saved, ctr;

	set_fs(0x0000);
	set_gs(0xffff);

	saved = ctr = rdfs32(A20_TEST_ADDR);

	while (loops--) {
		wrfs32(++ctr, A20_TEST_ADDR);
		io_delay();	/* Serialize and make delay constant */
		ok = rdgs32(A20_TEST_ADDR+0x10) ^ ctr;
		if (ok)
			break;
	}

	wrfs32(saved, A20_TEST_ADDR);
	return ok;
}

/* Quick test to see if A20 is already enabled */
static int a20_test_short(void)
{
	return a20_test(A20_TEST_SHORT);
}

static int a20_test_long(void)
{
	return a20_test(A20_TEST_LONG);
}

static void enable_a20_bios(void)
{
	asm volatile("pushfl; int $0x15; popfl"
		     : : "a" ((u16)0x2401));
}

static void enable_a20_kbc(void)
{
	empty_8042();

	outb(0xd1, 0x64);	/* Command write */
	empty_8042();

	outb(0xdf, 0x60);	/* A20 on */
	empty_8042();

	outb(0xff, 0x64);	/* Null command, but UHCI wants it */
	empty_8042();
}

static void enable_a20_fast(void)
{
	u8 port_a;

	port_a = inb(0x92);	/* Configuration port A */
	port_a |=  0x02;	/* Enable A20 */
	port_a &= ~0x01;	/* Do not reset machine */
	outb(port_a, 0x92);
}


#define A20_ENABLE_LOOPS 255	/* Number of times to try */

int enable_a20(void)
{
#if defined(CONFIG_X86_ELAN)
	/* Elan croaks if we try to touch the KBC */
	enable_a20_fast();
	while (!a20_test_long())
		;
	return 0;
#elif defined(CONFIG_X86_VOYAGER)
	/* On Voyager, a20_test() is unsafe? */
	enable_a20_kbc();
	return 0;
#else
       int loops = A20_ENABLE_LOOPS;
	while (loops--) {
		/* First, check to see if A20 is already enabled
		   (legacy free, etc.) */
		if (a20_test_short())
			return 0;

		/* Next, try the BIOS (INT 0x15, AX=0x2401) */
		enable_a20_bios();
		if (a20_test_short())
			return 0;

		/* Try enabling A20 through the keyboard controller */
		empty_8042();
		if (a20_test_short())
			return 0; /* BIOS worked, but with delayed reaction */

		enable_a20_kbc();
		if (a20_test_long())
			return 0;

		/* Finally, try enabling the "fast A20 gate" */
		enable_a20_fast();
		if (a20_test_long())
			return 0;
	}

	return -1;
#endif
}
