
#ifndef OP_MODEL_ARM11_CORE_H
#define OP_MODEL_ARM11_CORE_H

#define PMCR_E		(1 << 0)	/* Enable */
#define PMCR_P		(1 << 1)	/* Count reset */
#define PMCR_C		(1 << 2)	/* Cycle counter reset */
#define PMCR_D		(1 << 3)	/* Cycle counter counts every 64th cpu cycle */
#define PMCR_IEN_PMN0	(1 << 4)	/* Interrupt enable count reg 0 */
#define PMCR_IEN_PMN1	(1 << 5)	/* Interrupt enable count reg 1 */
#define PMCR_IEN_CCNT	(1 << 6)	/* Interrupt enable cycle counter */
#define PMCR_OFL_PMN0	(1 << 8)	/* Count reg 0 overflow */
#define PMCR_OFL_PMN1	(1 << 9)	/* Count reg 1 overflow */
#define PMCR_OFL_CCNT	(1 << 10)	/* Cycle counter overflow */

#define PMN0 0
#define PMN1 1
#define CCNT 2

#define CPU_COUNTER(cpu, counter)	((cpu) * 3 + (counter))

int arm11_setup_pmu(void);
int arm11_start_pmu(void);
int arm11_stop_pmu(void);
int arm11_request_interrupts(int *, int);
void arm11_release_interrupts(int *, int);

#endif
