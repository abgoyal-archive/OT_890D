

#ifndef __S5H1409_H__
#define __S5H1409_H__

#include <linux/dvb/frontend.h>

struct s5h1409_config {
	/* the demodulator's i2c address */
	u8 demod_address;

	/* serial/parallel output */
#define S5H1409_PARALLEL_OUTPUT 0
#define S5H1409_SERIAL_OUTPUT   1
	u8 output_mode;

	/* GPIO Setting */
#define S5H1409_GPIO_OFF 0
#define S5H1409_GPIO_ON  1
	u8 gpio;

	/* IF Freq for QAM in KHz, VSB is hardcoded to 5380 */
	u16 qam_if;

	/* Spectral Inversion */
#define S5H1409_INVERSION_OFF 0
#define S5H1409_INVERSION_ON  1
	u8 inversion;

	/* Return lock status based on tuner lock, or demod lock */
#define S5H1409_TUNERLOCKING 0
#define S5H1409_DEMODLOCKING 1
	u8 status_mode;

	/* MPEG signal timing */
#define S5H1409_MPEGTIMING_CONTINOUS_INVERTING_CLOCK       0
#define S5H1409_MPEGTIMING_CONTINOUS_NONINVERTING_CLOCK    1
#define S5H1409_MPEGTIMING_NONCONTINOUS_INVERTING_CLOCK    2
#define S5H1409_MPEGTIMING_NONCONTINOUS_NONINVERTING_CLOCK 3
	u16 mpeg_timing;
};

#if defined(CONFIG_DVB_S5H1409) || (defined(CONFIG_DVB_S5H1409_MODULE) \
	&& defined(MODULE))
extern struct dvb_frontend *s5h1409_attach(const struct s5h1409_config *config,
					   struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *s5h1409_attach(
	const struct s5h1409_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif /* CONFIG_DVB_S5H1409 */

#endif /* __S5H1409_H__ */

