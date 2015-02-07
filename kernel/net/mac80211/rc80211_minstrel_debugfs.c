
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/debugfs.h>
#include <linux/ieee80211.h>
#include <net/mac80211.h>
#include "rc80211_minstrel.h"

struct minstrel_stats_info {
	struct minstrel_sta_info *mi;
	char buf[4096];
	size_t len;
};

static int
minstrel_stats_open(struct inode *inode, struct file *file)
{
	struct minstrel_sta_info *mi = inode->i_private;
	struct minstrel_stats_info *ms;
	unsigned int i, tp, prob, eprob;
	char *p;

	ms = kmalloc(sizeof(*ms), GFP_KERNEL);
	if (!ms)
		return -ENOMEM;

	file->private_data = ms;
	p = ms->buf;
	p += sprintf(p, "rate     throughput  ewma prob   this prob  "
			"this succ/attempt   success    attempts\n");
	for (i = 0; i < mi->n_rates; i++) {
		struct minstrel_rate *mr = &mi->r[i];

		*(p++) = (i == mi->max_tp_rate) ? 'T' : ' ';
		*(p++) = (i == mi->max_tp_rate2) ? 't' : ' ';
		*(p++) = (i == mi->max_prob_rate) ? 'P' : ' ';
		p += sprintf(p, "%3u%s", mr->bitrate / 2,
				(mr->bitrate & 1 ? ".5" : "  "));

		tp = ((mr->cur_tp * 96) / 18000) >> 10;
		prob = mr->cur_prob / 18;
		eprob = mr->probability / 18;

		p += sprintf(p, "  %6u.%1u   %6u.%1u   %6u.%1u        "
				"%3u(%3u)   %8llu    %8llu\n",
				tp / 10, tp % 10,
				eprob / 10, eprob % 10,
				prob / 10, prob % 10,
				mr->last_success,
				mr->last_attempts,
				(unsigned long long)mr->succ_hist,
				(unsigned long long)mr->att_hist);
	}
	p += sprintf(p, "\nTotal packet count::    ideal %d      "
			"lookaround %d\n\n",
			mi->packet_count - mi->sample_count,
			mi->sample_count);
	ms->len = p - ms->buf;

	return 0;
}

static ssize_t
minstrel_stats_read(struct file *file, char __user *buf, size_t len, loff_t *o)
{
	struct minstrel_stats_info *ms;
	char *src;

	ms = file->private_data;
	src = ms->buf;

	len = min(len, ms->len);
	if (len <= *o)
		return 0;

	src += *o;
	len -= *o;
	*o += len;

	if (copy_to_user(buf, src, len))
		return -EFAULT;

	return len;
}

static int
minstrel_stats_release(struct inode *inode, struct file *file)
{
	struct minstrel_stats_info *ms = file->private_data;

	kfree(ms);

	return 0;
}

static struct file_operations minstrel_stat_fops = {
	.owner = THIS_MODULE,
	.open = minstrel_stats_open,
	.read = minstrel_stats_read,
	.release = minstrel_stats_release,
};

void
minstrel_add_sta_debugfs(void *priv, void *priv_sta, struct dentry *dir)
{
	struct minstrel_sta_info *mi = priv_sta;

	mi->dbg_stats = debugfs_create_file("rc_stats", S_IRUGO, dir, mi,
			&minstrel_stat_fops);
}

void
minstrel_remove_sta_debugfs(void *priv, void *priv_sta)
{
	struct minstrel_sta_info *mi = priv_sta;

	debugfs_remove(mi->dbg_stats);
}
