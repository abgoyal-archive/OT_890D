obj-$(CONFIG_FB_MT6516) += lcd_drv.o                \
                           dpi_drv.o                \
                           disp_drv_dbi.o           \
                           disp_drv_dpi.o           \
                           disp_drv.o               \
                           mtkfb_console.o          \
                           disp_assert_layer.o      \
                           mtkfb.o                  \
                           debug.o                  \
                           ../console/font_8x16.o

# ifndef CONFIG_FONT_8x16
# obj-$(CONFIG_FB_MT6516) += ../console/font_8x16.o
# else ifeq ($(CONFIG_FONT_8x16),n)
# obj-$(CONFIG_FB_MT6516) += ../console/font_8x16.o
# endif

ccflags-y += -Idrivers/video/mtk
#KBUILD_CFLAGS += -O0

# EOF