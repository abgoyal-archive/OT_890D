
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>    
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/parser.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>


/* Debug message event */
#define DBG_EVT_NONE		0x00000000	/* No event */
#define DBG_EVT_PROC		0x00000001	/* Add / Delete proc related event */
#define DBG_EVT_BUILD		0x00000002	/* Build priority link list events */
#define DBG_EVT_PRIO		0x00000004	/* PRIO ioctl related event */

#define DBG_EVT_ALL			0xffffffff
 
#define DBG_EVT_MASK      	(DBG_EVT_BUILD)

#if 1
#define MSG(evt, fmt, args...) \
do {	\
	if ((DBG_EVT_##evt) & DBG_EVT_MASK) { \
		printk(fmt, ##args); \
	} \
} while(0)

#define MSG_FUNC_ENTRY(f)	MSG(FUC, "<FUN_ENT>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{}while(0)
#define MSG_FUNC_ENTRY(f)	   do{}while(0)
#endif


typedef enum
{
	PRIO_SCHED_NORMAL = 0,
	PRIO_SCHED_FIFO,
	PRIO_SCHED_RR,		
	PRIO_SCHED_UNKNOWN
}SCHED_ENUM;

#define PROC_NAME 		"priority"	// For proc entry name : /proc/priority
#define	PRIO_DEV_NAME	"priority"	// For dev entry name : /dev/priority

static int g_priority_setted = 0;	// checked for priority is setted or not !
static struct mutex lock;			// priority link list lock
static struct list_head head;		// priority link list head
static dev_t prio_devno;
static struct cdev *prio_cdev;
static struct class *prio_class = NULL;

struct priority_data
{
        struct list_head list;
        char name[20];			// thread name
        int policy;				// thread policy -> SCHED_ENUM
        int priority;			// thread priority
};

// For copy from/to user space priority_data used

typedef struct
{
	char name[20];	// thread name
	int policy;		// thread policy -> SCHED_ENUM
	int priority;   // thread priority  
}PRIORITY_DATA;

// For token partition used

enum {Opt_err, Opt_name, Opt_policy, Opt_priority};

static const match_table_t prio_tokens = {
	{Opt_name, "name=%s"},
	{Opt_policy, "policy=%s"},
	{Opt_priority, "priority=%d"},
	{Opt_err, NULL}
};


static void clean_all(struct list_head *head)
{
	struct priority_data *data;

	while (!list_empty(head)) 
	{
		data = list_entry(head->next, struct priority_data, list);
		list_del(&data->list);
		kfree(data);
	}
}


static int find_entry(char *name, struct priority_data **node)
{
    struct priority_data *entry;
    
    MSG(PROC, "[find_entry]: name - %s\n", name);
    
    mutex_lock(&lock);
    list_for_each_entry(entry, &head, list)
    {
        MSG(PROC, "[find_entry]: each name - %s\n", entry->name);
        if(strcmp(entry->name, name)==0)
        {
            MSG(PROC, "[find_entry] 1. Find out the name of %s\n", entry->name);
            *node = entry;
			MSG(PROC, "[find_entry] 2. Find out the entry addr 0x%x\n", (unsigned int) entry);
			MSG(PROC, "[find_entry] 3. Find out the node addr 0x%x\n", (unsigned int) *node);
            mutex_unlock(&lock);
            return 1;
        }
    }    
    mutex_unlock(&lock);
    return 0;
}


static void add_entry(char *name, int policy, int priority)
{
        struct priority_data *data = NULL;
		
        MSG(PROC, "[add_entry]: name ~ %s policy ~ %d priority ~ %d\n", name, policy, priority);

		if(policy == PRIO_SCHED_UNKNOWN)
		{
			MSG(PROC, "[add_entry]: The entry is unknown so don't add it !!\n");
			goto exit;
		}
	   
        if(find_entry(name, &data))
        {		
            MSG(PROC, "[add_entry]: The entry already exist %s !!\n", data->name);
			MSG(PROC, "[add_entry]: 5. Find out the data addr of 0x%x\n", (unsigned int) data);
			data->policy = policy;
            data->priority = priority;
        }
        else
        {
            mutex_lock(&lock);
			data = kmalloc(sizeof(*data), GFP_KERNEL);
			
            if (data != NULL)
                    list_add_tail(&data->list, &head);
			else
				MSG(PROC, "[add_entry]: Allocate memory error !!\n");
			
            strcpy(data->name ,name);
			MSG(PROC, "[add_entry]: 6. Find out the data addr of 0x%x\n", (unsigned int) data);

			data->policy = policy;
			data->priority = priority;
            mutex_unlock(&lock);
        }
exit:
	MSG(PROC, "[add_entry]: Exit add_entry function !!\n");		
}


static int priority_parse_file(char *buf, int user_len)
{
    substring_t args[MAX_OPT_ARGS];
	char name[20];
    char *parm, *string1, *string2;
    int option, policy, priority;

	option = policy = priority = 0;
        
    while ((parm = strsep(&buf, ", ")) != NULL) 	// separate token
    {
		int token;
		
		if (!*parm)
			continue;

		token = match_token(parm, prio_tokens, args);
		switch (token) 
		{
		case Opt_name:
			string1 = match_strdup(args);
			MSG(PROC, "1. Name = %s ", string1);
			strcpy(name, string1);
			break;
		case Opt_policy:
			string2 = match_strdup(args);
			MSG(PROC, "2. Policy = %s \n", string2);

			if(strcmp(string2, "SCHED_NORMAL")==0)
			{
				MSG(PROC, "The entry policy is SCHED_NORMAL !!\n");
				policy = PRIO_SCHED_NORMAL;
			}
			else if(strcmp(string2, "SCHED_FIFO")==0)
			{
				MSG(PROC, "The entry policy is SCHED_FIFO !!\n");
				policy = PRIO_SCHED_FIFO;
			}
			else if(strcmp(string2, "SCHED_RR")==0)
			{
				MSG(PROC, "The entry policy is SCHED_RR !!\n");
				policy = PRIO_SCHED_RR;
			}
			else
			{
				MSG(PROC, "The entry policy is SCHED_UNKNOWN !!\n");
				policy = PRIO_SCHED_UNKNOWN;
			}
			
			break;
		case Opt_priority:
			if (match_int(&args[0], &option))
			{
                MSG(PROC, "priority error !!\n");
				return 1;
			}			
			MSG(PROC, "3. priority = %d\n", option);
			priority = option;
			add_entry(name, policy, priority);
			break;
		default:
			break;
        }        
    }		    
    return 0;
}


static ssize_t prio_write(struct file *file, const char __user * user_buf,
                       size_t user_len, loff_t *offset)
{
    char *buf;
	size_t i;
	ssize_t rc, ret;
	struct priority_data *entry;
	
	if(g_priority_setted)
        clean_all(&head);

	if (*offset)
		return -EINVAL;
    		
	if (user_len > 65536)
		user_len = 65536;
	       
	buf = vmalloc (user_len + 1);
		
	if (buf == NULL)
		return -ENOMEM;
	memset(buf, 0, user_len + 1);
	
	if (copy_from_user(buf, user_buf, user_len))
	{
		rc = -EFAULT;
		goto out_free;
	}	
    buf[user_len] = '\0';   
	 
	for(i=0; i< user_len; i++)
	{
        if(buf[i]=='\n')
            buf[i] = ' ';        
	}

	ret = priority_parse_file(buf, user_len);

    mutex_lock(&lock);
    list_for_each_entry(entry, &head, list)
    {
        MSG(BUILD, "[BUILD] : name ~ %s, policy ~ %d, priority ~ %d\n", 
        entry->name, entry->policy, entry->priority);
    }    
    mutex_unlock(&lock);	
			
	if (ret)
		rc = ret;
	else
		rc = user_len;
    
    g_priority_setted = 1;
    
out_free:
	vfree (buf);
	return rc;  
}


static void *prio_seq_start(struct seq_file *s, loff_t *pos)
{
	MSG(PROC, "[Priority] Priority entry seq start !!\n");	
	mutex_lock(&lock);
    return seq_list_start(&head, *pos);
}

static void *prio_seq_next(struct seq_file *s, void *v, loff_t *pos)
{	
	return seq_list_next(v, &head, pos);
}

static void prio_seq_stop(struct seq_file *s, void *v)
{
	mutex_unlock(&lock);    
}

static int prio_seq_show(struct seq_file *s, void *v)
{   
    struct priority_data *data = list_entry(v, struct priority_data, list);

    seq_printf(s, "name: %s\t", data->name);
    seq_printf(s, "policy: %d\t", data->policy);
    seq_printf(s, "priority: %d\n", data->priority);
    return 0;
}

// proc seq operations
static struct seq_operations prio_seq_ops = 
{
	.start = prio_seq_start,
	.next = prio_seq_next,
	.stop = prio_seq_stop,
	.show = prio_seq_show
};

static int prio_open(struct inode *inode, struct file *file)
{
	MSG(PROC, "[Priority] Priority entry opened !!\n");
	return seq_open(file, &prio_seq_ops);
};

// proc file operations
static struct file_operations prio_file_ops = {
	.owner = THIS_MODULE,
	.open = prio_open,
	.read = seq_read,
	.write = prio_write,
	.llseek = seq_lseek,
	.release = seq_release
};

static int mt_prio_open(struct inode *inode, struct file *file)
{
    return nonseekable_open(inode, file);
}

static int mt_prio_release(struct inode *inode, struct file *file)
{
    return 0;
}


static int mt_prio_ioctl(struct inode *inode, struct file *file, 
	unsigned int cmd, unsigned long arg)
{
	PRIORITY_DATA data;
	struct priority_data *entry;
	bool found = false;
	
	if(copy_from_user(&data, (void *)arg, sizeof(PRIORITY_DATA)))	
	{
		MSG(PRIO, "[IOCTL] Copy from user error\n");
		return -EFAULT;
	}
  
    MSG(PRIO, "[IOCTL] [find_entry]: name - %s\n", data.name);

	data.policy = PRIO_SCHED_UNKNOWN;
	data.priority = -1;
    
    mutex_lock(&lock);
    list_for_each_entry(entry, &head, list)
    {
        MSG(PRIO, "[IOCTL] [find_entry]: each name - %s, poplicy - %d, priority - %d\n", 
        entry->name, entry->policy, entry->priority);
		
        if(strcmp(entry->name, data.name)==0)
        {
            MSG(PRIO, "[IOCTL] Find out the name of %s\n", entry->name);
            data.policy = entry->policy;
			data.priority = entry->priority;
            found = true;
			break;
        }
    }
	mutex_unlock(&lock);

	if(!found)
	{
		data.policy = PRIO_SCHED_UNKNOWN;
		data.priority = 0;
	}
	
	if(copy_to_user((int *)arg, &data, sizeof(PRIORITY_DATA)))	
	{
		MSG(PRIO, "[IOCTL] Copy to user error\n");
		return -EFAULT;
	}
	
    return 0;
}

// dev file operations
static struct file_operations mt_prio_fops = 
{
    .owner=      THIS_MODULE,
    .ioctl=      mt_prio_ioctl,
    .open=       mt_prio_open,    
    .release=    mt_prio_release,
};


int init_module(void)
{    
	struct proc_dir_entry *entry;
	struct class_device *class_dev = NULL;
	int ret;
	
	mutex_init(&lock);		// initialize the mutex lock
	INIT_LIST_HEAD(&head);	// initialize the link list
	MSG(PROC, "[Priority] Priority entry initialized !!\n");
	
	entry = create_proc_entry(PROC_NAME, 0, NULL);	// create proc entry
	
	if (!entry) 
	{	
        clean_all(&head);
        return -ENOMEM;        
	}
	entry->proc_fops = &prio_file_ops;	// set the proc entry file operations

	ret = alloc_chrdev_region(&prio_devno, 0, 1, PRIO_DEV_NAME);
	// alloc char dev region
	
    if(ret)
    {
		MSG(PRIO, "Error: Can't Get Major number for Priority Device\n");
    }

	prio_cdev = cdev_alloc();
    prio_cdev->owner = THIS_MODULE;
    prio_cdev->ops = &mt_prio_fops;

    ret = cdev_add(prio_cdev, prio_devno, 1);

	prio_class = class_create(THIS_MODULE, PRIO_DEV_NAME);
    class_dev = (struct class_device *)device_create(prio_class, 
													NULL, 
													prio_devno, 
													NULL, 
													PRIO_DEV_NAME);
	
	return 0;
}

void cleanup_module(void)
{
	remove_proc_entry(PROC_NAME, NULL);	// destroy the proc entry
	clean_all(&head);
}

module_init(init_module);
module_exit(cleanup_module);
MODULE_DESCRIPTION("priority module");
MODULE_AUTHOR("Koshi Chiu <koshi.chiu@mediatek.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("MT6516:priority module");
