

#ifndef SPI_IMX_H_
#define SPI_IMX_H_


/*-------------------------------------------------------------------------*/
struct spi_imx_master {
	u8 num_chipselect;
	u8 enable_dma:1;
};
/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/
struct spi_imx_chip {
	u8	enable_loopback:1;
	u8	enable_dma:1;
	u8	ins_ss_pulse:1;
	u16	bclk_wait:15;
	void (*cs_control)(u32 control);
};

/* Chip-select state */
#define SPI_CS_ASSERT			(1 << 0)
#define SPI_CS_DEASSERT			(1 << 1)
/*-------------------------------------------------------------------------*/


#endif /* SPI_IMX_H_*/
