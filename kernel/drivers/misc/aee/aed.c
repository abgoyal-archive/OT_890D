
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hardirq.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/aee.h>

#include <mach/system.h>
#include <mach/mt6516_typedefs.h>
#include "aed.h"
#include "ipanic.h"
#include "mach/ccci_md.h"
#include <../../../drivers/video/mtk/disp_assert_layer.h>


#define AEDDEBUG // FIXME: should disable before check in

#ifdef AEDDEBUG
#define AED_DEBUG(f, s...) \
    do { \
        printk(KERN_ALERT "AED " f, ## s); \
    } while(0)
void msg_show(AE_Msg *msg)
{
	if (msg == NULL) {
		AED_DEBUG("EMPTY msg\n");
		return;
	}
	AED_DEBUG("cmdType=%d\n", msg->cmdType);
	AED_DEBUG("cmdId=%d\n", msg->cmdId);
	AED_DEBUG("seq=%d\n", msg->seq);
	AED_DEBUG("arg=%d\n", msg->arg);
	AED_DEBUG("len=%d\n", msg->len);
}

static struct proc_dir_entry *aed_proc_dir;
static struct proc_dir_entry *aed_proc_generate_oops_file;
static struct proc_dir_entry *aed_proc_generate_ke_file;
static struct proc_dir_entry *aed_proc_current_ke_console_file;

#else
#define AED_DEBUG(f, s...)
#define msg_show(a)
#endif

#define AED_MAJOR 199
#define AED_MINOR 0

#define AED_NR_DEVS 2

#define AED_MD_MINOR 0 // modem: /dev/aed0
#define AED_KE_MINOR 1 // kernel: /dev/aed1

#define CURRENT_KE_CONSOLE "current-ke-console"


struct aed_mdrec { // modem exception record
	int *md_log;
	int  md_log_size;
	int *md_phy;
	int  md_phy_size;
	char *msg;
};

struct aed_kerec { // TODO: kernel exception record
        char *msg;
        struct ipanic_oops *lastlog;
};

struct aed_dev {
	struct aed_mdrec  mdrec;
	wait_queue_head_t mdwait;

	struct aed_kerec  kerec;
	wait_queue_head_t kewait;
};


static unsigned int aed_poll(struct file *file, struct poll_table_struct *ptable);
static ssize_t aed_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t aed_write(struct file *filp, const char __user *buf, size_t count,
				loff_t *f_pos);
static long aed_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

void aed_ke_exception(int *log, int log_size);

/* Declare here without pulling another header */
int LouderSPKSound(kal_uint32 time);

static struct aed_dev aed_dev;


inline void msg_destroy(char **ppmsg)
{
	if (*ppmsg != NULL) {
		kfree(*ppmsg);
		*ppmsg = NULL;
	}
}

inline AE_Msg *msg_create(char **ppmsg, int extra_size)
{
	int size;

	msg_destroy(ppmsg);
	size = sizeof(AE_Msg) + extra_size;

	*ppmsg = kzalloc(size, GFP_KERNEL);
	if (*ppmsg == NULL)
		return NULL;

	((AE_Msg*)(*ppmsg))->len = extra_size;

	return (AE_Msg*)*ppmsg;
}

static void ke_gen_notavail_msg(void)
{
	AE_Msg *rep_msg;
	AED_DEBUG("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, 0);

	rep_msg->cmdType = AE_RSP;
	rep_msg->arg = AE_NOT_AVAILABLE;
	rep_msg->len = 0;
}

static void ke_gen_class_msg(void)
{
#define KE_CLASS_STR "Kernel Exception"
#define KE_CLASS_SIZE 17
	AE_Msg *rep_msg;
	char *data;

	AED_DEBUG("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, KE_CLASS_SIZE);

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_CLASS;
	rep_msg->len = KE_CLASS_SIZE;
	strncpy(data, KE_CLASS_STR, KE_CLASS_SIZE);
}

static void ke_gen_type_msg(void)
{
#define KE_TYPE_STR "PANIC"
#define KE_TYPE_SIZE 6
	AE_Msg *rep_msg;
	char *data;

	AED_DEBUG("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, KE_TYPE_SIZE);

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_TYPE;
	rep_msg->len = KE_TYPE_SIZE;
	strncpy(data, KE_TYPE_STR, KE_TYPE_SIZE);
}

static void ke_gen_detail_msg(const AE_Msg *req_msg)
{
	AE_Msg *rep_msg;
	char *data;

	AED_DEBUG("%s req_msg arg:%d\n", __func__, req_msg->arg);

	rep_msg = msg_create(&aed_dev.kerec.msg, 256);

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_DETAIL;
	rep_msg->arg = AE_PASS_BY_FILE;
	sprintf(data, "/proc/aed/%s", CURRENT_KE_CONSOLE);
	rep_msg->len = strlen(data) + 1;
}

static void ke_gen_process_msg(void)
{
	AE_Msg *rep_msg;
	char *data;

	AED_DEBUG("%s\n", __func__);
	rep_msg = msg_create(&aed_dev.kerec.msg, IPANIC_OOPS_HEADER_PROCESS_NAME_LENGTH);

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_PROCESS;

	strncpy(data, aed_dev.kerec.lastlog->header.process_path, IPANIC_OOPS_HEADER_PROCESS_NAME_LENGTH);
	/* Count into the NUL byte at end of string */
	rep_msg->len = strlen(data) + 1;
}

static void ke_gen_backtrace_msg(void)
{
	AE_Msg *rep_msg;
	char *data;

	AED_DEBUG("%s\n", __func__);
	rep_msg = msg_create(&aed_dev.kerec.msg, 4096);

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_BACKTRACE;

	strcpy(data, aed_dev.kerec.lastlog->header.backtrace);
	/* Count into the NUL byte at end of string */
	rep_msg->len = strlen(data) + 1;
}

static void ke_gen_ind_msg(void)
{
	AE_Msg *rep_msg;

	AED_DEBUG("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, 0);

	rep_msg->cmdType = AE_IND;
	rep_msg->cmdId = AE_IND_EXP_RAISED;
	rep_msg->arg = AE_KE;
	rep_msg->len = 0;
}

static void ke_destroy_log(void)
{
	AED_DEBUG("%s\n", __func__);
	msg_destroy(&aed_dev.kerec.msg);

	ipanic_oops_free(aed_dev.kerec.lastlog, 1);
	aed_dev.kerec.lastlog = NULL;
}

static int ke_log_avail(void)
{
  if (aed_dev.kerec.lastlog != NULL) {
    printk("panic log avaiable\n");
    return 1;
  }
  else
    return 0;
}

static void md_gen_notavail_msg(void)
{
	AE_Msg *rep_msg;
	AED_DEBUG("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.mdrec.msg, 0);

	rep_msg->cmdType = AE_RSP;
	rep_msg->arg = AE_NOT_AVAILABLE;
	rep_msg->len = 0;
}

static void md_gen_class_msg(void)
{
#define EX_CLASS_MD_STR "External Exception: Modem"
#define EX_CLASS_MD_SIZE 28
	AE_Msg *rep_msg;
	char *data;

	AED_DEBUG("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.mdrec.msg, EX_CLASS_MD_SIZE);

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_CLASS;
	rep_msg->len = EX_CLASS_MD_SIZE;
	strncpy(data, EX_CLASS_MD_STR, EX_CLASS_MD_SIZE);
}

static void md_gen_type_msg(void)
{
#define TYPE_STRLEN 256 // TODO: check if enough?
#define NEITHER_ASSERT_FATAL	0
#define IS_ASSERT		1
#define IS_FATAL		2
	int n = 0;
	int assert_or_fatal = NEITHER_ASSERT_FATAL;
	struct modem_assert_log *assert_log;
	struct modem_fatalerr_log *fatalerr_log;
	int type = aed_dev.mdrec.md_log[0];
	AE_Msg *rep_msg;
	char *data;

	AED_DEBUG("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.mdrec.msg, TYPE_STRLEN);
	data = (char*)rep_msg + sizeof(AE_Msg);

	switch (type) {
		case MD_EX_TYPE_INVALID:
			n = sprintf(data, "INVALID");
			break;
		case MD_EX_TYPE_UNDEF:
			n = sprintf(data, "UNDEF");
			break;
		case MD_EX_TYPE_SWI:
			n = sprintf(data, "SWI");
			break;
		case MD_EX_TYPE_PREF_ABT:
			n = sprintf(data, "PREFETCH ABORT");
			break;
		case MD_EX_TYPE_DATA_ABT:
			n = sprintf(data, "DATA ABORT");
			break;
		case MD_EX_TYPE_ASSERT:
			n = sprintf(data, "ASSERT");
			assert_or_fatal = IS_ASSERT;
			break;
		case MD_EX_TYPE_FATALERR_TASK:
			n = sprintf(data, "FATAL ERROR (TASK)");
			assert_or_fatal = IS_FATAL;
			break;
		case MD_EX_TYPE_FATALERR_BUF:
			n = sprintf(data, "FATAL ERROR (BUFF)");
			assert_or_fatal = IS_FATAL;
			break;
		case MD_EX_TYPE_LOCKUP:
			n = sprintf(data, "LOCKUP");
			break;
		case MD_EX_TYPE_ASSERT_DUMP:
			n = sprintf(data, "ASSERT");
			assert_or_fatal = IS_ASSERT;
			break;
		default:
			n = sprintf(data, "UNKNOWN TYPE");
			break;
	} 

	if (IS_ASSERT == assert_or_fatal) {
		assert_log = (struct modem_assert_log *)aed_dev.mdrec.md_log;
		n += sprintf(data + n, ", filename=%s,line=%d", assert_log->filename, assert_log->linenumber);
	}
	else if (IS_FATAL == assert_or_fatal) {
		fatalerr_log = (struct modem_fatalerr_log *)aed_dev.mdrec.md_log;
		n += sprintf(data + n, ", err1=%d,err2=%d", fatalerr_log->err_code1, fatalerr_log->err_code2);
	}

	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_TYPE;
	rep_msg->len = n + 1;
}

static void md_gen_detail_msg(void)
{
#define DETAIL_STRLEN 16384 // TODO: check if enough?
	int i, n = 0;
	AE_Msg *rep_msg;
	char *data;
	int *mem;

	AED_DEBUG("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.mdrec.msg, DETAIL_STRLEN);
	data = (char*)rep_msg + sizeof(AE_Msg);

	mem = (int*)aed_dev.mdrec.md_log;
	n += sprintf(data + n, "== MODEM EXCEPTION LOG ==\n");
	for (i = 0; i < aed_dev.mdrec.md_log_size / 4; i += 4) {
		n+=sprintf(data+n, "0x%08X 0x%08X 0x%08X 0x%08X\n", 
				mem[i], mem[i+1], mem[i+2], mem[i+3]);
	}

	mem = (int*)aed_dev.mdrec.md_phy;
	n += sprintf(data + n, "== MEM DUMP ==\n");
	for (i = 0; i < aed_dev.mdrec.md_phy_size / 4; i += 4) {
		n+=sprintf(data+n, "0x%08X 0x%08X 0x%08X 0x%08X\n", 
			mem[i], mem[i+1], mem[i+2], mem[i+3]);
	}

	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_DETAIL;
	rep_msg->arg = AE_PASS_BY_MEM;
	rep_msg->len = n+1;
}

static void md_gen_ind_msg(void)
{
	AE_Msg *rep_msg;

	AED_DEBUG("%s\n", __func__);

	rep_msg = msg_create(&aed_dev.mdrec.msg, 0);

	rep_msg->cmdType = AE_IND;
	rep_msg->cmdId = AE_IND_EXP_RAISED;
	rep_msg->arg = AE_EE;
	rep_msg->len = 0;
}

static void md_destroy_log(void)
{
	struct aed_mdrec *pmdrec = &aed_dev.mdrec;

	msg_destroy(&aed_dev.mdrec.msg);

	if (pmdrec->md_log != NULL) {
		kfree(pmdrec->md_log);
		pmdrec->md_log = NULL;
	}
	if (pmdrec->md_phy != NULL) {
		kfree(pmdrec->md_phy);
		pmdrec->md_phy = NULL;
	}
	pmdrec->md_log_size = 0;
	pmdrec->md_phy_size = 0;
}

static int md_log_avail(void)
{
	return (aed_dev.mdrec.md_log != NULL);
}

static int aed_ee_open(struct inode *inode, struct file *filp)
{
	AED_DEBUG("%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	filp->private_data = (int*)AED_MD_MINOR;
	return 0;
}

static int aed_ee_release(struct inode *inode, struct file *filp)
{
	AED_DEBUG("%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	md_destroy_log();
	return 0;
}

static int aed_ke_open(struct inode *inode, struct file *filp)
{
	AED_DEBUG("%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	filp->private_data = (int*)AED_KE_MINOR;
	/* The panic log only occur on system startup, so check it now */
	aed_ke_exception(NULL, 0);
	return 0;
}

static int aed_ke_release(struct inode *inode, struct file *filp)
{
	AED_DEBUG("%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	ke_destroy_log();
	return 0;
}

static int aed_proc_current_ke_console(char *page, char **start,
				       off_t off, int count,
				       int *eof, void *data)
{
	int len = 0;
  
	if (aed_dev.kerec.lastlog) {
		struct ipanic_oops *oops = aed_dev.kerec.lastlog;
		if (off + count > oops->console_len) {
		  return 0;
		}

		len = count;
		if (len > oops->console_len - off) {
		  len = oops->console_len - off;
		}
		memcpy(page, oops->console + off, len);
		*start = len;
		if (off + len == oops->console_len)
		  *eof = 1;
	}
	else {
		len = sprintf(page, "No current kernel exception console\n");
		*eof = 1;
	}
	return len;
}

static unsigned int aed_poll(struct file *file, struct poll_table_struct *ptable)
{
	//AED_DEBUG("%s\n", __func__);

	if (file->private_data == (int*)AED_MD_MINOR) {
	        if (md_log_avail()) {
			return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
	        }
		else {
			poll_wait(file, &aed_dev.mdwait, ptable);
		}
	}
	else if (file->private_data == (int*)AED_KE_MINOR) {
		if (ke_log_avail()) {
			return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
		  }
		else {
			poll_wait(file, &aed_dev.mdwait, ptable);
		}
	}

	return 0;
}

static ssize_t aed_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t ret = 0;
	int len;
	char *msg;

	AED_DEBUG("%s\n", __func__);

	if (filp->private_data == (int*)AED_MD_MINOR)
		msg = aed_dev.mdrec.msg;
	else if (filp->private_data == (int*)AED_KE_MINOR)
		msg = aed_dev.kerec.msg;
	else
		return 0;


	AED_DEBUG("%s\n", __func__);
	AED_DEBUG("read %d bytes from %d\n", count, (int)*f_pos);

	if (msg == NULL)
		return 0;

	msg_show((AE_Msg*)msg);

	len = ((AE_Msg*)msg)->len + sizeof(AE_Msg);

	if (*f_pos >= len) {
		ret = 0;
		goto out;
	}

	// TODO: semaphore
	if ((*f_pos + count) > len) {
		printk("read size overflow, count=%d, *f_pos=%d\n", count, (int)*f_pos);
		count = len - *f_pos;
		ret = -EFAULT;
		goto out;
	}

	if (copy_to_user(buf, msg + *f_pos, count)) {
		printk("copy_to_user failed\n");
		ret = -EFAULT;
		goto out;
	}
	*f_pos += count;
	ret = count;
out:
	return ret;
}

static ssize_t aed_write(struct file *filp, const char __user *buf, size_t count,
				loff_t *f_pos)
{
	AE_Msg msg;
	int rsize;
	int is_md;
	AED_DEBUG("%s\n", __func__);

	if (filp->private_data == (int*)AED_MD_MINOR)
		is_md = 1;
	else if (filp->private_data == (int*)AED_KE_MINOR)
		is_md = 0;
	else
		return -1;

	// recevied a new request means the previous response is unavilable
	// 1. set position to be zero
	// 2. destroy the previous response message
	*f_pos = 0;

	if (is_md)
		msg_destroy(&aed_dev.mdrec.msg);
	else
		msg_destroy(&aed_dev.kerec.msg);

	// the request must be an *AE_Msg buffer
	if (count != sizeof(AE_Msg)) {
		AED_DEBUG("ERR: aed_wirte count=%d\n", count);
		return -1;
	}

	rsize = copy_from_user(&msg, buf, count);
	if (rsize != 0) {
		printk("copy_from_user rsize=%d\n", rsize);
		return -1;
	}

	msg_show(&msg);

	if (msg.cmdType == AE_REQ) {
		if (is_md && !md_log_avail()) {
			md_gen_notavail_msg();
			return count;
		}
		else if (!is_md && !ke_log_avail()) {
			ke_gen_notavail_msg();
			return count;
		}
		switch(msg.cmdId) {
			case AE_REQ_CLASS:
				if (is_md) md_gen_class_msg();
				else ke_gen_class_msg();
				break;
			case AE_REQ_TYPE:
				if (is_md) md_gen_type_msg();
				else ke_gen_type_msg();
				break;
			case AE_REQ_DETAIL:
				if (is_md) md_gen_detail_msg();
				else ke_gen_detail_msg(&msg);
				break;
	                case AE_REQ_PROCESS:
				if (is_md) md_gen_notavail_msg();
				else ke_gen_process_msg();
				break;
			case AE_REQ_BACKTRACE:
				if (is_md) md_gen_notavail_msg();
				else ke_gen_backtrace_msg();
				break;
			default:
				if (is_md) md_gen_notavail_msg();
				else ke_gen_notavail_msg();
				
				break;
		}
	}
	else if (msg.cmdType == AE_IND) {
		switch(msg.cmdId) {
			case AE_IND_LOG_CLOSE:
				if (is_md) md_destroy_log();
				else ke_destroy_log();
				break;
			default:
				// IGNORE
				break;
		}
	}
	else if (msg.cmdType == AE_RSP) { // IGNORE
	}

	return count;
}

static long aed_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	switch (cmd) {
	case AEEIOCTL_DAL_SHOW:
	{
		struct aee_dal_show dal_show;

		if (copy_from_user(&dal_show, (struct aee_dal_show __user *)arg,
				   sizeof(struct aee_dal_show)))
			return -EFAULT;

		/* Try to prevent overrun */
		dal_show.msg[255] = 0;
		DAL_Printf("%s", dal_show.msg);
		break;
	}
	case AEEIOCTL_DAL_CLEAN:
		DAL_Clean();
		break;

	case AEEIOCTL_BEEP:
	{
#if defined (CONFIG_SOUND_MT6516)
		unsigned long duration = arg;
		/* Don't beep too long */
		if (duration > 5000)
			duration = 5000;
		LouderSPKSound(duration);
#else
		ret = -ENOSYS;
#endif
		break;
	}
	default:
		ret = -EINVAL;
	}
	return ret;
}

void aed_ke_exception(int *log, int log_size)
{
	AED_DEBUG("%s\n", __func__);

	aed_dev.kerec.lastlog = ipanic_oops_copy();
	if (aed_dev.kerec.lastlog != NULL) {
		ke_gen_ind_msg();
		wake_up(&aed_dev.kewait);
	}
}
EXPORT_SYMBOL(aed_ke_exception);

void aed_md_exception(int *log, int log_size, int *phy, int phy_size)
{
	AED_DEBUG("%s\n", __func__);
	if (aed_dev.mdrec.md_log != NULL) {
		// TODO: exception of exception?
		return;
	}
	DAL_Printf("*** EXCEPTION OCCUR ***\nClass: External Exception (Modem)\n");

	aed_dev.mdrec.md_log_size = log_size;
	aed_dev.mdrec.md_phy_size = phy_size;
	aed_dev.mdrec.md_log = (int*)kmalloc(log_size, GFP_KERNEL);
	aed_dev.mdrec.md_phy = (int*)kmalloc(phy_size, GFP_KERNEL);
	// TODO: check log_size and phy_size

	memcpy(aed_dev.mdrec.md_log, log, log_size);
	memcpy(aed_dev.mdrec.md_phy, phy, phy_size);
	md_gen_ind_msg();

	wake_up(&aed_dev.mdwait);
}
EXPORT_SYMBOL(aed_md_exception);

void aee_bug(const char *source, const char *msg);

#if defined(AEDDEBUG)
static int proc_read_generate_oops(char *page, char **start,
			     off_t off, int count,
			     int *eof, void *data)
{
	int len;

	aee_bug("AED-12345678901234567890", "Test from generate oops");
	len = sprintf(page, "Oops Generated\n");

	return len;
}

static int proc_read_generate_ke(char *page, char **start,
			     off_t off, int count,
			     int *eof, void *data)
{
	return sprintf(page, "KE Generated\n");
}

#endif


static int aed_proc_init(void) 
{
	aed_proc_dir = proc_mkdir("aed", NULL);
	if(aed_proc_dir == NULL) {
	  printk("aed proc_mkdir failed\n");
	  return -ENOMEM;
	}

	aed_proc_current_ke_console_file = create_proc_read_entry(CURRENT_KE_CONSOLE, 
								  0444, aed_proc_dir, 
								  aed_proc_current_ke_console,
								  NULL);
	if (aed_proc_current_ke_console_file == NULL) {
	  printk("aed create_proc_read_entry failed at generate-oops\n");
	  return -ENOMEM;
	}
#if defined(AEDDEBUG)
	aed_proc_generate_oops_file = create_proc_read_entry("generate-oops", 
							     0444, aed_proc_dir, 
							     proc_read_generate_oops,
							     NULL);
	if (aed_proc_generate_oops_file == NULL) {
	  printk("aed create_proc_read_entry failed at generate-oops\n");
	  return -ENOMEM;
	}

	aed_proc_generate_ke_file = create_proc_read_entry("generate-ke", 
							     0444, aed_proc_dir, 
							     proc_read_generate_ke,
							     NULL);
	if(aed_proc_generate_ke_file == NULL) {
	  printk("aed create_proc_read_entry failed at generate-ke\n");
	  return -ENOMEM;
	}
#endif
	return 0;
}

static int aed_proc_done(void)
{
#if defined(AEDDEBUG)
	remove_proc_entry("generate-oops", aed_proc_dir);
	remove_proc_entry("generate-ke", aed_proc_dir);
#endif
	remove_proc_entry("aed", NULL);
	return 0;
}

static struct file_operations aed_ee_fops = {
	.owner   = THIS_MODULE,
	.open    = aed_ee_open,
	.release = aed_ee_release,
	.poll    = aed_poll,
	.read    = aed_read,
	.write   = aed_write,
	.unlocked_ioctl   = aed_ioctl,
};

static struct file_operations aed_ke_fops = {
	.owner   = THIS_MODULE,
	.open    = aed_ke_open,
	.release = aed_ke_release,
	.poll    = aed_poll,
	.read    = aed_read,
	.write   = aed_write,
	.unlocked_ioctl   = aed_ioctl,
};

static struct miscdevice aed_ee_dev = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = "aed0",
    .fops    = &aed_ee_fops,
};

static struct miscdevice aed_ke_dev = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = "aed1",
    .fops    = &aed_ke_fops,
};

static int __init aed_init(void)
{
	int err = 0;

	err = aed_proc_init();
	if (err != 0)
		return err;

	memset(&aed_dev.mdrec, 0, sizeof(struct aed_mdrec));
	init_waitqueue_head(&aed_dev.mdwait);
	memset(&aed_dev.kerec, 0, sizeof(struct aed_kerec));
	init_waitqueue_head(&aed_dev.kewait);
	
	err = misc_register(&aed_ee_dev);
	if (unlikely(err)) {
		printk(KERN_ERR "aee: failed to register aed0(ee) device!\n");
		return err;
	}

	err = misc_register(&aed_ke_dev);
	if(unlikely(err)) {
		printk(KERN_ERR "aee: failed to register aed1(ke) device!\n");
		return err;
	}

	return err;
}

static void __exit aed_exit(void)
{
	int err;

	err = misc_deregister(&aed_ee_dev);
	if (unlikely(err))
		printk(KERN_ERR "xLog: failed to unregister aed(ee) device!\n");
	err = misc_deregister(&aed_ke_dev);
	if (unlikely(err))
		printk(KERN_ERR "xLog: failed to unregister aed(ke) device!\n");

	aed_proc_done();
}

module_init(aed_init);
module_exit(aed_exit);

MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("MediaTek AED Driver");
MODULE_AUTHOR("MediaTek Inc.");

