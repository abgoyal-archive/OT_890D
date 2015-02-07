
#ifndef OP_MODEL_V7_H
#define OP_MODEL_V7_H

#define PMNC_E		(1 << 0)	/* Enable all counters */
#define PMNC_P		(1 << 1)	/* Reset all counters */
#define PMNC_C		(1 << 2)	/* Cycle counter reset */
#define PMNC_D		(1 << 3)	/* CCNT counts every 64th cpu cycle */
#define PMNC_X		(1 << 4)	/* Export to ETM */
#define PMNC_DP		(1 << 5)	/* Disable CCNT if non-invasive debug*/
#define	PMNC_MASK	0x3f		/* Mask for writable bits */

#define CCNT 		0
#define CNT0 		1
#define CNT1 		2
#define CNT2 		3
#define CNT3 		4
#define CNTMAX 		5

#define CPU_COUNTER(cpu, counter)	((cpu) * CNTMAX + (counter))

#define CNTENS_P0	(1 << 0)
#define CNTENS_P1	(1 << 1)
#define CNTENS_P2	(1 << 2)
#define CNTENS_P3	(1 << 3)
#define CNTENS_C	(1 << 31)
#define	CNTENS_MASK	0x8000000f	/* Mask for writable bits */

#define CNTENC_P0	(1 << 0)
#define CNTENC_P1	(1 << 1)
#define CNTENC_P2	(1 << 2)
#define CNTENC_P3	(1 << 3)
#define CNTENC_C	(1 << 31)
#define	CNTENC_MASK	0x8000000f	/* Mask for writable bits */

#define INTENS_P0	(1 << 0)
#define INTENS_P1	(1 << 1)
#define INTENS_P2	(1 << 2)
#define INTENS_P3	(1 << 3)
#define INTENS_C	(1 << 31)
#define	INTENS_MASK	0x8000000f	/* Mask for writable bits */

#define	EVTSEL_MASK	0x7f		/* Mask for writable bits */

#define	SELECT_MASK	0x1f		/* Mask for writable bits */

#define FLAG_P0		(1 << 0)
#define FLAG_P1		(1 << 1)
#define FLAG_P2		(1 << 2)
#define FLAG_P3		(1 << 3)
#define FLAG_C		(1 << 31)
#define	FLAG_MASK	0x8000000f	/* Mask for writable bits */


int armv7_setup_pmu(void);
int armv7_start_pmu(void);
int armv7_stop_pmu(void);
int armv7_request_interrupts(int *, int);
void armv7_release_interrupts(int *, int);

#endif
