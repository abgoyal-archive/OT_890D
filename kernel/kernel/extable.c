
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ftrace.h>
#include <asm/uaccess.h>
#include <asm/sections.h>

extern struct exception_table_entry __start___ex_table[];
extern struct exception_table_entry __stop___ex_table[];

/* Sort the kernel's built-in exception table */
void __init sort_main_extable(void)
{
	sort_extable(__start___ex_table, __stop___ex_table);
}

/* Given an address, look for it in the exception tables. */
const struct exception_table_entry *search_exception_tables(unsigned long addr)
{
	const struct exception_table_entry *e;

	e = search_extable(__start___ex_table, __stop___ex_table-1, addr);
	if (!e)
		e = search_module_extables(addr);
	return e;
}

__notrace_funcgraph int core_kernel_text(unsigned long addr)
{
	if (addr >= (unsigned long)_stext &&
	    addr <= (unsigned long)_etext)
		return 1;

	if (system_state == SYSTEM_BOOTING &&
	    addr >= (unsigned long)_sinittext &&
	    addr <= (unsigned long)_einittext)
		return 1;
	return 0;
}

__notrace_funcgraph int __kernel_text_address(unsigned long addr)
{
	if (core_kernel_text(addr))
		return 1;
	return __module_text_address(addr) != NULL;
}

int kernel_text_address(unsigned long addr)
{
	if (core_kernel_text(addr))
		return 1;
	return module_text_address(addr) != NULL;
}

int func_ptr_is_kernel_text(void *ptr)
{
	unsigned long addr;
	addr = (unsigned long) dereference_function_descriptor(ptr);
	if (core_kernel_text(addr))
		return 1;
	return module_text_address(addr) != NULL;
}
