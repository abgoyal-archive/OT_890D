

#ifndef __SIGP__
#define __SIGP__

#include <asm/ptrace.h>
#include <asm/atomic.h>

/* get real cpu address from logical cpu number */
extern volatile int __cpu_logical_map[];

typedef enum
{
	sigp_unassigned=0x0,
	sigp_sense,
	sigp_external_call,
	sigp_emergency_signal,
	sigp_start,
	sigp_stop,
	sigp_restart,
	sigp_unassigned1,
	sigp_unassigned2,
	sigp_stop_and_store_status,
	sigp_unassigned3,
	sigp_initial_cpu_reset,
	sigp_cpu_reset,
	sigp_set_prefix,
	sigp_store_status_at_address,
	sigp_store_extended_status_at_address
} sigp_order_code;

typedef __u32 sigp_status_word;

typedef enum
{
        sigp_order_code_accepted=0,
	sigp_status_stored,
	sigp_busy,
	sigp_not_operational
} sigp_ccode;



/* 'Bit' signals, asynchronous */
typedef enum
{
	ec_schedule=0,
	ec_call_function,
	ec_call_function_single,
	ec_bit_last
} ec_bit_sig;

static inline sigp_ccode
signal_processor(__u16 cpu_addr, sigp_order_code order_code)
{
	register unsigned long reg1 asm ("1") = 0;
	sigp_ccode ccode;

	asm volatile(
		"	sigp	%1,%2,0(%3)\n"
		"	ipm	%0\n"
		"	srl	%0,28\n"
		:	"=d"	(ccode)
		: "d" (reg1), "d" (__cpu_logical_map[cpu_addr]),
		  "a" (order_code) : "cc" , "memory");
	return ccode;
}

static inline sigp_ccode
signal_processor_p(__u32 parameter, __u16 cpu_addr, sigp_order_code order_code)
{
	register unsigned int reg1 asm ("1") = parameter;
	sigp_ccode ccode;

	asm volatile(
		"	sigp	%1,%2,0(%3)\n"
		"	ipm	%0\n"
		"	srl	%0,28\n"
		: "=d" (ccode)
		: "d" (reg1), "d" (__cpu_logical_map[cpu_addr]),
		  "a" (order_code) : "cc" , "memory");
	return ccode;
}

static inline sigp_ccode
signal_processor_ps(__u32 *statusptr, __u32 parameter, __u16 cpu_addr,
		    sigp_order_code order_code)
{
	register unsigned int reg1 asm ("1") = parameter;
	sigp_ccode ccode;

	asm volatile(
		"	sigp	%1,%2,0(%3)\n"
		"	ipm	%0\n"
		"	srl	%0,28\n"
		: "=d" (ccode), "+d" (reg1)
		: "d" (__cpu_logical_map[cpu_addr]), "a" (order_code)
		: "cc" , "memory");
	*statusptr = reg1;
	return ccode;
}

#endif /* __SIGP__ */
