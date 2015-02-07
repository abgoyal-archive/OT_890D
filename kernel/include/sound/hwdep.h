
#ifndef __SOUND_HWDEP_H
#define __SOUND_HWDEP_H


#include <sound/asound.h>
#include <linux/poll.h>

struct snd_hwdep;

struct snd_hwdep_ops {
	long long (*llseek) (struct snd_hwdep *hw, struct file * file, long long offset, int orig);
	long (*read) (struct snd_hwdep *hw, char __user *buf, long count, loff_t *offset);
	long (*write) (struct snd_hwdep *hw, const char __user *buf, long count, loff_t *offset);
	int (*open) (struct snd_hwdep * hw, struct file * file);
	int (*release) (struct snd_hwdep *hw, struct file * file);
	unsigned int (*poll) (struct snd_hwdep *hw, struct file * file, poll_table * wait);
	int (*ioctl) (struct snd_hwdep *hw, struct file * file, unsigned int cmd, unsigned long arg);
	int (*ioctl_compat) (struct snd_hwdep *hw, struct file * file, unsigned int cmd, unsigned long arg);
	int (*mmap) (struct snd_hwdep *hw, struct file * file, struct vm_area_struct * vma);
	int (*dsp_status) (struct snd_hwdep *hw, struct snd_hwdep_dsp_status *status);
	int (*dsp_load) (struct snd_hwdep *hw, struct snd_hwdep_dsp_image *image);
};

struct snd_hwdep {
	struct snd_card *card;
	struct list_head list;
	int device;
	char id[32];
	char name[80];
	int iface;

#ifdef CONFIG_SND_OSSEMUL
	char oss_dev[32];
	int oss_type;
	int ossreg;
#endif

	struct snd_hwdep_ops ops;
	wait_queue_head_t open_wait;
	void *private_data;
	void (*private_free) (struct snd_hwdep *hwdep);

	struct mutex open_mutex;
	int used;
	unsigned int dsp_loaded;
	unsigned int exclusive: 1;
};

extern int snd_hwdep_new(struct snd_card *card, char *id, int device,
			 struct snd_hwdep **rhwdep);

#endif /* __SOUND_HWDEP_H */
