

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/openprom.h>
#include <asm/oplib.h>
#include <asm/system.h>
#include <linux/string.h>

extern int prom_stdin, prom_stdout;

inline int
prom_nbgetchar(void)
{
	char inc;

	if (p1275_cmd("read", P1275_ARG(1,P1275_ARG_OUT_BUF)|
			      P1275_INOUT(3,1),
			      prom_stdin, &inc, P1275_SIZE(1)) == 1)
		return inc;
	else
		return -1;
}

inline int
prom_nbputchar(char c)
{
	char outc;
	
	outc = c;
	if (p1275_cmd("write", P1275_ARG(1,P1275_ARG_IN_BUF)|
			       P1275_INOUT(3,1),
			       prom_stdout, &outc, P1275_SIZE(1)) == 1)
		return 0;
	else
		return -1;
}

/* Blocking version of get character routine above. */
char
prom_getchar(void)
{
	int character;
	while((character = prom_nbgetchar()) == -1) ;
	return (char) character;
}

/* Blocking version of put character routine above. */
void
prom_putchar(char c)
{
	prom_nbputchar(c);
	return;
}

void
prom_puts(const char *s, int len)
{
	p1275_cmd("write", P1275_ARG(1,P1275_ARG_IN_BUF)|
			   P1275_INOUT(3,1),
			   prom_stdout, s, P1275_SIZE(len));
}
