asflags-$(CONFIG_FULLDEBUG) := -DDEBUGGER_COMPATIBLE_CACHE=1

obj-$(CONFIG_COLDFIRE)	+= dma.o entry.o vectors.o
obj-$(CONFIG_M5206)	+= timers.o
obj-$(CONFIG_M5206e)	+= timers.o
obj-$(CONFIG_M520x)	+= pit.o
obj-$(CONFIG_M523x)	+= pit.o dma_timer.o
obj-$(CONFIG_M5249)	+= timers.o
obj-$(CONFIG_M527x)	+= pit.o
obj-$(CONFIG_M5272)	+= timers.o
obj-$(CONFIG_M528x)	+= pit.o
obj-$(CONFIG_M5307)	+= timers.o
obj-$(CONFIG_M532x)	+= timers.o
obj-$(CONFIG_M5407)	+= timers.o

extra-y := head.o
