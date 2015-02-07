include/asm-sh/machtypes.h: $(src)/gen-mach-types $(src)/mach-types
	@echo '  Generating $@'
	$(Q)if [ ! -d include/asm-sh ]; then mkdir -p include/asm-sh; fi
	$(Q)$(AWK) -f $^ > $@ || { rm -f $@; /bin/false; }
