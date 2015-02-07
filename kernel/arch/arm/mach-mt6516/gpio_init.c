
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_gpio.h>
#include <cust_gpio_boot.h>
#include <asm/io.h>
/*----------------------------------------------------------------------------*/
#define GIO_PFX "[GPIO] "
#define GIO_DBG(fmt, arg...)    printk(KERN_INFO GIO_PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define GIO_ERR(fmt, arg...)    printk(KERN_ERR GIO_PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define GIO_VER                 printk
/*----------------------------------------------------------------------------*/
u16 gpio_init_dir_data[] = {
    ((GPIO0_DIR       <<  0) |(GPIO1_DIR       <<  1) |(GPIO2_DIR       <<  2) |(GPIO3_DIR       <<  3) |
     (GPIO4_DIR       <<  4) |(GPIO5_DIR       <<  5) |(GPIO6_DIR       <<  6) |(GPIO7_DIR       <<  7) |
     (GPIO8_DIR       <<  8) |(GPIO9_DIR       <<  9) |(GPIO10_DIR      << 10) |(GPIO11_DIR      << 11) |
     (GPIO12_DIR      << 12) |(GPIO13_DIR      << 13) |(GPIO14_DIR      << 14) |(GPIO15_DIR      << 15)),

    ((GPIO16_DIR      <<  0) |(GPIO17_DIR      <<  1) |(GPIO18_DIR      <<  2) |(GPIO19_DIR      <<  3) |
     (GPIO20_DIR      <<  4) |(GPIO21_DIR      <<  5) |(GPIO22_DIR      <<  6) |(GPIO23_DIR      <<  7) |
     (GPIO24_DIR      <<  8) |(GPIO25_DIR      <<  9) |(GPIO26_DIR      << 10) |(GPIO27_DIR      << 11) |
     (GPIO28_DIR      << 12) |(GPIO29_DIR      << 13) |(GPIO30_DIR      << 14) |(GPIO31_DIR      << 15)),

    ((GPIO32_DIR      <<  0) |(GPIO33_DIR      <<  1) |(GPIO34_DIR      <<  2) |(GPIO35_DIR      <<  3) |
     (GPIO36_DIR      <<  4) |(GPIO37_DIR      <<  5) |(GPIO38_DIR      <<  6) |(GPIO39_DIR      <<  7) |
     (GPIO40_DIR      <<  8) |(GPIO41_DIR      <<  9) |(GPIO42_DIR      << 10) |(GPIO43_DIR      << 11) |
     (GPIO44_DIR      << 12) |(GPIO45_DIR      << 13) |(GPIO46_DIR      << 14) |(GPIO47_DIR      << 15)),

    ((GPIO48_DIR      <<  0) |(GPIO49_DIR      <<  1) |(GPIO50_DIR      <<  2) |(GPIO51_DIR      <<  3) |
     (GPIO52_DIR      <<  4) |(GPIO53_DIR      <<  5) |(GPIO54_DIR      <<  6) |(GPIO55_DIR      <<  7) |
     (GPIO56_DIR      <<  8) |(GPIO57_DIR      <<  9) |(GPIO58_DIR      << 10) |(GPIO59_DIR      << 11) |
     (GPIO60_DIR      << 12) |(GPIO61_DIR      << 13) |(GPIO62_DIR      << 14) |(GPIO63_DIR      << 15)),

    ((GPIO64_DIR      <<  0) |(GPIO65_DIR      <<  1) |(GPIO66_DIR      <<  2) |(GPIO67_DIR      <<  3) |
     (GPIO68_DIR      <<  4) |(GPIO69_DIR      <<  5) |(GPIO70_DIR      <<  6) |(GPIO71_DIR      <<  7) |
     (GPIO72_DIR      <<  8) |(GPIO73_DIR      <<  9) |(GPIO74_DIR      << 10) |(GPIO75_DIR      << 11) |
     (GPIO76_DIR      << 12) |(GPIO77_DIR      << 13) |(GPIO78_DIR      << 14) |(GPIO79_DIR      << 15)),

    ((GPIO80_DIR      <<  0) |(GPIO81_DIR      <<  1) |(GPIO82_DIR      <<  2) |(GPIO83_DIR      <<  3) |
     (GPIO84_DIR      <<  4) |(GPIO85_DIR      <<  5) |(GPIO86_DIR      <<  6) |(GPIO87_DIR      <<  7) |
     (GPIO88_DIR      <<  8) |(GPIO89_DIR      <<  9) |(GPIO90_DIR      << 10) |(GPIO91_DIR      << 11) |
     (GPIO92_DIR      << 12) |(GPIO93_DIR      << 13) |(GPIO94_DIR      << 14) |(GPIO95_DIR      << 15)),

    ((GPIO96_DIR      <<  0) |(GPIO97_DIR      <<  1) |(GPIO98_DIR      <<  2) |(GPIO99_DIR      <<  3) |
     (GPIO100_DIR     <<  4) |(GPIO101_DIR     <<  5) |(GPIO102_DIR     <<  6) |(GPIO103_DIR     <<  7) |
     (GPIO104_DIR     <<  8) |(GPIO105_DIR     <<  9) |(GPIO106_DIR     << 10) |(GPIO107_DIR     << 11) |
     (GPIO108_DIR     << 12) |(GPIO109_DIR     << 13) |(GPIO110_DIR     << 14) |(GPIO111_DIR     << 15)),

    ((GPIO112_DIR     <<  0) |(GPIO113_DIR     <<  1) |(GPIO114_DIR     <<  2) |(GPIO115_DIR     <<  3) |
     (GPIO116_DIR     <<  4) |(GPIO117_DIR     <<  5) |(GPIO118_DIR     <<  6) |(GPIO119_DIR     <<  7) |
     (GPIO120_DIR     <<  8) |(GPIO121_DIR     <<  9) |(GPIO122_DIR     << 10) |(GPIO123_DIR     << 11) |
     (GPIO124_DIR     << 12) |(GPIO125_DIR     << 13) |(GPIO126_DIR     << 14) |(GPIO127_DIR     << 15)),

    ((GPIO128_DIR     <<  0) |(GPIO129_DIR     <<  1) |(GPIO130_DIR     <<  2) |(GPIO131_DIR     <<  3) |
     (GPIO132_DIR     <<  4) |(GPIO133_DIR     <<  5) |(GPIO134_DIR     <<  6) |(GPIO135_DIR     <<  7) |
     (GPIO136_DIR     <<  8) |(GPIO137_DIR     <<  9) |(GPIO138_DIR     << 10) |(GPIO139_DIR     << 11) |
     (GPIO140_DIR     << 12) |(GPIO141_DIR     << 13) |(GPIO142_DIR     << 14) |(GPIO143_DIR     << 15)),

    ((GPIO144_DIR     <<  0) |(GPIO145_DIR     <<  1) |(GPIO146_DIR     <<  2)),
}; /*end of gpio_init_dir_data*/
/*----------------------------------------------------------------------------*/
u16 gpio_init_pullen_data[] = {
    ((GPIO0_PULLEN    <<  0) |(GPIO1_PULLEN    <<  1) |(GPIO2_PULLEN    <<  2) |(GPIO3_PULLEN    <<  3) |
     (GPIO4_PULLEN    <<  4) |(GPIO5_PULLEN    <<  5) |(GPIO6_PULLEN    <<  6) |(GPIO7_PULLEN    <<  7) |
     (GPIO8_PULLEN    <<  8) |(GPIO9_PULLEN    <<  9) |(GPIO10_PULLEN   << 10) |(GPIO11_PULLEN   << 11) |
     (GPIO12_PULLEN   << 12) |(GPIO13_PULLEN   << 13) |(GPIO14_PULLEN   << 14) |(GPIO15_PULLEN   << 15)),

    ((GPIO16_PULLEN   <<  0) |(GPIO17_PULLEN   <<  1) |(GPIO18_PULLEN   <<  2) |(GPIO19_PULLEN   <<  3) |
     (GPIO20_PULLEN   <<  4) |(GPIO21_PULLEN   <<  5) |(GPIO22_PULLEN   <<  6) |(GPIO23_PULLEN   <<  7) |
     (GPIO24_PULLEN   <<  8) |(GPIO25_PULLEN   <<  9) |(GPIO26_PULLEN   << 10) |(GPIO27_PULLEN   << 11) |
     (GPIO28_PULLEN   << 12) |(GPIO29_PULLEN   << 13) |(GPIO30_PULLEN   << 14) |(GPIO31_PULLEN   << 15)),

    ((GPIO32_PULLEN   <<  0) |(GPIO33_PULLEN   <<  1) |(GPIO34_PULLEN   <<  2) |(GPIO35_PULLEN   <<  3) |
     (GPIO36_PULLEN   <<  4) |(GPIO37_PULLEN   <<  5) |(GPIO38_PULLEN   <<  6) |(GPIO39_PULLEN   <<  7) |
     (GPIO40_PULLEN   <<  8) |(GPIO41_PULLEN   <<  9) |(GPIO42_PULLEN   << 10) |(GPIO43_PULLEN   << 11) |
     (GPIO44_PULLEN   << 12) |(GPIO45_PULLEN   << 13) |(GPIO46_PULLEN   << 14) |(GPIO47_PULLEN   << 15)),

    ((GPIO48_PULLEN   <<  0) |(GPIO49_PULLEN   <<  1) |(GPIO50_PULLEN   <<  2) |(GPIO51_PULLEN   <<  3) |
     (GPIO52_PULLEN   <<  4) |(GPIO53_PULLEN   <<  5) |(GPIO54_PULLEN   <<  6) |(GPIO55_PULLEN   <<  7) |
     (GPIO56_PULLEN   <<  8) |(GPIO57_PULLEN   <<  9) |(GPIO58_PULLEN   << 10) |(GPIO59_PULLEN   << 11) |
     (GPIO60_PULLEN   << 12) |(GPIO61_PULLEN   << 13) |(GPIO62_PULLEN   << 14) |(GPIO63_PULLEN   << 15)),

    ((GPIO64_PULLEN   <<  0) |(GPIO65_PULLEN   <<  1) |(GPIO66_PULLEN   <<  2) |(GPIO67_PULLEN   <<  3) |
     (GPIO68_PULLEN   <<  4) |(GPIO69_PULLEN   <<  5) |(GPIO70_PULLEN   <<  6) |(GPIO71_PULLEN   <<  7) |
     (GPIO72_PULLEN   <<  8) |(GPIO73_PULLEN   <<  9) |(GPIO74_PULLEN   << 10) |(GPIO75_PULLEN   << 11) |
     (GPIO76_PULLEN   << 12) |(GPIO77_PULLEN   << 13) |(GPIO78_PULLEN   << 14) |(GPIO79_PULLEN   << 15)),

    ((GPIO80_PULLEN   <<  0) |(GPIO81_PULLEN   <<  1) |(GPIO82_PULLEN   <<  2) |(GPIO83_PULLEN   <<  3) |
     (GPIO84_PULLEN   <<  4) |(GPIO85_PULLEN   <<  5) |(GPIO86_PULLEN   <<  6) |(GPIO87_PULLEN   <<  7) |
     (GPIO88_PULLEN   <<  8) |(GPIO89_PULLEN   <<  9) |(GPIO90_PULLEN   << 10) |(GPIO91_PULLEN   << 11) |
     (GPIO92_PULLEN   << 12) |(GPIO93_PULLEN   << 13) |(GPIO94_PULLEN   << 14) |(GPIO95_PULLEN   << 15)),

    ((GPIO96_PULLEN   <<  0) |(GPIO97_PULLEN   <<  1) |(GPIO98_PULLEN   <<  2) |(GPIO99_PULLEN   <<  3) |
     (GPIO100_PULLEN  <<  4) |(GPIO101_PULLEN  <<  5) |(GPIO102_PULLEN  <<  6) |(GPIO103_PULLEN  <<  7) |
     (GPIO104_PULLEN  <<  8) |(GPIO105_PULLEN  <<  9) |(GPIO106_PULLEN  << 10) |(GPIO107_PULLEN  << 11) |
     (GPIO108_PULLEN  << 12) |(GPIO109_PULLEN  << 13) |(GPIO110_PULLEN  << 14) |(GPIO111_PULLEN  << 15)),

    ((GPIO112_PULLEN  <<  0) |(GPIO113_PULLEN  <<  1) |(GPIO114_PULLEN  <<  2) |(GPIO115_PULLEN  <<  3) |
     (GPIO116_PULLEN  <<  4) |(GPIO117_PULLEN  <<  5) |(GPIO118_PULLEN  <<  6) |(GPIO119_PULLEN  <<  7) |
     (GPIO120_PULLEN  <<  8) |(GPIO121_PULLEN  <<  9) |(GPIO122_PULLEN  << 10) |(GPIO123_PULLEN  << 11) |
     (GPIO124_PULLEN  << 12) |(GPIO125_PULLEN  << 13) |(GPIO126_PULLEN  << 14) |(GPIO127_PULLEN  << 15)),

    ((GPIO128_PULLEN  <<  0) |(GPIO129_PULLEN  <<  1) |(GPIO130_PULLEN  <<  2) |(GPIO131_PULLEN  <<  3) |
     (GPIO132_PULLEN  <<  4) |(GPIO133_PULLEN  <<  5) |(GPIO134_PULLEN  <<  6) |(GPIO135_PULLEN  <<  7) |
     (GPIO136_PULLEN  <<  8) |(GPIO137_PULLEN  <<  9) |(GPIO138_PULLEN  << 10) |(GPIO139_PULLEN  << 11) |
     (GPIO140_PULLEN  << 12) |(GPIO141_PULLEN  << 13) |(GPIO142_PULLEN  << 14) |(GPIO143_PULLEN  << 15)),

    ((GPIO144_PULLEN  <<  0) |(GPIO145_PULLEN  <<  1) |(GPIO146_PULLEN  <<  2)),
}; /*end of gpio_init_pullen_data*/
/*----------------------------------------------------------------------------*/
u16 gpio_init_pullsel_data[] = {
    ((GPIO0_PULL      <<  0) |(GPIO1_PULL      <<  1) |(GPIO2_PULL      <<  2) |(GPIO3_PULL      <<  3) |
     (GPIO4_PULL      <<  4) |(GPIO5_PULL      <<  5) |(GPIO6_PULL      <<  6) |(GPIO7_PULL      <<  7) |
     (GPIO8_PULL      <<  8) |(GPIO9_PULL      <<  9) |(GPIO10_PULL     << 10) |(GPIO11_PULL     << 11) |
     (GPIO12_PULL     << 12) |(GPIO13_PULL     << 13) |(GPIO14_PULL     << 14) |(GPIO15_PULL     << 15)),

    ((GPIO16_PULL     <<  0) |(GPIO17_PULL     <<  1) |(GPIO18_PULL     <<  2) |(GPIO19_PULL     <<  3) |
     (GPIO20_PULL     <<  4) |(GPIO21_PULL     <<  5) |(GPIO22_PULL     <<  6) |(GPIO23_PULL     <<  7) |
     (GPIO24_PULL     <<  8) |(GPIO25_PULL     <<  9) |(GPIO26_PULL     << 10) |(GPIO27_PULL     << 11) |
     (GPIO28_PULL     << 12) |(GPIO29_PULL     << 13) |(GPIO30_PULL     << 14) |(GPIO31_PULL     << 15)),

    ((GPIO32_PULL     <<  0) |(GPIO33_PULL     <<  1) |(GPIO34_PULL     <<  2) |(GPIO35_PULL     <<  3) |
     (GPIO36_PULL     <<  4) |(GPIO37_PULL     <<  5) |(GPIO38_PULL     <<  6) |(GPIO39_PULL     <<  7) |
     (GPIO40_PULL     <<  8) |(GPIO41_PULL     <<  9) |(GPIO42_PULL     << 10) |(GPIO43_PULL     << 11) |
     (GPIO44_PULL     << 12) |(GPIO45_PULL     << 13) |(GPIO46_PULL     << 14) |(GPIO47_PULL     << 15)),

    ((GPIO48_PULL     <<  0) |(GPIO49_PULL     <<  1) |(GPIO50_PULL     <<  2) |(GPIO51_PULL     <<  3) |
     (GPIO52_PULL     <<  4) |(GPIO53_PULL     <<  5) |(GPIO54_PULL     <<  6) |(GPIO55_PULL     <<  7) |
     (GPIO56_PULL     <<  8) |(GPIO57_PULL     <<  9) |(GPIO58_PULL     << 10) |(GPIO59_PULL     << 11) |
     (GPIO60_PULL     << 12) |(GPIO61_PULL     << 13) |(GPIO62_PULL     << 14) |(GPIO63_PULL     << 15)),

    ((GPIO64_PULL     <<  0) |(GPIO65_PULL     <<  1) |(GPIO66_PULL     <<  2) |(GPIO67_PULL     <<  3) |
     (GPIO68_PULL     <<  4) |(GPIO69_PULL     <<  5) |(GPIO70_PULL     <<  6) |(GPIO71_PULL     <<  7) |
     (GPIO72_PULL     <<  8) |(GPIO73_PULL     <<  9) |(GPIO74_PULL     << 10) |(GPIO75_PULL     << 11) |
     (GPIO76_PULL     << 12) |(GPIO77_PULL     << 13) |(GPIO78_PULL     << 14) |(GPIO79_PULL     << 15)),

    ((GPIO80_PULL     <<  0) |(GPIO81_PULL     <<  1) |(GPIO82_PULL     <<  2) |(GPIO83_PULL     <<  3) |
     (GPIO84_PULL     <<  4) |(GPIO85_PULL     <<  5) |(GPIO86_PULL     <<  6) |(GPIO87_PULL     <<  7) |
     (GPIO88_PULL     <<  8) |(GPIO89_PULL     <<  9) |(GPIO90_PULL     << 10) |(GPIO91_PULL     << 11) |
     (GPIO92_PULL     << 12) |(GPIO93_PULL     << 13) |(GPIO94_PULL     << 14) |(GPIO95_PULL     << 15)),

    ((GPIO96_PULL     <<  0) |(GPIO97_PULL     <<  1) |(GPIO98_PULL     <<  2) |(GPIO99_PULL     <<  3) |
     (GPIO100_PULL    <<  4) |(GPIO101_PULL    <<  5) |(GPIO102_PULL    <<  6) |(GPIO103_PULL    <<  7) |
     (GPIO104_PULL    <<  8) |(GPIO105_PULL    <<  9) |(GPIO106_PULL    << 10) |(GPIO107_PULL    << 11) |
     (GPIO108_PULL    << 12) |(GPIO109_PULL    << 13) |(GPIO110_PULL    << 14) |(GPIO111_PULL    << 15)),

    ((GPIO112_PULL    <<  0) |(GPIO113_PULL    <<  1) |(GPIO114_PULL    <<  2) |(GPIO115_PULL    <<  3) |
     (GPIO116_PULL    <<  4) |(GPIO117_PULL    <<  5) |(GPIO118_PULL    <<  6) |(GPIO119_PULL    <<  7) |
     (GPIO120_PULL    <<  8) |(GPIO121_PULL    <<  9) |(GPIO122_PULL    << 10) |(GPIO123_PULL    << 11) |
     (GPIO124_PULL    << 12) |(GPIO125_PULL    << 13) |(GPIO126_PULL    << 14) |(GPIO127_PULL    << 15)),

    ((GPIO128_PULL    <<  0) |(GPIO129_PULL    <<  1) |(GPIO130_PULL    <<  2) |(GPIO131_PULL    <<  3) |
     (GPIO132_PULL    <<  4) |(GPIO133_PULL    <<  5) |(GPIO134_PULL    <<  6) |(GPIO135_PULL    <<  7) |
     (GPIO136_PULL    <<  8) |(GPIO137_PULL    <<  9) |(GPIO138_PULL    << 10) |(GPIO139_PULL    << 11) |
     (GPIO140_PULL    << 12) |(GPIO141_PULL    << 13) |(GPIO142_PULL    << 14) |(GPIO143_PULL    << 15)),

    ((GPIO144_PULL    <<  0) |(GPIO145_PULL    <<  1) |(GPIO146_PULL    <<  2)),
}; /*end of gpio_init_pullsel_data*/
/*----------------------------------------------------------------------------*/
u16 gpio_init_dinv_data[] = {
    ((GPIO0_DATAINV   <<  0) |(GPIO1_DATAINV   <<  1) |(GPIO2_DATAINV   <<  2) |(GPIO3_DATAINV   <<  3) |
     (GPIO4_DATAINV   <<  4) |(GPIO5_DATAINV   <<  5) |(GPIO6_DATAINV   <<  6) |(GPIO7_DATAINV   <<  7) |
     (GPIO8_DATAINV   <<  8) |(GPIO9_DATAINV   <<  9) |(GPIO10_DATAINV  << 10) |(GPIO11_DATAINV  << 11) |
     (GPIO12_DATAINV  << 12) |(GPIO13_DATAINV  << 13) |(GPIO14_DATAINV  << 14) |(GPIO15_DATAINV  << 15)),

    ((GPIO16_DATAINV  <<  0) |(GPIO17_DATAINV  <<  1) |(GPIO18_DATAINV  <<  2) |(GPIO19_DATAINV  <<  3) |
     (GPIO20_DATAINV  <<  4) |(GPIO21_DATAINV  <<  5) |(GPIO22_DATAINV  <<  6) |(GPIO23_DATAINV  <<  7) |
     (GPIO24_DATAINV  <<  8) |(GPIO25_DATAINV  <<  9) |(GPIO26_DATAINV  << 10) |(GPIO27_DATAINV  << 11) |
     (GPIO28_DATAINV  << 12) |(GPIO29_DATAINV  << 13) |(GPIO30_DATAINV  << 14) |(GPIO31_DATAINV  << 15)),

    ((GPIO32_DATAINV  <<  0) |(GPIO33_DATAINV  <<  1) |(GPIO34_DATAINV  <<  2) |(GPIO35_DATAINV  <<  3) |
     (GPIO36_DATAINV  <<  4) |(GPIO37_DATAINV  <<  5) |(GPIO38_DATAINV  <<  6) |(GPIO39_DATAINV  <<  7) |
     (GPIO40_DATAINV  <<  8) |(GPIO41_DATAINV  <<  9) |(GPIO42_DATAINV  << 10) |(GPIO43_DATAINV  << 11) |
     (GPIO44_DATAINV  << 12) |(GPIO45_DATAINV  << 13) |(GPIO46_DATAINV  << 14) |(GPIO47_DATAINV  << 15)),

    ((GPIO48_DATAINV  <<  0) |(GPIO49_DATAINV  <<  1) |(GPIO50_DATAINV  <<  2) |(GPIO51_DATAINV  <<  3) |
     (GPIO52_DATAINV  <<  4) |(GPIO53_DATAINV  <<  5) |(GPIO54_DATAINV  <<  6) |(GPIO55_DATAINV  <<  7) |
     (GPIO56_DATAINV  <<  8) |(GPIO57_DATAINV  <<  9) |(GPIO58_DATAINV  << 10) |(GPIO59_DATAINV  << 11) |
     (GPIO60_DATAINV  << 12) |(GPIO61_DATAINV  << 13) |(GPIO62_DATAINV  << 14) |(GPIO63_DATAINV  << 15)),

    ((GPIO64_DATAINV  <<  0) |(GPIO65_DATAINV  <<  1) |(GPIO66_DATAINV  <<  2) |(GPIO67_DATAINV  <<  3) |
     (GPIO68_DATAINV  <<  4) |(GPIO69_DATAINV  <<  5) |(GPIO70_DATAINV  <<  6) |(GPIO71_DATAINV  <<  7) |
     (GPIO72_DATAINV  <<  8) |(GPIO73_DATAINV  <<  9) |(GPIO74_DATAINV  << 10) |(GPIO75_DATAINV  << 11) |
     (GPIO76_DATAINV  << 12) |(GPIO77_DATAINV  << 13) |(GPIO78_DATAINV  << 14) |(GPIO79_DATAINV  << 15)),

    ((GPIO80_DATAINV  <<  0) |(GPIO81_DATAINV  <<  1) |(GPIO82_DATAINV  <<  2) |(GPIO83_DATAINV  <<  3) |
     (GPIO84_DATAINV  <<  4) |(GPIO85_DATAINV  <<  5) |(GPIO86_DATAINV  <<  6) |(GPIO87_DATAINV  <<  7) |
     (GPIO88_DATAINV  <<  8) |(GPIO89_DATAINV  <<  9) |(GPIO90_DATAINV  << 10) |(GPIO91_DATAINV  << 11) |
     (GPIO92_DATAINV  << 12) |(GPIO93_DATAINV  << 13) |(GPIO94_DATAINV  << 14) |(GPIO95_DATAINV  << 15)),

    ((GPIO96_DATAINV  <<  0) |(GPIO97_DATAINV  <<  1) |(GPIO98_DATAINV  <<  2) |(GPIO99_DATAINV  <<  3) |
     (GPIO100_DATAINV <<  4) |(GPIO101_DATAINV <<  5) |(GPIO102_DATAINV <<  6) |(GPIO103_DATAINV <<  7) |
     (GPIO104_DATAINV <<  8) |(GPIO105_DATAINV <<  9) |(GPIO106_DATAINV << 10) |(GPIO107_DATAINV << 11) |
     (GPIO108_DATAINV << 12) |(GPIO109_DATAINV << 13) |(GPIO110_DATAINV << 14) |(GPIO111_DATAINV << 15)),

    ((GPIO112_DATAINV <<  0) |(GPIO113_DATAINV <<  1) |(GPIO114_DATAINV <<  2) |(GPIO115_DATAINV <<  3) |
     (GPIO116_DATAINV <<  4) |(GPIO117_DATAINV <<  5) |(GPIO118_DATAINV <<  6) |(GPIO119_DATAINV <<  7) |
     (GPIO120_DATAINV <<  8) |(GPIO121_DATAINV <<  9) |(GPIO122_DATAINV << 10) |(GPIO123_DATAINV << 11) |
     (GPIO124_DATAINV << 12) |(GPIO125_DATAINV << 13) |(GPIO126_DATAINV << 14) |(GPIO127_DATAINV << 15)),

    ((GPIO128_DATAINV <<  0) |(GPIO129_DATAINV <<  1) |(GPIO130_DATAINV <<  2) |(GPIO131_DATAINV <<  3) |
     (GPIO132_DATAINV <<  4) |(GPIO133_DATAINV <<  5) |(GPIO134_DATAINV <<  6) |(GPIO135_DATAINV <<  7) |
     (GPIO136_DATAINV <<  8) |(GPIO137_DATAINV <<  9) |(GPIO138_DATAINV << 10) |(GPIO139_DATAINV << 11) |
     (GPIO140_DATAINV << 12) |(GPIO141_DATAINV << 13) |(GPIO142_DATAINV << 14) |(GPIO143_DATAINV << 15)),

    ((GPIO144_DATAINV <<  0) |(GPIO145_DATAINV <<  1) |(GPIO146_DATAINV <<  2)),
}; /*end of gpio_init_dinv_data*/
/*----------------------------------------------------------------------------*/
u16 gpio_init_dout_data[] = {
    ((GPIO0_DATAOUT   <<  0) |(GPIO1_DATAOUT   <<  1) |(GPIO2_DATAOUT   <<  2) |(GPIO3_DATAOUT   <<  3) |
     (GPIO4_DATAOUT   <<  4) |(GPIO5_DATAOUT   <<  5) |(GPIO6_DATAOUT   <<  6) |(GPIO7_DATAOUT   <<  7) |
     (GPIO8_DATAOUT   <<  8) |(GPIO9_DATAOUT   <<  9) |(GPIO10_DATAOUT  << 10) |(GPIO11_DATAOUT  << 11) |
     (GPIO12_DATAOUT  << 12) |(GPIO13_DATAOUT  << 13) |(GPIO14_DATAOUT  << 14) |(GPIO15_DATAOUT  << 15)),

    ((GPIO16_DATAOUT  <<  0) |(GPIO17_DATAOUT  <<  1) |(GPIO18_DATAOUT  <<  2) |(GPIO19_DATAOUT  <<  3) |
     (GPIO20_DATAOUT  <<  4) |(GPIO21_DATAOUT  <<  5) |(GPIO22_DATAOUT  <<  6) |(GPIO23_DATAOUT  <<  7) |
     (GPIO24_DATAOUT  <<  8) |(GPIO25_DATAOUT  <<  9) |(GPIO26_DATAOUT  << 10) |(GPIO27_DATAOUT  << 11) |
     (GPIO28_DATAOUT  << 12) |(GPIO29_DATAOUT  << 13) |(GPIO30_DATAOUT  << 14) |(GPIO31_DATAOUT  << 15)),

    ((GPIO32_DATAOUT  <<  0) |(GPIO33_DATAOUT  <<  1) |(GPIO34_DATAOUT  <<  2) |(GPIO35_DATAOUT  <<  3) |
     (GPIO36_DATAOUT  <<  4) |(GPIO37_DATAOUT  <<  5) |(GPIO38_DATAOUT  <<  6) |(GPIO39_DATAOUT  <<  7) |
     (GPIO40_DATAOUT  <<  8) |(GPIO41_DATAOUT  <<  9) |(GPIO42_DATAOUT  << 10) |(GPIO43_DATAOUT  << 11) |
     (GPIO44_DATAOUT  << 12) |(GPIO45_DATAOUT  << 13) |(GPIO46_DATAOUT  << 14) |(GPIO47_DATAOUT  << 15)),

    ((GPIO48_DATAOUT  <<  0) |(GPIO49_DATAOUT  <<  1) |(GPIO50_DATAOUT  <<  2) |(GPIO51_DATAOUT  <<  3) |
     (GPIO52_DATAOUT  <<  4) |(GPIO53_DATAOUT  <<  5) |(GPIO54_DATAOUT  <<  6) |(GPIO55_DATAOUT  <<  7) |
     (GPIO56_DATAOUT  <<  8) |(GPIO57_DATAOUT  <<  9) |(GPIO58_DATAOUT  << 10) |(GPIO59_DATAOUT  << 11) |
     (GPIO60_DATAOUT  << 12) |(GPIO61_DATAOUT  << 13) |(GPIO62_DATAOUT  << 14) |(GPIO63_DATAOUT  << 15)),

    ((GPIO64_DATAOUT  <<  0) |(GPIO65_DATAOUT  <<  1) |(GPIO66_DATAOUT  <<  2) |(GPIO67_DATAOUT  <<  3) |
     (GPIO68_DATAOUT  <<  4) |(GPIO69_DATAOUT  <<  5) |(GPIO70_DATAOUT  <<  6) |(GPIO71_DATAOUT  <<  7) |
     (GPIO72_DATAOUT  <<  8) |(GPIO73_DATAOUT  <<  9) |(GPIO74_DATAOUT  << 10) |(GPIO75_DATAOUT  << 11) |
     (GPIO76_DATAOUT  << 12) |(GPIO77_DATAOUT  << 13) |(GPIO78_DATAOUT  << 14) |(GPIO79_DATAOUT  << 15)),

    ((GPIO80_DATAOUT  <<  0) |(GPIO81_DATAOUT  <<  1) |(GPIO82_DATAOUT  <<  2) |(GPIO83_DATAOUT  <<  3) |
     (GPIO84_DATAOUT  <<  4) |(GPIO85_DATAOUT  <<  5) |(GPIO86_DATAOUT  <<  6) |(GPIO87_DATAOUT  <<  7) |
     (GPIO88_DATAOUT  <<  8) |(GPIO89_DATAOUT  <<  9) |(GPIO90_DATAOUT  << 10) |(GPIO91_DATAOUT  << 11) |
     (GPIO92_DATAOUT  << 12) |(GPIO93_DATAOUT  << 13) |(GPIO94_DATAOUT  << 14) |(GPIO95_DATAOUT  << 15)),

    ((GPIO96_DATAOUT  <<  0) |(GPIO97_DATAOUT  <<  1) |(GPIO98_DATAOUT  <<  2) |(GPIO99_DATAOUT  <<  3) |
     (GPIO100_DATAOUT <<  4) |(GPIO101_DATAOUT <<  5) |(GPIO102_DATAOUT <<  6) |(GPIO103_DATAOUT <<  7) |
     (GPIO104_DATAOUT <<  8) |(GPIO105_DATAOUT <<  9) |(GPIO106_DATAOUT << 10) |(GPIO107_DATAOUT << 11) |
     (GPIO108_DATAOUT << 12) |(GPIO109_DATAOUT << 13) |(GPIO110_DATAOUT << 14) |(GPIO111_DATAOUT << 15)),

    ((GPIO112_DATAOUT <<  0) |(GPIO113_DATAOUT <<  1) |(GPIO114_DATAOUT <<  2) |(GPIO115_DATAOUT <<  3) |
     (GPIO116_DATAOUT <<  4) |(GPIO117_DATAOUT <<  5) |(GPIO118_DATAOUT <<  6) |(GPIO119_DATAOUT <<  7) |
     (GPIO120_DATAOUT <<  8) |(GPIO121_DATAOUT <<  9) |(GPIO122_DATAOUT << 10) |(GPIO123_DATAOUT << 11) |
     (GPIO124_DATAOUT << 12) |(GPIO125_DATAOUT << 13) |(GPIO126_DATAOUT << 14) |(GPIO127_DATAOUT << 15)),

    ((GPIO128_DATAOUT <<  0) |(GPIO129_DATAOUT <<  1) |(GPIO130_DATAOUT <<  2) |(GPIO131_DATAOUT <<  3) |
     (GPIO132_DATAOUT <<  4) |(GPIO133_DATAOUT <<  5) |(GPIO134_DATAOUT <<  6) |(GPIO135_DATAOUT <<  7) |
     (GPIO136_DATAOUT <<  8) |(GPIO137_DATAOUT <<  9) |(GPIO138_DATAOUT << 10) |(GPIO139_DATAOUT << 11) |
     (GPIO140_DATAOUT << 12) |(GPIO141_DATAOUT << 13) |(GPIO142_DATAOUT << 14) |(GPIO143_DATAOUT << 15)),

    ((GPIO144_DATAOUT <<  0) |(GPIO145_DATAOUT <<  1) |(GPIO146_DATAOUT <<  2)),
}; /*end of gpio_init_dout_data*/
/*----------------------------------------------------------------------------*/
u16 gpio_init_mode_data[] = {
    ((GPIO0_MODE      <<  0) |(GPIO1_MODE      <<  2) |(GPIO2_MODE      <<  4) |(GPIO3_MODE      <<  6) |
     (GPIO4_MODE      <<  8) |(GPIO5_MODE      << 10) |(GPIO6_MODE      << 12) |(GPIO7_MODE      << 14)),

    ((GPIO8_MODE      <<  0) |(GPIO9_MODE      <<  2) |(GPIO10_MODE     <<  4) |(GPIO11_MODE     <<  6) |
     (GPIO12_MODE     <<  8) |(GPIO13_MODE     << 10) |(GPIO14_MODE     << 12) |(GPIO15_MODE     << 14)),

    ((GPIO16_MODE     <<  0) |(GPIO17_MODE     <<  2) |(GPIO18_MODE     <<  4) |(GPIO19_MODE     <<  6) |
     (GPIO20_MODE     <<  8) |(GPIO21_MODE     << 10) |(GPIO22_MODE     << 12) |(GPIO23_MODE     << 14)),

    ((GPIO24_MODE     <<  0) |(GPIO25_MODE     <<  2) |(GPIO26_MODE     <<  4) |(GPIO27_MODE     <<  6) |
     (GPIO28_MODE     <<  8) |(GPIO29_MODE     << 10) |(GPIO30_MODE     << 12) |(GPIO31_MODE     << 14)),

    ((GPIO32_MODE     <<  0) |(GPIO33_MODE     <<  2) |(GPIO34_MODE     <<  4) |(GPIO35_MODE     <<  6) |
     (GPIO36_MODE     <<  8) |(GPIO37_MODE     << 10) |(GPIO38_MODE     << 12) |(GPIO39_MODE     << 14)),

    ((GPIO40_MODE     <<  0) |(GPIO41_MODE     <<  2) |(GPIO42_MODE     <<  4) |(GPIO43_MODE     <<  6) |
     (GPIO44_MODE     <<  8) |(GPIO45_MODE     << 10) |(GPIO46_MODE     << 12) |(GPIO47_MODE     << 14)),

    ((GPIO48_MODE     <<  0) |(GPIO49_MODE     <<  2) |(GPIO50_MODE     <<  4) |(GPIO51_MODE     <<  6) |
     (GPIO52_MODE     <<  8) |(GPIO53_MODE     << 10) |(GPIO54_MODE     << 12) |(GPIO55_MODE     << 14)),

    ((GPIO56_MODE     <<  0) |(GPIO57_MODE     <<  2) |(GPIO58_MODE     <<  4) |(GPIO59_MODE     <<  6) |
     (GPIO60_MODE     <<  8) |(GPIO61_MODE     << 10) |(GPIO62_MODE     << 12) |(GPIO63_MODE     << 14)),

    ((GPIO64_MODE     <<  0) |(GPIO65_MODE     <<  2) |(GPIO66_MODE     <<  4) |(GPIO67_MODE     <<  6) |
     (GPIO68_MODE     <<  8) |(GPIO69_MODE     << 10) |(GPIO70_MODE     << 12) |(GPIO71_MODE     << 14)),

    ((GPIO72_MODE     <<  0) |(GPIO73_MODE     <<  2) |(GPIO74_MODE     <<  4) |(GPIO75_MODE     <<  6) |
     (GPIO76_MODE     <<  8) |(GPIO77_MODE     << 10) |(GPIO78_MODE     << 12) |(GPIO79_MODE     << 14)),

    ((GPIO80_MODE     <<  0) |(GPIO81_MODE     <<  2) |(GPIO82_MODE     <<  4) |(GPIO83_MODE     <<  6) |
     (GPIO84_MODE     <<  8) |(GPIO85_MODE     << 10) |(GPIO86_MODE     << 12) |(GPIO87_MODE     << 14)),

    ((GPIO88_MODE     <<  0) |(GPIO89_MODE     <<  2) |(GPIO90_MODE     <<  4) |(GPIO91_MODE     <<  6) |
     (GPIO92_MODE     <<  8) |(GPIO93_MODE     << 10) |(GPIO94_MODE     << 12) |(GPIO95_MODE     << 14)),

    ((GPIO96_MODE     <<  0) |(GPIO97_MODE     <<  2) |(GPIO98_MODE     <<  4) |(GPIO99_MODE     <<  6) |
     (GPIO100_MODE    <<  8) |(GPIO101_MODE    << 10) |(GPIO102_MODE    << 12) |(GPIO103_MODE    << 14)),

    ((GPIO104_MODE    <<  0) |(GPIO105_MODE    <<  2) |(GPIO106_MODE    <<  4) |(GPIO107_MODE    <<  6) |
     (GPIO108_MODE    <<  8) |(GPIO109_MODE    << 10) |(GPIO110_MODE    << 12) |(GPIO111_MODE    << 14)),

    ((GPIO112_MODE    <<  0) |(GPIO113_MODE    <<  2) |(GPIO114_MODE    <<  4) |(GPIO115_MODE    <<  6) |
     (GPIO116_MODE    <<  8) |(GPIO117_MODE    << 10) |(GPIO118_MODE    << 12) |(GPIO119_MODE    << 14)),

    ((GPIO120_MODE    <<  0) |(GPIO121_MODE    <<  2) |(GPIO122_MODE    <<  4) |(GPIO123_MODE    <<  6) |
     (GPIO124_MODE    <<  8) |(GPIO125_MODE    << 10) |(GPIO126_MODE    << 12) |(GPIO127_MODE    << 14)),

    ((GPIO128_MODE    <<  0) |(GPIO129_MODE    <<  2) |(GPIO130_MODE    <<  4) |(GPIO131_MODE    <<  6) |
     (GPIO132_MODE    <<  8) |(GPIO133_MODE    << 10) |(GPIO134_MODE    << 12) |(GPIO135_MODE    << 14)),

    ((GPIO136_MODE    <<  0) |(GPIO137_MODE    <<  2) |(GPIO138_MODE    <<  4) |(GPIO139_MODE    <<  6) |
     (GPIO140_MODE    <<  8) |(GPIO141_MODE    << 10) |(GPIO142_MODE    << 12) |(GPIO143_MODE    << 14)),

    ((GPIO144_MODE    <<  0) |(GPIO145_MODE    <<  2) |(GPIO146_MODE    <<  4)),
}; /*end of gpio_init_mode_data*/
/*----------------------------------------------------------------------------*/
static GPIO_REGS mask;
/*----------------------------------------------------------------------------*/
#if defined(GPIO_INIT_DEBUG)
static GPIO_REGS saved;
#endif 
/*----------------------------------------------------------------------------*/
void mt_gpio_unlock_init(int all)
{
    if (!all)
        memset(&mask, 0x00, sizeof(mask));
    else
        memset(&mask, 0xff, sizeof(mask));
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL(mt_gpio_unlock_init);
/*----------------------------------------------------------------------------*/
void mt_gpio_unlock_pin(int pin, u32 conf)
{
    u32 regno, offset;
    if (conf & GPIO_CONF_MODE) {
        regno  = pin / MAX_GPIO_MODE_PER_REG;
        offset = pin % MAX_GPIO_MODE_PER_REG;
        mask.mode[regno].val |= (0x03 << 2*offset);
    }
    if (conf & GPIO_CONF_DIR) {
        regno  = pin / MAX_GPIO_REG_BITS;
        offset = pin % MAX_GPIO_REG_BITS;
        mask.dir[regno].val  |= (1UL << offset);
    }
    if (conf & GPIO_CONF_PULLEN) {
        regno  = pin / MAX_GPIO_REG_BITS;
        offset = pin % MAX_GPIO_REG_BITS;
        mask.pullen[regno].val |= (1UL << offset);
    }
    if (conf & GPIO_CONF_PULLSEL) {
        regno  = pin / MAX_GPIO_REG_BITS;
        offset = pin % MAX_GPIO_REG_BITS;
        mask.pullsel[regno].val |= (1UL << offset);
    }
    if (conf & GPIO_CONF_DINV) {
        regno  = pin / MAX_GPIO_REG_BITS;
        offset = pin % MAX_GPIO_REG_BITS;
        mask.dinv[regno].val |= (1UL << offset);
    }
    if (conf & GPIO_CONF_DOUT) {
        regno  = pin / MAX_GPIO_REG_BITS;
        offset = pin % MAX_GPIO_REG_BITS;
        mask.dout[regno].val |= (1UL << offset);
    }
    if (conf & GPIO_CONF_DIN) {
        regno  = pin / MAX_GPIO_REG_BITS;
        offset = pin % MAX_GPIO_REG_BITS;
        mask.din[regno].val |= (1UL << offset);        
    }
}
/*----------------------------------------------------------------------------*/
void mt_gpio_unlock_dump(void)
{
    GIO_DBG("%s\n",__func__);
    return mt_gpio_dump(&mask);
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL(mt_gpio_unlock_pin);
/*----------------------------------------------------------------------------*/
void mt_gpio_set_default(void)
{   
    GPIO_REGS *pReg = (GPIO_REGS*)(GPIO_BASE);
    GPIO_REGS cur;
    int idx;
    u32 val;
    
    mt_gpio_load(&cur);

    for (idx = 0; idx < sizeof(pReg->dir)/sizeof(pReg->dir[0]); idx++) {
        val = (gpio_init_dir_data[idx] & mask.dir[idx].val) | (cur.dir[idx].val & ~mask.dir[idx].val);
        if (0 == (cur.dir[idx].val ^ val))
            continue;
        GIO_DBG("dir[%02d]:     0x%08X <=> 0x%08X\n", idx, cur.dir[idx].val, val);    
        __raw_writel(val, &pReg->dir[idx]);
    }
    for (idx = 0; idx < sizeof(pReg->pullen)/sizeof(pReg->pullen[0]); idx++) {
        val = (gpio_init_pullen_data[idx] & mask.pullen[idx].val) | (cur.pullen[idx].val & ~mask.pullen[idx].val);
        if (0 == (cur.pullen[idx].val ^ val))
            continue;
        GIO_DBG("pullen[%02d]:  0x%08X <=> 0x%08X\n", idx, cur.pullen[idx].val, val);    
        __raw_writel(val, &pReg->pullen[idx]);
    }
    for (idx = 0; idx < sizeof(pReg->pullsel)/sizeof(pReg->pullsel[0]); idx++) {
        val = (gpio_init_pullsel_data[idx] & mask.pullsel[idx].val) | (cur.pullsel[idx].val & ~mask.pullsel[idx].val);
        if (0 == (cur.pullsel[idx].val ^ val))
            continue;
        GIO_DBG("pullsel[%02d]: 0x%08X <=> 0x%08X\n", idx, cur.pullsel[idx].val, val);    
        __raw_writel(val, &pReg->pullsel[idx]);
    }
    for (idx = 0; idx < sizeof(pReg->dinv)/sizeof(pReg->dinv[0]); idx++) {
        val = (gpio_init_dinv_data[idx] & mask.dinv[idx].val) | (cur.dinv[idx].val & ~mask.dinv[idx].val);
        if (0 == (cur.dinv[idx].val ^ val))
            continue;
        GIO_DBG("dinv[%02d]:    0x%08X <=> 0x%08X\n", idx, cur.dinv[idx].val, val);
        __raw_writel(val, &pReg->dinv[idx]);
    }
    for (idx = 0; idx < sizeof(pReg->dout)/sizeof(pReg->dout[0]); idx++) {
        val = (gpio_init_dout_data[idx] & mask.dout[idx].val) | (cur.dout[idx].val & ~mask.dout[idx].val);
        if (0 == (cur.dout[idx].val ^ val))
            continue;
        GIO_DBG("dout[%02d]:    0x%08X <=> 0x%08X\n", idx, cur.dout[idx].val, val);    
        __raw_writel(val, &pReg->dout[idx]);
    }
    for (idx = 0; idx < sizeof(pReg->mode)/sizeof(pReg->mode[0]); idx++) {
        val = (gpio_init_mode_data[idx] & mask.mode[idx].val) | (cur.mode[idx].val & ~mask.mode[idx].val);
        if (0 == (cur.mode[idx].val ^ val))
            continue;
        GIO_DBG("mode[%02d]:    0x%08X <=> 0x%08X\n", idx, cur.mode[idx].val, val);    
        __raw_writel(val, &pReg->mode[idx]);
    }
    mt_gpio_dump(NULL);
    GIO_DBG("mt_gpio_set_default() done\n");        
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL(mt_gpio_set_default);
/*----------------------------------------------------------------------------*/
void mt_gpio_load(GPIO_REGS *regs) 
{
    GPIO_REGS *pReg = (GPIO_REGS*)(GPIO_BASE);
    int idx;
    
    if (!regs)
        GIO_DBG("%s: null pointer\n", __func__);
    memset(regs, 0x00, sizeof(*regs));
    for (idx = 0; idx < sizeof(pReg->dir)/sizeof(pReg->dir[0]); idx++)
        regs->dir[idx].val = __raw_readl(&pReg->dir[idx]);
    for (idx = 0; idx < sizeof(pReg->pullen)/sizeof(pReg->pullen[0]); idx++)
        regs->pullen[idx].val = __raw_readl(&pReg->pullen[idx]);
    for (idx = 0; idx < sizeof(pReg->pullsel)/sizeof(pReg->pullsel[0]); idx++)
        regs->pullsel[idx].val =__raw_readl(&pReg->pullsel[idx]);
    for (idx = 0; idx < sizeof(pReg->dinv)/sizeof(pReg->dinv[0]); idx++)
        regs->dinv[idx].val =__raw_readl(&pReg->dinv[idx]);
    for (idx = 0; idx < sizeof(pReg->dout)/sizeof(pReg->dout[0]); idx++)
        regs->dout[idx].val = __raw_readl(&pReg->dout[idx]);
    for (idx = 0; idx < sizeof(pReg->mode)/sizeof(pReg->mode[0]); idx++)
        regs->mode[idx].val = __raw_readl(&pReg->mode[idx]);
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL(mt_gpio_load);
/*----------------------------------------------------------------------------*/
void mt_gpio_dump(GPIO_REGS *regs) 
{
    GPIO_REGS cur;
    int idx;

    printk("%s\n", __func__);
    if (regs == NULL) { /*if arg is null, load & dump; otherwise, dump only*/
        regs = &cur;
        mt_gpio_load(regs);
        GIO_VER("dump current: %p\n", regs);
    } else {
        GIO_VER("dump %p ...\n", regs);    
    }

    GIO_VER("---# dir #-----------------------------------------------------------------\n");
    for (idx = 0; idx < sizeof(regs->dir)/sizeof(regs->dir[0]); idx++) {
        GIO_VER("0x%04X ", regs->dir[idx].val);
        if (7 == (idx % 8)) GIO_VER("\n");
    }
    GIO_VER("\n---# pullen #--------------------------------------------------------------\n");        
    for (idx = 0; idx < sizeof(regs->pullen)/sizeof(regs->pullen[0]); idx++) {
        GIO_VER("0x%04X ", regs->pullen[idx].val);    
        if (7 == (idx % 8)) GIO_VER("\n");
    }
    GIO_VER("\n---# pullsel #-------------------------------------------------------------\n");   
    for (idx = 0; idx < sizeof(regs->pullsel)/sizeof(regs->pullsel[0]); idx++) {
        GIO_VER("0x%04X ", regs->pullsel[idx].val);     
        if (7 == (idx % 8)) GIO_VER("\n");
    }
    GIO_VER("\n---# dinv #----------------------------------------------------------------\n");   
    for (idx = 0; idx < sizeof(regs->dinv)/sizeof(regs->dinv[0]); idx++) {
        GIO_VER("0x%04X ", regs->dinv[idx].val);     
        if (7 == (idx % 8)) GIO_VER("\n");
    }
    GIO_VER("\n---# dout #----------------------------------------------------------------\n");   
    for (idx = 0; idx < sizeof(regs->dout)/sizeof(regs->dout[0]); idx++) {
        GIO_VER("0x%04X ", regs->dout[idx].val);     
        if (7 == (idx % 8)) GIO_VER("\n");
    }
    GIO_VER("\n---# mode #----------------------------------------------------------------\n");   
    for (idx = 0; idx < sizeof(regs->mode)/sizeof(regs->mode[0]); idx++) {
        GIO_VER("0x%04X ", regs->mode[idx].val);     
        if (7 == (idx % 8)) GIO_VER("\n");
    }    
    GIO_VER("\n---------------------------------------------------------------------------\n");    
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL(mt_gpio_dump);
/*----------------------------------------------------------------------------*/
void mt_gpio_checkpoint_save(void)
{
#if defined(GPIO_INIT_DEBUG)    
    GPIO_REGS *pReg = (GPIO_REGS*)(GPIO_BASE);
    GPIO_REGS *cur = &saved;
    int idx;
    
    memset(cur, 0x00, sizeof(*cur));
    for (idx = 0; idx < sizeof(pReg->dir)/sizeof(pReg->dir[0]); idx++)
        cur->dir[idx].val = __raw_readl(&pReg->dir[idx]);
    for (idx = 0; idx < sizeof(pReg->pullen)/sizeof(pReg->pullen[0]); idx++)
        cur->pullen[idx].val = __raw_readl(&pReg->pullen[idx]);
    for (idx = 0; idx < sizeof(pReg->pullsel)/sizeof(pReg->pullsel[0]); idx++)
        cur->pullsel[idx].val =__raw_readl(&pReg->pullsel[idx]);
    for (idx = 0; idx < sizeof(pReg->dinv)/sizeof(pReg->dinv[0]); idx++)
        cur->dinv[idx].val =__raw_readl(&pReg->dinv[idx]);
    for (idx = 0; idx < sizeof(pReg->dout)/sizeof(pReg->dout[0]); idx++)
        cur->dout[idx].val = __raw_readl(&pReg->dout[idx]);
    for (idx = 0; idx < sizeof(pReg->mode)/sizeof(pReg->mode[0]); idx++)
        cur->mode[idx].val = __raw_readl(&pReg->mode[idx]);    
#endif     
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL(mt_gpio_checkpoint_save);
/*----------------------------------------------------------------------------*/
void mt_gpio_dump_diff(GPIO_REGS* pre, GPIO_REGS* cur)
{
#if defined(GPIO_INIT_DEBUG)        
    GPIO_REGS *pReg = (GPIO_REGS*)(GPIO_BASE);
    int idx;
    unsigned char* p = (unsigned char*)pre;
    unsigned char* q = (unsigned char*)cur;
    
    GIO_VER("------ dumping difference between %p and %p ------\n", pre, cur);
    for (idx = 0; idx < sizeof(pReg->dir)/sizeof(pReg->dir[0]); idx++) {
        if (pre->dir[idx].val != cur->dir[idx].val)
            GIO_VER("diff: dir[%2d]    : 0x%08X <=> 0x%08X\n", idx, pre->dir[idx].val, cur->dir[idx].val);
    }
    for (idx = 0; idx < sizeof(pReg->pullen)/sizeof(pReg->pullen[0]); idx++) {
        if (pre->pullen[idx].val != cur->pullen[idx].val)
            GIO_VER("diff: pullen[%2d] : 0x%08X <=> 0x%08X\n", idx, pre->pullen[idx].val, cur->pullen[idx].val);
    }
    for (idx = 0; idx < sizeof(pReg->pullsel)/sizeof(pReg->pullsel[0]); idx++) {
        if (pre->pullsel[idx].val != cur->pullsel[idx].val)
            GIO_VER("diff: pullsel[%2d]: 0x%08X <=> 0x%08X\n", idx, pre->pullsel[idx].val, cur->pullsel[idx].val);
    }
    for (idx = 0; idx < sizeof(pReg->dinv)/sizeof(pReg->dinv[0]); idx++) {
        if (pre->dinv[idx].val != cur->dinv[idx].val)
            GIO_VER("diff: dinv[%2d]   : 0x%08X <=> 0x%08X\n", idx, pre->dinv[idx].val, cur->dinv[idx].val);
    }
    for (idx = 0; idx < sizeof(pReg->dout)/sizeof(pReg->dout[0]); idx++) {
        if (pre->dout[idx].val != cur->dout[idx].val)
            GIO_VER("diff: dout[%2d]   : 0x%08X <=> 0x%08X\n", idx, pre->dout[idx].val, cur->dout[idx].val);
    }
    for (idx = 0; idx < sizeof(pReg->mode)/sizeof(pReg->mode[0]); idx++) {
        if (pre->mode[idx].val != cur->mode[idx].val)
            GIO_VER("diff: mode[%2d]   : 0x%08X <=> 0x%08X\n", idx, pre->mode[idx].val, cur->mode[idx].val);
    }
    
    for (idx = 0; idx < sizeof(*pre); idx++) {
        if (p[idx] != q[idx])
            GIO_VER("diff: raw[%2d]: 0x%02X <=> 0x%02X\n", idx, p[idx], q[idx]);
    }
    GIO_VER("memcmp(%p, %p, %d) = %d\n", p, q, sizeof(*pre), memcmp(p, q, sizeof(*pre)));
    GIO_VER("------ dumping difference end --------------------------------\n");
#endif 
}
/*----------------------------------------------------------------------------*/
void mt_gpio_checkpoint_compare(void)
{
#if defined(GPIO_INIT_DEBUG)        
    GPIO_REGS *pReg = (GPIO_REGS*)(GPIO_BASE);
    GPIO_REGS latest;
    GPIO_REGS *cur = &latest;
    int idx;
    
    memset(cur, 0x00, sizeof(*cur));
    for (idx = 0; idx < sizeof(pReg->dir)/sizeof(pReg->dir[0]); idx++)
        cur->dir[idx].val = __raw_readl(&pReg->dir[idx]);
    for (idx = 0; idx < sizeof(pReg->pullen)/sizeof(pReg->pullen[0]); idx++)
        cur->pullen[idx].val = __raw_readl(&pReg->pullen[idx]);
    for (idx = 0; idx < sizeof(pReg->pullsel)/sizeof(pReg->pullsel[0]); idx++)
        cur->pullsel[idx].val =__raw_readl(&pReg->pullsel[idx]);
    for (idx = 0; idx < sizeof(pReg->dinv)/sizeof(pReg->dinv[0]); idx++)
        cur->dinv[idx].val =__raw_readl(&pReg->dinv[idx]);
    for (idx = 0; idx < sizeof(pReg->dout)/sizeof(pReg->dout[0]); idx++)
        cur->dout[idx].val = __raw_readl(&pReg->dout[idx]);
    for (idx = 0; idx < sizeof(pReg->mode)/sizeof(pReg->mode[0]); idx++)
        cur->mode[idx].val = __raw_readl(&pReg->mode[idx]);    
 
    //mt_gpio_dump_diff(&latest, &saved);
    //GIO_DBG("memcmp(%p, %p, %d) = %d\n", &latest, &saved, sizeof(GPIO_REGS), memcmp(&latest, &saved, sizeof(GPIO_REGS)));
    if (memcmp(&latest, &saved, sizeof(GPIO_REGS))) {
        GIO_DBG("checkpoint compare fail!!\n");
        GIO_DBG("dump checkpoint....\n");
        mt_gpio_dump(&saved);
        GIO_DBG("\n\n");
        GIO_DBG("dump current state\n");
        mt_gpio_dump(&latest);
        GIO_DBG("\n\n");
        mt_gpio_dump_diff(&saved, &latest);        
        WARN_ON(1);
    } else {
        GIO_DBG("checkpoint compare success!!\n");
    }
#endif     
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL(mt_gpio_checkpoint_compare);
/*----------------------------------------------------------------------------*/
