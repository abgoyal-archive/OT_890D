ccflags-y := -Iinclude/drm
radeon-y := radeon_drv.o radeon_cp.o radeon_state.o radeon_mem.o radeon_irq.o r300_cmdbuf.o

radeon-$(CONFIG_COMPAT) += radeon_ioc32.o

obj-$(CONFIG_DRM_RADEON)+= radeon.o
