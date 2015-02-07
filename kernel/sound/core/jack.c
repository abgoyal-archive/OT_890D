

#include <linux/input.h>
#include <sound/jack.h>
#include <sound/core.h>

static int snd_jack_dev_free(struct snd_device *device)
{
	struct snd_jack *jack = device->device_data;

	/* If the input device is registered with the input subsystem
	 * then we need to use a different deallocator. */
	if (jack->registered)
		input_unregister_device(jack->input_dev);
	else
		input_free_device(jack->input_dev);

	kfree(jack->id);
	kfree(jack);

	return 0;
}

static int snd_jack_dev_register(struct snd_device *device)
{
	struct snd_jack *jack = device->device_data;
	struct snd_card *card = device->card;
	int err;

	snprintf(jack->name, sizeof(jack->name), "%s %s",
		 card->shortname, jack->id);
	jack->input_dev->name = jack->name;

	/* Default to the sound card device. */
	if (!jack->input_dev->dev.parent)
		jack->input_dev->dev.parent = card->dev;

	err = input_register_device(jack->input_dev);
	if (err == 0)
		jack->registered = 1;

	return err;
}

int snd_jack_new(struct snd_card *card, const char *id, int type,
		 struct snd_jack **jjack)
{
	struct snd_jack *jack;
	int err;
	static struct snd_device_ops ops = {
		.dev_free = snd_jack_dev_free,
		.dev_register = snd_jack_dev_register,
	};

	jack = kzalloc(sizeof(struct snd_jack), GFP_KERNEL);
	if (jack == NULL)
		return -ENOMEM;

	jack->id = kstrdup(id, GFP_KERNEL);

	jack->input_dev = input_allocate_device();
	if (jack->input_dev == NULL) {
		err = -ENOMEM;
		goto fail_input;
	}

	jack->input_dev->phys = "ALSA";

	jack->type = type;

	if (type & SND_JACK_HEADPHONE)
		input_set_capability(jack->input_dev, EV_SW,
				     SW_HEADPHONE_INSERT);
	if (type & SND_JACK_LINEOUT)
		input_set_capability(jack->input_dev, EV_SW,
				     SW_LINEOUT_INSERT);
	if (type & SND_JACK_MICROPHONE)
		input_set_capability(jack->input_dev, EV_SW,
				     SW_MICROPHONE_INSERT);
	if (type & SND_JACK_MECHANICAL)
		input_set_capability(jack->input_dev, EV_SW,
				     SW_JACK_PHYSICAL_INSERT);

	err = snd_device_new(card, SNDRV_DEV_JACK, jack, &ops);
	if (err < 0)
		goto fail_input;

	*jjack = jack;

	return 0;

fail_input:
	input_free_device(jack->input_dev);
	kfree(jack);
	return err;
}
EXPORT_SYMBOL(snd_jack_new);

void snd_jack_set_parent(struct snd_jack *jack, struct device *parent)
{
	WARN_ON(jack->registered);

	jack->input_dev->dev.parent = parent;
}
EXPORT_SYMBOL(snd_jack_set_parent);

void snd_jack_report(struct snd_jack *jack, int status)
{
	if (!jack)
		return;

	if (jack->type & SND_JACK_HEADPHONE)
		input_report_switch(jack->input_dev, SW_HEADPHONE_INSERT,
				    status & SND_JACK_HEADPHONE);
	if (jack->type & SND_JACK_LINEOUT)
		input_report_switch(jack->input_dev, SW_LINEOUT_INSERT,
				    status & SND_JACK_LINEOUT);
	if (jack->type & SND_JACK_MICROPHONE)
		input_report_switch(jack->input_dev, SW_MICROPHONE_INSERT,
				    status & SND_JACK_MICROPHONE);
	if (jack->type & SND_JACK_MECHANICAL)
		input_report_switch(jack->input_dev, SW_JACK_PHYSICAL_INSERT,
				    status & SND_JACK_MECHANICAL);

	input_sync(jack->input_dev);
}
EXPORT_SYMBOL(snd_jack_report);

MODULE_AUTHOR("Mark Brown <broonie@opensource.wolfsonmicro.com>");
MODULE_DESCRIPTION("Jack detection support for ALSA");
MODULE_LICENSE("GPL");
