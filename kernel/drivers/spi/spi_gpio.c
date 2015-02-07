
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/spi/spi_gpio.h>



struct spi_gpio {
	struct spi_bitbang		bitbang;
	struct spi_gpio_platform_data	pdata;
	struct platform_device		*pdev;
};

/*----------------------------------------------------------------------*/


#ifndef DRIVER_NAME
#define DRIVER_NAME	"spi_gpio"

#define GENERIC_BITBANG	/* vs tight inlines */

/* all functions referencing these symbols must define pdata */
#define SPI_MISO_GPIO	((pdata)->miso)
#define SPI_MOSI_GPIO	((pdata)->mosi)
#define SPI_SCK_GPIO	((pdata)->sck)

#define SPI_N_CHIPSEL	((pdata)->num_chipselect)

#endif

/*----------------------------------------------------------------------*/

static inline const struct spi_gpio_platform_data * __pure
spi_to_pdata(const struct spi_device *spi)
{
	const struct spi_bitbang	*bang;
	const struct spi_gpio		*spi_gpio;

	bang = spi_master_get_devdata(spi->master);
	spi_gpio = container_of(bang, struct spi_gpio, bitbang);
	return &spi_gpio->pdata;
}

/* this is #defined to avoid unused-variable warnings when inlining */
#define pdata		spi_to_pdata(spi)

static inline void setsck(const struct spi_device *spi, int is_on)
{
	gpio_set_value(SPI_SCK_GPIO, is_on);
}

static inline void setmosi(const struct spi_device *spi, int is_on)
{
	gpio_set_value(SPI_MOSI_GPIO, is_on);
}

static inline int getmiso(const struct spi_device *spi)
{
	return !!gpio_get_value(SPI_MISO_GPIO);
}

#undef pdata

#define spidelay(nsecs)	do {} while (0)

#define	EXPAND_BITBANG_TXRX
#include <linux/spi/spi_bitbang.h>


static u32 spi_gpio_txrx_word_mode0(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	return bitbang_txrx_be_cpha0(spi, nsecs, 0, word, bits);
}

static u32 spi_gpio_txrx_word_mode1(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	return bitbang_txrx_be_cpha1(spi, nsecs, 0, word, bits);
}

static u32 spi_gpio_txrx_word_mode2(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	return bitbang_txrx_be_cpha0(spi, nsecs, 1, word, bits);
}

static u32 spi_gpio_txrx_word_mode3(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	return bitbang_txrx_be_cpha1(spi, nsecs, 1, word, bits);
}

/*----------------------------------------------------------------------*/

static void spi_gpio_chipselect(struct spi_device *spi, int is_active)
{
	unsigned long cs = (unsigned long) spi->controller_data;

	/* set initial clock polarity */
	if (is_active)
		setsck(spi, spi->mode & SPI_CPOL);

	/* SPI is normally active-low */
	gpio_set_value(cs, (spi->mode & SPI_CS_HIGH) ? is_active : !is_active);
}

static int spi_gpio_setup(struct spi_device *spi)
{
	unsigned long	cs = (unsigned long) spi->controller_data;
	int		status = 0;

	if (spi->bits_per_word > 32)
		return -EINVAL;

	if (!spi->controller_state) {
		status = gpio_request(cs, spi->dev.bus_id);
		if (status)
			return status;
		status = gpio_direction_output(cs, spi->mode & SPI_CS_HIGH);
	}
	if (!status)
		status = spi_bitbang_setup(spi);
	if (status) {
		if (!spi->controller_state)
			gpio_free(cs);
	}
	return status;
}

static void spi_gpio_cleanup(struct spi_device *spi)
{
	unsigned long	cs = (unsigned long) spi->controller_data;

	gpio_free(cs);
	spi_bitbang_cleanup(spi);
}

static int __init spi_gpio_alloc(unsigned pin, const char *label, bool is_in)
{
	int value;

	value = gpio_request(pin, label);
	if (value == 0) {
		if (is_in)
			value = gpio_direction_input(pin);
		else
			value = gpio_direction_output(pin, 0);
	}
	return value;
}

static int __init
spi_gpio_request(struct spi_gpio_platform_data *pdata, const char *label)
{
	int value;

	/* NOTE:  SPI_*_GPIO symbols may reference "pdata" */

	value = spi_gpio_alloc(SPI_MOSI_GPIO, label, false);
	if (value)
		goto done;

	value = spi_gpio_alloc(SPI_MISO_GPIO, label, true);
	if (value)
		goto free_mosi;

	value = spi_gpio_alloc(SPI_SCK_GPIO, label, false);
	if (value)
		goto free_miso;

	goto done;

free_miso:
	gpio_free(SPI_MISO_GPIO);
free_mosi:
	gpio_free(SPI_MOSI_GPIO);
done:
	return value;
}

static int __init spi_gpio_probe(struct platform_device *pdev)
{
	int				status;
	struct spi_master		*master;
	struct spi_gpio			*spi_gpio;
	struct spi_gpio_platform_data	*pdata;

	pdata = pdev->dev.platform_data;
#ifdef GENERIC_BITBANG
	if (!pdata || !pdata->num_chipselect)
		return -ENODEV;
#endif

	status = spi_gpio_request(pdata, dev_name(&pdev->dev));
	if (status < 0)
		return status;

	master = spi_alloc_master(&pdev->dev, sizeof *spi_gpio);
	if (!master) {
		status = -ENOMEM;
		goto gpio_free;
	}
	spi_gpio = spi_master_get_devdata(master);
	platform_set_drvdata(pdev, spi_gpio);

	spi_gpio->pdev = pdev;
	if (pdata)
		spi_gpio->pdata = *pdata;

	master->bus_num = pdev->id;
	master->num_chipselect = SPI_N_CHIPSEL;
	master->setup = spi_gpio_setup;
	master->cleanup = spi_gpio_cleanup;

	spi_gpio->bitbang.master = spi_master_get(master);
	spi_gpio->bitbang.chipselect = spi_gpio_chipselect;
	spi_gpio->bitbang.txrx_word[SPI_MODE_0] = spi_gpio_txrx_word_mode0;
	spi_gpio->bitbang.txrx_word[SPI_MODE_1] = spi_gpio_txrx_word_mode1;
	spi_gpio->bitbang.txrx_word[SPI_MODE_2] = spi_gpio_txrx_word_mode2;
	spi_gpio->bitbang.txrx_word[SPI_MODE_3] = spi_gpio_txrx_word_mode3;
	spi_gpio->bitbang.setup_transfer = spi_bitbang_setup_transfer;
	spi_gpio->bitbang.flags = SPI_CS_HIGH;

	status = spi_bitbang_start(&spi_gpio->bitbang);
	if (status < 0) {
		spi_master_put(spi_gpio->bitbang.master);
gpio_free:
		gpio_free(SPI_MISO_GPIO);
		gpio_free(SPI_MOSI_GPIO);
		gpio_free(SPI_SCK_GPIO);
		spi_master_put(master);
	}

	return status;
}

static int __exit spi_gpio_remove(struct platform_device *pdev)
{
	struct spi_gpio			*spi_gpio;
	struct spi_gpio_platform_data	*pdata;
	int				status;

	spi_gpio = platform_get_drvdata(pdev);
	pdata = pdev->dev.platform_data;

	/* stop() unregisters child devices too */
	status = spi_bitbang_stop(&spi_gpio->bitbang);
	spi_master_put(spi_gpio->bitbang.master);

	platform_set_drvdata(pdev, NULL);

	gpio_free(SPI_MISO_GPIO);
	gpio_free(SPI_MOSI_GPIO);
	gpio_free(SPI_SCK_GPIO);

	return status;
}

MODULE_ALIAS("platform:" DRIVER_NAME);

static struct platform_driver spi_gpio_driver = {
	.driver.name	= DRIVER_NAME,
	.driver.owner	= THIS_MODULE,
	.remove		= __exit_p(spi_gpio_remove),
};

static int __init spi_gpio_init(void)
{
	return platform_driver_probe(&spi_gpio_driver, spi_gpio_probe);
}
module_init(spi_gpio_init);

static void __exit spi_gpio_exit(void)
{
	platform_driver_unregister(&spi_gpio_driver);
}
module_exit(spi_gpio_exit);


MODULE_DESCRIPTION("SPI master driver using generic bitbanged GPIO ");
MODULE_AUTHOR("David Brownell");
MODULE_LICENSE("GPL");