

#ifndef __SOUND_HDA_BEEP_H
#define __SOUND_HDA_BEEP_H

#include "hda_codec.h"

/* beep information */
struct hda_beep {
	struct input_dev *dev;
	struct hda_codec *codec;
	char phys[32];
	int tone;
	int nid;
	int enabled;
	struct work_struct beep_work; /* scheduled task for beep event */
};

#ifdef CONFIG_SND_HDA_INPUT_BEEP
int snd_hda_attach_beep_device(struct hda_codec *codec, int nid);
void snd_hda_detach_beep_device(struct hda_codec *codec);
#else
#define snd_hda_attach_beep_device(...)
#define snd_hda_detach_beep_device(...)
#endif
#endif
