
#include <asm/voyager.h>
#include <asm/setup.h>
#define VOYAGER_BIOS_INFO ((struct voyager_bios_info *) \
			(&boot_params.apm_bios_info))

/* Hook to call BIOS initialisation function */


#define ARCH_SETUP	voyager_detect(VOYAGER_BIOS_INFO);

