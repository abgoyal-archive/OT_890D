
 /* linux/include/asm-arm/arch-mt6516/mt6516_devs.h
  *
  * Devices supported on MT6516 machine
  *
  * Copyright (C) 2008,2009 MediaTek <www.mediatek.com>
  * Authors: Infinity Chen <infinity.chen@mediatek.com>  
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */

#ifndef _MT6516_BOOT_H_
#define _MT6516_BOOT_H_

/* MT6516 boot type definitions */
typedef enum 
{
    NORMAL_BOOT = 0,
    META_BOOT = 1,
    RECOVERY_BOOT = 2,    
    SW_REBOOT = 3,
    FACTORY_BOOT = 4,
    UNKNOWN_BOOT
} BOOTMODE;

#define BOOT_DEV_NAME 	   	   "BOOT"
#define BOOT_SYSFS 	   		   "boot"
#define BOOT_SYSFS_ATTR        "boot_mode"
#define MD_SYSFS_ATTR        "md"

/* this vairable will be set by mt6516_fixup */
extern BOOTMODE g_boot_mode;

extern BOOTMODE get_boot_mode(void);
extern bool is_meta_mode(void);

extern void boot_register_md_func(ssize_t (*show)(char*), ssize_t (*store)(const char*,size_t));

#endif 

