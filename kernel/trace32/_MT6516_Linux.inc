;
; Script for loading Linux kernel image for RealView Debugger
;
; 1. You must change the linux kernel paths(3 paths) to where you are placing the linux kernel source tree.
; 2. If the ramdisk size is changed, update the Ramdisk size pls
; 3. If the phys_offset is changed, change the value with pointed with "<==xxx" pls
  
; Turn off MMU
setreg @CP15_CONTROL=0x0005107E

; Load vmlinux to 0x8000
load/r 'y:\work\Android\kernel\vmlinux' &-0xbfa00000 //<==0xc0000000-phys_offset

; clear boot parameters
fill /32 0x600100..0x600800=0 //<== 0x100..0x800+phys_offset

; boot parameters
ce ((unsigned long *)0x00600100)[0]=0x2
ce ((unsigned long *)0x00600104)[0]=0x54410001


; First memory bank
ce ((unsigned long *)0x00600108)[0]=0x4
ce ((unsigned long *)0x0060010c)[0]=0x54410002
; RAM size
ce ((unsigned long *)0x00600110)[0]=0x07A00000//<==0x08000000-phys_offset
; Start address
ce ((unsigned long *)0x00600114)[0]=0x00600000//<==0x0+phys_offset

;Second memory bank
; You must make sure the MAX_MEM_SIZE_SUPPORT in kernel is larger than the total
; memory size of all banks if you want it can be work.
ce ((unsigned long *)0x00600108)[0]=0x4
ce ((unsigned long *)0x0060010c)[0]=0x54410002
; RAM size
ce ((unsigned long *)0x00600110)[0]=0x08000000//<== Size of second bank 
; Start address
ce ((unsigned long *)0x00600114)[0]=0x10000000//<== Start address of second bank

ce ((unsigned long *)0x00600118)[0]=0x4
ce ((unsigned long *)0x0060011c)[0]=0x54420005
; Start address of RAM disk
ce ((unsigned long *)0x00600120)[0]=0x00E00000//<==0x800000+phys_offset
; Size of RAM disk
ce ((unsigned long *)0x00600124)[0]=2862376//<==Ramdisk size
ce ((unsigned long *)0x00600128)[0]=0x0
ce ((unsigned long *)0x0060012c)[0]=0x0

; Reset registers
setreg @R0=0x00000000
setreg @R1=0x00000000
setreg @R2=0x00000000
setreg @R4=0x00000000
setreg @R5=0x00000000
setreg @R6=0x00000000
setreg @R7=0x00000000
setreg @R11=0x00000000
setreg @R12=0x00000000
setreg @R13=0x00000000

; Set program counter
setreg @R15=0x00608000 //<==0x8000+phys_offset
; Machine id
setreg @R1=0x00001974 
; Start address of boot arguments
setreg @R2=0x00600100 //<==0x100+phys_offset

; Load RAM disk
readfile,raw,nowarn 'y:\work\Android\kernel\trace32\rd6516-devel.gz'=0x00E00000 //<==0x800000+phys_offset

; Load vmlinux symbol
load/ni/r/np 'y:\work\Android\kernel\vmlinux'

; Run
go

