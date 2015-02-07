

#undef CONFIG_PARAVIRT
#ifdef CONFIG_X86_32
#define _ASM_X86_DESC_H 1
#endif

#ifdef CONFIG_X86_64
#define _LINUX_STRING_H_ 1
#define __LINUX_BITMAP_H 1
#endif

#include <linux/linkage.h>
#include <linux/screen_info.h>
#include <linux/elf.h>
#include <linux/io.h>
#include <asm/page.h>
#include <asm/boot.h>
#include <asm/bootparam.h>




#define OF(args)	args
#define STATIC		static

#undef memset
#undef memcpy
#define memzero(s, n)	memset((s), 0, (n))

typedef unsigned char	uch;
typedef unsigned short	ush;
typedef unsigned long	ulg;

#define WSIZE		0x80000000

/* Input buffer: */
static unsigned char	*inbuf;

/* Sliding window buffer (and final output buffer): */
static unsigned char	*window;

/* Valid bytes in inbuf: */
static unsigned		insize;

/* Index of next byte to be processed in inbuf: */
static unsigned		inptr;

/* Bytes in output buffer: */
static unsigned		outcnt;

/* gzip flag byte */
#define ASCII_FLAG	0x01 /* bit 0 set: file probably ASCII text */
#define CONTINUATION	0x02 /* bit 1 set: continuation of multi-part gz file */
#define EXTRA_FIELD	0x04 /* bit 2 set: extra field present */
#define ORIG_NAM	0x08 /* bit 3 set: original file name present */
#define COMMENT		0x10 /* bit 4 set: file comment present */
#define ENCRYPTED	0x20 /* bit 5 set: file is encrypted */
#define RESERVED	0xC0 /* bit 6, 7:  reserved */

#define get_byte()	(inptr < insize ? inbuf[inptr++] : fill_inbuf())

/* Diagnostic functions */
#ifdef DEBUG
#  define Assert(cond, msg) do { if (!(cond)) error(msg); } while (0)
#  define Trace(x)	do { fprintf x; } while (0)
#  define Tracev(x)	do { if (verbose) fprintf x ; } while (0)
#  define Tracevv(x)	do { if (verbose > 1) fprintf x ; } while (0)
#  define Tracec(c, x)	do { if (verbose && (c)) fprintf x ; } while (0)
#  define Tracecv(c, x)	do { if (verbose > 1 && (c)) fprintf x ; } while (0)
#else
#  define Assert(cond, msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c, x)
#  define Tracecv(c, x)
#endif

static int  fill_inbuf(void);
static void flush_window(void);
static void error(char *m);

static struct boot_params *real_mode;		/* Pointer to real-mode data */
static int quiet;

extern unsigned char input_data[];
extern int input_len;

static long bytes_out;

static void *memset(void *s, int c, unsigned n);
static void *memcpy(void *dest, const void *src, unsigned n);

static void __putstr(int, const char *);
#define putstr(__x)  __putstr(0, __x)

#ifdef CONFIG_X86_64
#define memptr long
#else
#define memptr unsigned
#endif

static memptr free_mem_ptr;
static memptr free_mem_end_ptr;

static char *vidmem;
static int vidport;
static int lines, cols;

#include "../../../../lib/inflate.c"

static void scroll(void)
{
	int i;

	memcpy(vidmem, vidmem + cols * 2, (lines - 1) * cols * 2);
	for (i = (lines - 1) * cols * 2; i < lines * cols * 2; i += 2)
		vidmem[i] = ' ';
}

static void __putstr(int error, const char *s)
{
	int x, y, pos;
	char c;

#ifndef CONFIG_X86_VERBOSE_BOOTUP
	if (!error)
		return;
#endif

#ifdef CONFIG_X86_32
	if (real_mode->screen_info.orig_video_mode == 0 &&
	    lines == 0 && cols == 0)
		return;
#endif

	x = real_mode->screen_info.orig_x;
	y = real_mode->screen_info.orig_y;

	while ((c = *s++) != '\0') {
		if (c == '\n') {
			x = 0;
			if (++y >= lines) {
				scroll();
				y--;
			}
		} else {
			vidmem[(x + cols * y) * 2] = c;
			if (++x >= cols) {
				x = 0;
				if (++y >= lines) {
					scroll();
					y--;
				}
			}
		}
	}

	real_mode->screen_info.orig_x = x;
	real_mode->screen_info.orig_y = y;

	pos = (x + cols * y) * 2;	/* Update cursor position */
	outb(14, vidport);
	outb(0xff & (pos >> 9), vidport+1);
	outb(15, vidport);
	outb(0xff & (pos >> 1), vidport+1);
}

static void *memset(void *s, int c, unsigned n)
{
	int i;
	char *ss = s;

	for (i = 0; i < n; i++)
		ss[i] = c;
	return s;
}

static void *memcpy(void *dest, const void *src, unsigned n)
{
	int i;
	const char *s = src;
	char *d = dest;

	for (i = 0; i < n; i++)
		d[i] = s[i];
	return dest;
}

static int fill_inbuf(void)
{
	error("ran out of input data");
	return 0;
}

static void flush_window(void)
{
	/* With my window equal to my output buffer
	 * I only need to compute the crc here.
	 */
	unsigned long c = crc;         /* temporary variable */
	unsigned n;
	unsigned char *in, ch;

	in = window;
	for (n = 0; n < outcnt; n++) {
		ch = *in++;
		c = crc_32_tab[((int)c ^ ch) & 0xff] ^ (c >> 8);
	}
	crc = c;
	bytes_out += (unsigned long)outcnt;
	outcnt = 0;
}

static void error(char *x)
{
	__putstr(1, "\n\n");
	__putstr(1, x);
	__putstr(1, "\n\n -- System halted");

	while (1)
		asm("hlt");
}

static void parse_elf(void *output)
{
#ifdef CONFIG_X86_64
	Elf64_Ehdr ehdr;
	Elf64_Phdr *phdrs, *phdr;
#else
	Elf32_Ehdr ehdr;
	Elf32_Phdr *phdrs, *phdr;
#endif
	void *dest;
	int i;

	memcpy(&ehdr, output, sizeof(ehdr));
	if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
	   ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
	   ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
	   ehdr.e_ident[EI_MAG3] != ELFMAG3) {
		error("Kernel is not a valid ELF file");
		return;
	}

	if (!quiet)
		putstr("Parsing ELF... ");

	phdrs = malloc(sizeof(*phdrs) * ehdr.e_phnum);
	if (!phdrs)
		error("Failed to allocate space for phdrs");

	memcpy(phdrs, output + ehdr.e_phoff, sizeof(*phdrs) * ehdr.e_phnum);

	for (i = 0; i < ehdr.e_phnum; i++) {
		phdr = &phdrs[i];

		switch (phdr->p_type) {
		case PT_LOAD:
#ifdef CONFIG_RELOCATABLE
			dest = output;
			dest += (phdr->p_paddr - LOAD_PHYSICAL_ADDR);
#else
			dest = (void *)(phdr->p_paddr);
#endif
			memcpy(dest,
			       output + phdr->p_offset,
			       phdr->p_filesz);
			break;
		default: /* Ignore other PT_* */ break;
		}
	}
}

asmlinkage void decompress_kernel(void *rmode, memptr heap,
				  unsigned char *input_data,
				  unsigned long input_len,
				  unsigned char *output)
{
	real_mode = rmode;

	if (real_mode->hdr.loadflags & QUIET_FLAG)
		quiet = 1;

	if (real_mode->screen_info.orig_video_mode == 7) {
		vidmem = (char *) 0xb0000;
		vidport = 0x3b4;
	} else {
		vidmem = (char *) 0xb8000;
		vidport = 0x3d4;
	}

	lines = real_mode->screen_info.orig_video_lines;
	cols = real_mode->screen_info.orig_video_cols;

	window = output;		/* Output buffer (Normally at 1M) */
	free_mem_ptr     = heap;	/* Heap */
	free_mem_end_ptr = heap + BOOT_HEAP_SIZE;
	inbuf  = input_data;		/* Input buffer */
	insize = input_len;
	inptr  = 0;

#ifdef CONFIG_X86_64
	if ((unsigned long)output & (__KERNEL_ALIGN - 1))
		error("Destination address not 2M aligned");
	if ((unsigned long)output >= 0xffffffffffUL)
		error("Destination address too large");
#else
	if ((u32)output & (CONFIG_PHYSICAL_ALIGN - 1))
		error("Destination address not CONFIG_PHYSICAL_ALIGN aligned");
	if (heap > ((-__PAGE_OFFSET-(512<<20)-1) & 0x7fffffff))
		error("Destination address too large");
#ifndef CONFIG_RELOCATABLE
	if ((u32)output != LOAD_PHYSICAL_ADDR)
		error("Wrong destination address");
#endif
#endif

	makecrc();
	if (!quiet)
		putstr("\nDecompressing Linux... ");
	gunzip();
	parse_elf(output);
	if (!quiet)
		putstr("done.\nBooting the kernel.\n");
	return;
}
