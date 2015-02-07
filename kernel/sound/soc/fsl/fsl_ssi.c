

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/immap_86xx.h>

#include "fsl_ssi.h"

#define FSLSSI_I2S_RATES (SNDRV_PCM_RATE_5512 | SNDRV_PCM_RATE_8000_192000 | \
			  SNDRV_PCM_RATE_CONTINUOUS)

#ifdef __BIG_ENDIAN
#define FSLSSI_I2S_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_BE | \
	 SNDRV_PCM_FMTBIT_S18_3BE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_3BE | SNDRV_PCM_FMTBIT_S24_BE)
#else
#define FSLSSI_I2S_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
	 SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | \
	 SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S24_LE)
#endif

struct fsl_ssi_private {
	char name[8];
	struct ccsr_ssi __iomem *ssi;
	dma_addr_t ssi_phys;
	unsigned int irq;
	struct snd_pcm_substream *first_stream;
	struct snd_pcm_substream *second_stream;
	struct device *dev;
	unsigned int playback;
	unsigned int capture;
	struct snd_soc_dai cpu_dai;
	struct device_attribute dev_attr;

	struct {
		unsigned int rfrc;
		unsigned int tfrc;
		unsigned int cmdau;
		unsigned int cmddu;
		unsigned int rxt;
		unsigned int rdr1;
		unsigned int rdr0;
		unsigned int tde1;
		unsigned int tde0;
		unsigned int roe1;
		unsigned int roe0;
		unsigned int tue1;
		unsigned int tue0;
		unsigned int tfs;
		unsigned int rfs;
		unsigned int tls;
		unsigned int rls;
		unsigned int rff1;
		unsigned int rff0;
		unsigned int tfe1;
		unsigned int tfe0;
	} stats;
};

static irqreturn_t fsl_ssi_isr(int irq, void *dev_id)
{
	struct fsl_ssi_private *ssi_private = dev_id;
	struct ccsr_ssi __iomem *ssi = ssi_private->ssi;
	irqreturn_t ret = IRQ_NONE;
	__be32 sisr;
	__be32 sisr2 = 0;

	/* We got an interrupt, so read the status register to see what we
	   were interrupted for.  We mask it with the Interrupt Enable register
	   so that we only check for events that we're interested in.
	 */
	sisr = in_be32(&ssi->sisr) & in_be32(&ssi->sier);

	if (sisr & CCSR_SSI_SISR_RFRC) {
		ssi_private->stats.rfrc++;
		sisr2 |= CCSR_SSI_SISR_RFRC;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_TFRC) {
		ssi_private->stats.tfrc++;
		sisr2 |= CCSR_SSI_SISR_TFRC;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_CMDAU) {
		ssi_private->stats.cmdau++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_CMDDU) {
		ssi_private->stats.cmddu++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_RXT) {
		ssi_private->stats.rxt++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_RDR1) {
		ssi_private->stats.rdr1++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_RDR0) {
		ssi_private->stats.rdr0++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_TDE1) {
		ssi_private->stats.tde1++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_TDE0) {
		ssi_private->stats.tde0++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_ROE1) {
		ssi_private->stats.roe1++;
		sisr2 |= CCSR_SSI_SISR_ROE1;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_ROE0) {
		ssi_private->stats.roe0++;
		sisr2 |= CCSR_SSI_SISR_ROE0;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_TUE1) {
		ssi_private->stats.tue1++;
		sisr2 |= CCSR_SSI_SISR_TUE1;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_TUE0) {
		ssi_private->stats.tue0++;
		sisr2 |= CCSR_SSI_SISR_TUE0;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_TFS) {
		ssi_private->stats.tfs++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_RFS) {
		ssi_private->stats.rfs++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_TLS) {
		ssi_private->stats.tls++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_RLS) {
		ssi_private->stats.rls++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_RFF1) {
		ssi_private->stats.rff1++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_RFF0) {
		ssi_private->stats.rff0++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_TFE1) {
		ssi_private->stats.tfe1++;
		ret = IRQ_HANDLED;
	}

	if (sisr & CCSR_SSI_SISR_TFE0) {
		ssi_private->stats.tfe0++;
		ret = IRQ_HANDLED;
	}

	/* Clear the bits that we set */
	if (sisr2)
		out_be32(&ssi->sisr, sisr2);

	return ret;
}

static int fsl_ssi_startup(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct fsl_ssi_private *ssi_private = rtd->dai->cpu_dai->private_data;

	/*
	 * If this is the first stream opened, then request the IRQ
	 * and initialize the SSI registers.
	 */
	if (!ssi_private->playback && !ssi_private->capture) {
		struct ccsr_ssi __iomem *ssi = ssi_private->ssi;
		int ret;

		ret = request_irq(ssi_private->irq, fsl_ssi_isr, 0,
				  ssi_private->name, ssi_private);
		if (ret < 0) {
			dev_err(substream->pcm->card->dev,
				"could not claim irq %u\n", ssi_private->irq);
			return ret;
		}

		/*
		 * Section 16.5 of the MPC8610 reference manual says that the
		 * SSI needs to be disabled before updating the registers we set
		 * here.
		 */
		clrbits32(&ssi->scr, CCSR_SSI_SCR_SSIEN);

		/*
		 * Program the SSI into I2S Slave Non-Network Synchronous mode.
		 * Also enable the transmit and receive FIFO.
		 *
		 * FIXME: Little-endian samples require a different shift dir
		 */
		clrsetbits_be32(&ssi->scr, CCSR_SSI_SCR_I2S_MODE_MASK,
			CCSR_SSI_SCR_TFR_CLK_DIS |
			CCSR_SSI_SCR_I2S_MODE_SLAVE | CCSR_SSI_SCR_SYN);

		out_be32(&ssi->stcr,
			 CCSR_SSI_STCR_TXBIT0 | CCSR_SSI_STCR_TFEN0 |
			 CCSR_SSI_STCR_TFSI | CCSR_SSI_STCR_TEFS |
			 CCSR_SSI_STCR_TSCKP);

		out_be32(&ssi->srcr,
			 CCSR_SSI_SRCR_RXBIT0 | CCSR_SSI_SRCR_RFEN0 |
			 CCSR_SSI_SRCR_RFSI | CCSR_SSI_SRCR_REFS |
			 CCSR_SSI_SRCR_RSCKP);

		/*
		 * The DC and PM bits are only used if the SSI is the clock
		 * master.
		 */

		/* 4. Enable the interrupts and DMA requests */
		out_be32(&ssi->sier,
			 CCSR_SSI_SIER_TFRC_EN | CCSR_SSI_SIER_TDMAE |
			 CCSR_SSI_SIER_TIE | CCSR_SSI_SIER_TUE0_EN |
			 CCSR_SSI_SIER_TUE1_EN | CCSR_SSI_SIER_RFRC_EN |
			 CCSR_SSI_SIER_RDMAE | CCSR_SSI_SIER_RIE |
			 CCSR_SSI_SIER_ROE0_EN | CCSR_SSI_SIER_ROE1_EN);

		/*
		 * Set the watermark for transmit FIFI 0 and receive FIFO 0. We
		 * don't use FIFO 1.  Since the SSI only supports stereo, the
		 * watermark should never be an odd number.
		 */
		out_be32(&ssi->sfcsr,
			 CCSR_SSI_SFCSR_TFWM0(6) | CCSR_SSI_SFCSR_RFWM0(2));

		/*
		 * We keep the SSI disabled because if we enable it, then the
		 * DMA controller will start.  It's not supposed to start until
		 * the SCR.TE (or SCR.RE) bit is set, but it does anyway.  The
		 * DMA controller will transfer one "BWC" of data (i.e. the
		 * amount of data that the MR.BWC bits are set to).  The reason
		 * this is bad is because at this point, the PCM driver has not
		 * finished initializing the DMA controller.
		 */
	}

	if (!ssi_private->first_stream)
		ssi_private->first_stream = substream;
	else {
		/* This is the second stream open, so we need to impose sample
		 * rate and maybe sample size constraints.  Note that this can
		 * cause a race condition if the second stream is opened before
		 * the first stream is fully initialized.
		 *
		 * We provide some protection by checking to make sure the first
		 * stream is initialized, but it's not perfect.  ALSA sometimes
		 * re-initializes the driver with a different sample rate or
		 * size.  If the second stream is opened before the first stream
		 * has received its final parameters, then the second stream may
		 * be constrained to the wrong sample rate or size.
		 *
		 * FIXME: This code does not handle opening and closing streams
		 * repeatedly.  If you open two streams and then close the first
		 * one, you may not be able to open another stream until you
		 * close the second one as well.
		 */
		struct snd_pcm_runtime *first_runtime =
			ssi_private->first_stream->runtime;

		if (!first_runtime->rate || !first_runtime->sample_bits) {
			dev_err(substream->pcm->card->dev,
				"set sample rate and size in %s stream first\n",
				substream->stream == SNDRV_PCM_STREAM_PLAYBACK
				? "capture" : "playback");
			return -EAGAIN;
		}

		snd_pcm_hw_constraint_minmax(substream->runtime,
			SNDRV_PCM_HW_PARAM_RATE,
			first_runtime->rate, first_runtime->rate);

		snd_pcm_hw_constraint_minmax(substream->runtime,
			SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
			first_runtime->sample_bits,
			first_runtime->sample_bits);

		ssi_private->second_stream = substream;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ssi_private->playback++;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ssi_private->capture++;

	return 0;
}

static int fsl_ssi_prepare(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct fsl_ssi_private *ssi_private = rtd->dai->cpu_dai->private_data;

	struct ccsr_ssi __iomem *ssi = ssi_private->ssi;

	if (substream == ssi_private->first_stream) {
		u32 wl;

		/* The SSI should always be disabled at this points (SSIEN=0) */
		wl = CCSR_SSI_SxCCR_WL(snd_pcm_format_width(runtime->format));

		/* In synchronous mode, the SSI uses STCCR for capture */
		clrsetbits_be32(&ssi->stccr, CCSR_SSI_SxCCR_WL_MASK, wl);
	}

	return 0;
}

static int fsl_ssi_trigger(struct snd_pcm_substream *substream, int cmd,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct fsl_ssi_private *ssi_private = rtd->dai->cpu_dai->private_data;
	struct ccsr_ssi __iomem *ssi = ssi_private->ssi;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			clrbits32(&ssi->scr, CCSR_SSI_SCR_SSIEN);
			setbits32(&ssi->scr,
				CCSR_SSI_SCR_SSIEN | CCSR_SSI_SCR_TE);
		} else {
			clrbits32(&ssi->scr, CCSR_SSI_SCR_SSIEN);
			setbits32(&ssi->scr,
				CCSR_SSI_SCR_SSIEN | CCSR_SSI_SCR_RE);

			/*
			 * I think we need this delay to allow time for the SSI
			 * to put data into its FIFO.  Without it, ALSA starts
			 * to complain about overruns.
			 */
			mdelay(1);
		}
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			clrbits32(&ssi->scr, CCSR_SSI_SCR_TE);
		else
			clrbits32(&ssi->scr, CCSR_SSI_SCR_RE);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static void fsl_ssi_shutdown(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct fsl_ssi_private *ssi_private = rtd->dai->cpu_dai->private_data;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ssi_private->playback--;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ssi_private->capture--;

	if (ssi_private->first_stream == substream)
		ssi_private->first_stream = ssi_private->second_stream;

	ssi_private->second_stream = NULL;

	/*
	 * If this is the last active substream, disable the SSI and release
	 * the IRQ.
	 */
	if (!ssi_private->playback && !ssi_private->capture) {
		struct ccsr_ssi __iomem *ssi = ssi_private->ssi;

		clrbits32(&ssi->scr, CCSR_SSI_SCR_SSIEN);

		free_irq(ssi_private->irq, ssi_private);
	}
}

static int fsl_ssi_set_sysclk(struct snd_soc_dai *cpu_dai,
			      int clk_id, unsigned int freq, int dir)
{

	return (dir == SND_SOC_CLOCK_IN) ? 0 : -EINVAL;
}

static int fsl_ssi_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int format)
{
	return (format == SND_SOC_DAIFMT_I2S) ? 0 : -EINVAL;
}

static struct snd_soc_dai fsl_ssi_dai_template = {
	.playback = {
		/* The SSI does not support monaural audio. */
		.channels_min = 2,
		.channels_max = 2,
		.rates = FSLSSI_I2S_RATES,
		.formats = FSLSSI_I2S_FORMATS,
	},
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = FSLSSI_I2S_RATES,
		.formats = FSLSSI_I2S_FORMATS,
	},
	.ops = {
		.startup = fsl_ssi_startup,
		.prepare = fsl_ssi_prepare,
		.shutdown = fsl_ssi_shutdown,
		.trigger = fsl_ssi_trigger,
		.set_sysclk = fsl_ssi_set_sysclk,
		.set_fmt = fsl_ssi_set_fmt,
	},
};

static ssize_t fsl_sysfs_ssi_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct fsl_ssi_private *ssi_private =
	container_of(attr, struct fsl_ssi_private, dev_attr);
	ssize_t length;

	length = sprintf(buf, "rfrc=%u", ssi_private->stats.rfrc);
	length += sprintf(buf + length, "\ttfrc=%u", ssi_private->stats.tfrc);
	length += sprintf(buf + length, "\tcmdau=%u", ssi_private->stats.cmdau);
	length += sprintf(buf + length, "\tcmddu=%u", ssi_private->stats.cmddu);
	length += sprintf(buf + length, "\trxt=%u", ssi_private->stats.rxt);
	length += sprintf(buf + length, "\trdr1=%u", ssi_private->stats.rdr1);
	length += sprintf(buf + length, "\trdr0=%u", ssi_private->stats.rdr0);
	length += sprintf(buf + length, "\ttde1=%u", ssi_private->stats.tde1);
	length += sprintf(buf + length, "\ttde0=%u", ssi_private->stats.tde0);
	length += sprintf(buf + length, "\troe1=%u", ssi_private->stats.roe1);
	length += sprintf(buf + length, "\troe0=%u", ssi_private->stats.roe0);
	length += sprintf(buf + length, "\ttue1=%u", ssi_private->stats.tue1);
	length += sprintf(buf + length, "\ttue0=%u", ssi_private->stats.tue0);
	length += sprintf(buf + length, "\ttfs=%u", ssi_private->stats.tfs);
	length += sprintf(buf + length, "\trfs=%u", ssi_private->stats.rfs);
	length += sprintf(buf + length, "\ttls=%u", ssi_private->stats.tls);
	length += sprintf(buf + length, "\trls=%u", ssi_private->stats.rls);
	length += sprintf(buf + length, "\trff1=%u", ssi_private->stats.rff1);
	length += sprintf(buf + length, "\trff0=%u", ssi_private->stats.rff0);
	length += sprintf(buf + length, "\ttfe1=%u", ssi_private->stats.tfe1);
	length += sprintf(buf + length, "\ttfe0=%u\n", ssi_private->stats.tfe0);

	return length;
}

struct snd_soc_dai *fsl_ssi_create_dai(struct fsl_ssi_info *ssi_info)
{
	struct snd_soc_dai *fsl_ssi_dai;
	struct fsl_ssi_private *ssi_private;
	int ret = 0;
	struct device_attribute *dev_attr;

	ssi_private = kzalloc(sizeof(struct fsl_ssi_private), GFP_KERNEL);
	if (!ssi_private) {
		dev_err(ssi_info->dev, "could not allocate DAI object\n");
		return NULL;
	}
	memcpy(&ssi_private->cpu_dai, &fsl_ssi_dai_template,
	       sizeof(struct snd_soc_dai));

	fsl_ssi_dai = &ssi_private->cpu_dai;
	dev_attr = &ssi_private->dev_attr;

	sprintf(ssi_private->name, "ssi%u", (u8) ssi_info->id);
	ssi_private->ssi = ssi_info->ssi;
	ssi_private->ssi_phys = ssi_info->ssi_phys;
	ssi_private->irq = ssi_info->irq;
	ssi_private->dev = ssi_info->dev;

	ssi_private->dev->driver_data = fsl_ssi_dai;

	/* Initialize the the device_attribute structure */
	dev_attr->attr.name = "ssi-stats";
	dev_attr->attr.mode = S_IRUGO;
	dev_attr->show = fsl_sysfs_ssi_show;

	ret = device_create_file(ssi_private->dev, dev_attr);
	if (ret) {
		dev_err(ssi_info->dev, "could not create sysfs %s file\n",
			ssi_private->dev_attr.attr.name);
		kfree(fsl_ssi_dai);
		return NULL;
	}

	fsl_ssi_dai->private_data = ssi_private;
	fsl_ssi_dai->name = ssi_private->name;
	fsl_ssi_dai->id = ssi_info->id;
	fsl_ssi_dai->dev = ssi_info->dev;

	ret = snd_soc_register_dai(fsl_ssi_dai);
	if (ret != 0) {
		dev_err(ssi_info->dev, "failed to register DAI: %d\n", ret);
		kfree(fsl_ssi_dai);
		return NULL;
	}

	return fsl_ssi_dai;
}
EXPORT_SYMBOL_GPL(fsl_ssi_create_dai);

void fsl_ssi_destroy_dai(struct snd_soc_dai *fsl_ssi_dai)
{
	struct fsl_ssi_private *ssi_private =
	container_of(fsl_ssi_dai, struct fsl_ssi_private, cpu_dai);

	device_remove_file(ssi_private->dev, &ssi_private->dev_attr);

	snd_soc_unregister_dai(&ssi_private->cpu_dai);

	kfree(ssi_private);
}
EXPORT_SYMBOL_GPL(fsl_ssi_destroy_dai);

MODULE_AUTHOR("Timur Tabi <timur@freescale.com>");
MODULE_DESCRIPTION("Freescale Synchronous Serial Interface (SSI) ASoC Driver");
MODULE_LICENSE("GPL");
