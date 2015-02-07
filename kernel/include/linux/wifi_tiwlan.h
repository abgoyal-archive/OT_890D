
#ifndef _LINUX_WIFI_TIWLAN_H_
#define _LINUX_WIFI_TIWLAN_H_

#define WMPA_NUMBER_OF_SECTIONS	3
#define WMPA_NUMBER_OF_BUFFERS	160
#define WMPA_SECTION_HEADER	24
#define WMPA_SECTION_SIZE_0	(WMPA_NUMBER_OF_BUFFERS * 64)
#define WMPA_SECTION_SIZE_1	(WMPA_NUMBER_OF_BUFFERS * 256)
#define WMPA_SECTION_SIZE_2	(WMPA_NUMBER_OF_BUFFERS * 2048)

struct wifi_platform_data {
        int (*set_power)(int val);
        int (*set_reset)(int val);
        int (*set_carddetect)(int val);
	void *(*mem_prealloc)(int section, unsigned long size);
};

#endif
