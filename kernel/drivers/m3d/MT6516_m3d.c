
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/signal.h> // SIGIO, POLL_IN
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/wait.h>
#include <linux/earlysuspend.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_typedefs.h> 
#include <mach/mt6516_reg_base.h> 
#include <mach/mt6516_mm_mem.h>
#include <mach/mt6516_timer.h>
#include <asm/cacheflush.h>	//for flush cache
#include <asm/tlbflush.h>
#include "M3DDriver.h"

#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
#include <linux/proc_fs.h>
#endif

#define M3D_DEVNAME "MT6516-M3D"
#define PROCNAME "driver/m3d"
#define TS_PRINT_TEST  0
#define TS_PRINT_ARRAY 1

#ifdef M3D_DEBUG
#define DbgPrt printk
#else
#define DbgPrt    
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#define CONFIG_HAS_EARLYSUSPEND_M3D
#endif

static dev_t m3d_devno;
static struct cdev *m3d_cdev;
static struct class *m3d_class = NULL;
#ifdef M3D_IOCTL_LOCK  
static struct semaphore g_Lock;
#endif
unsigned char *src_buffer_va;
unsigned int src_buffer_pa;
unsigned char *src1_buffer_va;
unsigned int src1_buffer_pa;
static struct fasync_struct *M3D_async = NULL;
int G_len;
int alloc_src = 0;
int alloc_src1 = 0;

//wait_queue_head_t m3dWaitQueue;
//volatile BOOL   g_m3d_interrupt_handler = FALSE;

#if 0
static irqreturn_t m3d_intr_dlr(int irq, void *dev_id)
{   

    g_m3d_interrupt_handler = TRUE;
    wake_up_interruptible(&m3dWaitQueue);

    return IRQ_HANDLED;
}
#endif

#ifdef __M3D_INTERRUPT__
static int m3d_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
#ifdef M3D_IOCTL_LOCK    
    if (down_interruptible(&g_Lock)) { /* get the lock */
        return -ERESTARTSYS;
    }
#endif

    switch(cmd)
    {
    case NEW_DCACHE_OP:
        {
            DCacheOperation kDCacheOP;
            void __user *argp = (void __user *)arg;
            if (copy_from_user(&kDCacheOP, argp, sizeof(DCacheOperation))){
                ret = -EFAULT;
            }
            if (kDCacheOP.eOP == DCACHE_OP_INVALID){
                dmac_inv_range(kDCacheOP.pStart, kDCacheOP.pStart + kDCacheOP.i4Size);
            } else if (kDCacheOP.eOP == DCACHE_OP_FLUSH){
                dmac_clean_range(kDCacheOP.pStart, kDCacheOP.pStart + kDCacheOP.i4Size);
            }
        }
        break;

    case CHECK_APP_NAME:
        {
            ProcessInfo kPInfo;
            void __user *argp = (void __user *)arg;
            kPInfo.pid = current->pid;
            strcpy(kPInfo.comm, current->comm);
            if (copy_to_user(argp, &kPInfo, sizeof(ProcessInfo))){
                ret = -EFAULT;
            }            
        }
        break;
        
    default:
        ret = M3DWorkerEntry(file, cmd, arg);
        break;
    }

#ifdef M3D_IOCTL_LOCK
    up(&g_Lock);
#endif

    return ret;
}
#else
static int m3d_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
//    int i,ret;
//    unsigned char *user_array_addr;

    spin_lock(&csM3d_lock);

    //printk("\n----MT6516 m3d_ioctl----\n");

    switch(cmd)
    {
#if 0    
        case TS_PRINT_TEST:
            printk("\n----MT6516 m3d_ioctl_TEST----\n");
            break;

        case TS_PRINT_ARRAY:
            user_array_addr = (unsigned char *)arg;
            src1_buffer_va = dma_alloc_coherent(0, 5, &src1_buffer_pa, GFP_KERNEL);
            alloc_src1 = 1;
			ret = copy_from_user(src1_buffer_va, user_array_addr, 5);            
            for(i = 0; i < 5; i++)
            {
                printk("ioctl_input_array[%d] = %x\n", i, *(src1_buffer_va+i));
            }
            break;
#endif            
        default:
            //printk("\n----MT6516 m3d_ioctl default case----\n");            
            M3DWorkerEntry(cmd, arg);
            break;
    }

    spin_unlock(&csM3d_lock);
    
    return 0;
}
#endif

static int m3d_open(struct inode *inode, struct file *file)
{
   printk("\n----MT6516 m3d_open----\n");
   M3DOpen(file);
   return 0;
}

static ssize_t m3d_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    int ret;
    
    printk("\n----MT6516 m3d_read----\n");

    ret = copy_to_user(data, src_buffer_va, len);
    if(ret != 0)
    {
        printk("error: unable to copy kernel space to user space\n");
        return ret;
    }
    
    return len;
}

static ssize_t m3d_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
    int i, ret;
    
    printk("\n----MT6516 m3d_write----\n");
    
    src_buffer_va = dma_alloc_coherent(0, len, &src_buffer_pa, GFP_KERNEL);
    alloc_src = 1;
    G_len = len;
    ret = copy_from_user(src_buffer_va, data, len);
    if(ret != 0)
    {
        printk("error: unable to copy user space to kernel space\n");
        return ret;
    }

    printk("The first %d byte in physcal buffer : \n",len);
    for(i = 0 ; i < len ; i++)
    {
        printk("%x ", *(src_buffer_va+i));
    }
    printk("\n");
    
    printk("\n----fasync begin----\n");
    if (M3D_async)
    {
        printk("\n----fasync begin_2----\n"); 
        kill_fasync(&M3D_async, SIGIO, POLL_IN);
    }
    
    return len;
}

static int m3d_fasync(int fd, struct file *file, int mode)
{
    printk("\n----MT6516 m3d_fasync----\n");
    return fasync_helper(fd, file, mode, &M3D_async);
   // return 0;
}

static int m3d_release(struct inode *inode, struct file *file)
{
    printk("\n----MT6516 m3d_release----\n");

	M3DRelease(file);

    if(alloc_src == 1)
	    dma_free_coherent(0, G_len, src_buffer_va, src_buffer_pa);
    
	if(alloc_src1 == 1)
	    dma_free_coherent(0, 5, src1_buffer_va, src1_buffer_pa);
	return m3d_fasync(-1, file, 0);
    return 0;
}

static int m3d_mmap(struct file *file, struct vm_area_struct * vma)
{
//Kevin
    remap_pfn_range(vma , vma->vm_start , vma->vm_pgoff ,
     	vma->vm_end - vma->vm_start , vma->vm_page_prot);

    return 0;
}

static struct file_operations m3d_fops = {
    .owner      = THIS_MODULE,
    .ioctl      = m3d_ioctl,
    .open       = m3d_open,
    .release	= m3d_release,
    .read       = m3d_read,
    .write      = m3d_write,
    .fasync     = m3d_fasync,
    .mmap       = m3d_mmap, 
};

static int m3d_remove(struct platform_device *dev)
{
	printk("----MT6516 M3D remove----\n");
	return 0;
}

static void m3d_shutdown(struct platform_device *dev)
{
	printk("----MT6516 M3D shutdown----\n");
}


#ifdef CONFIG_HAS_EARLYSUSPEND_M3D
static int m3d_suspend(struct early_suspend *h)
#else
static int m3d_suspend(struct platform_device *dev, pm_message_t state)
#endif
{    
    //printk("----MT6516 M3D suspend prepare----\n");
    M3DSuspend();
    printk("----MT6516 M3D suspend----\n");
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND_M3D
static int m3d_resume(struct platform_device *dev)
#else
static int m3d_resume(struct early_suspend *h)
#endif
{   
    //printk("----MT6516 M3D resume prepare----\n");
    M3DResume();
    printk("----MT6516 M3D resume----\n");
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND_M3D
static struct early_suspend MT6516_m3d_early_suspend_handler = 
{
    .level = EARLY_SUSPEND_LEVEL_DISABLE_FB+50,    
    .suspend = m3d_suspend,
    .resume  = m3d_resume,
};
#endif

static int m3d_probe(struct platform_device *dev)
{
	int ret;
       struct class_device *class_dev = NULL;
       
#if 1
#else
    int major = 250;
    int minor = 0;
    m3d_devno = MKDEV(major, minor);
#endif

    printk("\n----MT6516 M3D probe----\n");

#ifdef CONFIG_HAS_EARLYSUSPEND_M3D
    register_early_suspend(&MT6516_m3d_early_suspend_handler);
#endif

#if 1 //dynamic
	ret = alloc_chrdev_region(&m3d_devno, 0, 1, M3D_DEVNAME);
#else //static
    ret = register_chrdev_region(m3d_devno, 1, M3D_DEVNAME);
#endif

	if(ret)
	{
	    printk("Error: Can't Get Major number for m3d Device\n");
	}
	else
	    printk("Get M3D Device Major number (%d)\n", ret);

	m3d_cdev = cdev_alloc();
    m3d_cdev->owner = THIS_MODULE;
	m3d_cdev->ops = &m3d_fops;

	ret = cdev_add(m3d_cdev, m3d_devno, 1);

#if 0
    //Register Interrupt 
    if (request_irq(MT6516_M3D_IRQ_LINE, m3d_intr_dlr, 0, M3D_DEVNAME, NULL) < 0)
    {
       printk("[M3D][ERROR] error to request M3D irq\n"); 
    }
    else
    {
       printk("[M3D] success to request M3D irq\n");
    }
#endif

    m3d_class = class_create(THIS_MODULE, M3D_DEVNAME);    
    class_dev = (struct class_device *)device_create(m3d_class,
                                           	NULL,
                                           	m3d_devno,
                                           	NULL,
                                           	M3D_DEVNAME
                                           	);

    //init_waitqueue_head(&m3dWaitQueue);    

	printk("\n----MT6516 M3D probe Done----\n");
	
	return 0;
}

static struct platform_driver m3d_driver = {
    .probe		= m3d_probe,
    .remove		= m3d_remove,
    .shutdown	= m3d_shutdown,
#ifndef CONFIG_HAS_EARLYSUSPEND_M3D
    .suspend	= m3d_suspend,
    .resume		= m3d_resume,
#endif
    .driver     = {
        .name	= M3D_DEVNAME,
    },
};

static struct platform_device m3d_device = {
    .name	 = M3D_DEVNAME,
    .id      = 0,
};

#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
static int m3d_proc_read(char *page, char **start, off_t off,
    int count, int *eof, void *data)
{
    if (off > 0) {
        return 0;
    }
    return M3DProcRead();
}
//-------------------------------------------------------------------------------
static int m3d_proc_write(struct file* file, const char* buffer,
    unsigned long count, void *data)
{
    return M3DProcWrite(buffer, count);
}
#endif

static int __init m3d_device_driver_init(void)
{
    int ret;

#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
    struct proc_dir_entry *entry;
    entry = create_proc_entry(PROCNAME, 0666, NULL);
    if (entry == NULL) {
        printk(KERN_WARNING "m3d: unable to create /proc entry\n");
        return -ENOMEM;
    }
    entry->read_proc = m3d_proc_read;
    entry->write_proc = m3d_proc_write;
    entry->owner = THIS_MODULE;
#endif

    printk("----m3d_device_driver_init----\n");
    printk("----Register the M3D Device----\n");

    if (platform_device_register(&m3d_device)) 
    {
        printk("----failed to register M3D Device----\n");
        ret = -ENODEV;
        return ret;
    }

    printk("----Register the M3D Driver----\n");
    if (platform_driver_register(&m3d_driver)) 
    {
        printk("----failed to register M3D Driver----\n");
        platform_device_unregister(&m3d_device);
        ret = -ENODEV;
        return ret;
    }
#ifdef M3D_IOCTL_LOCK  
    init_MUTEX(&g_Lock);
#endif
    printk("----m3d_device_driver_init Done----\n");

    return M3dInit();	//TODO move to m3d_open
}

static void __exit m3d_device_driver_exit(void)
{
    printk("----m3d_device_driver_exit----\n");

    M3dDeinit(); //TODO move to m3d_close
#if defined(M3D_PROC_DEBUG) || defined(M3D_PROC_PROFILE)
    remove_proc_entry(PROCNAME, NULL);
#endif
    device_destroy(m3d_class, m3d_devno);    
    class_destroy(m3d_class);
    
    cdev_del(m3d_cdev);
    unregister_chrdev_region(m3d_devno, 1);
    platform_driver_register(&m3d_driver);
    platform_device_unregister(&m3d_device);

#ifdef CONFIG_HAS_EARLYSUSPEND_M3D
    unregister_early_suspend(&MT6516_m3d_early_suspend_handler);
#endif

    printk("----m3d_device_driver_exit Done\n----");
}

module_init(m3d_device_driver_init);
module_exit(m3d_device_driver_exit);
MODULE_AUTHOR("Kevin, Ho <kevin.ho@mediatek.com>");
MODULE_DESCRIPTION("MT6516 M3D Driver");
MODULE_LICENSE("GPL");
