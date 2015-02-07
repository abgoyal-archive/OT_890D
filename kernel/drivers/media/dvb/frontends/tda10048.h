

#ifndef TDA10048_H
#define TDA10048_H

#include <linux/dvb/frontend.h>
#include <linux/firmware.h>

struct tda10048_config {

	/* the demodulator's i2c address */
	u8 demod_address;

	/* serial/parallel output */
#define TDA10048_PARALLEL_OUTPUT 0
#define TDA10048_SERIAL_OUTPUT   1
	u8 output_mode;

#define TDA10048_BULKWRITE_200	200
#define TDA10048_BULKWRITE_50	50
	u8 fwbulkwritelen;

	/* Spectral Inversion */
#define TDA10048_INVERSION_OFF 0
#define TDA10048_INVERSION_ON  1
	u8 inversion;
};

#if defined(CONFIG_DVB_TDA10048) || \
	(defined(CONFIG_DVB_TDA10048_MODULE) && defined(MODULE))
extern struct dvb_frontend *tda10048_attach(
	const struct tda10048_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tda10048_attach(
	const struct tda10048_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif /* CONFIG_DVB_TDA10048 */

#endif /* TDA10048_H */
