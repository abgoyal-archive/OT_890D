
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <mach/mt6516_gpt_sw.h>
#include <mach/mt6516_timer.h>
#include <linux/sampletrigger.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
MODULE_LICENSE("Dual BSD/GPL");

static int sampletrigger_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
unsigned long sampletrigger_handler(unsigned long state, int cmd);
struct sampletrigger_dev {
  struct semaphore sem;
  struct cdev cdev; 
} stdev;


struct file_operations sampletrigger_fops = {
  .owner   = THIS_MODULE,
  .ioctl   = sampletrigger_ioctl,
  .open    = NULL,
  .release = NULL,
  .read    = NULL,
  .write   = NULL,
};

static dev_t devno;
unsigned long sampletrigger_state;
static struct class *st_class = NULL;
static struct device *st_device = NULL;

void XGPTConfig(XGPT_NUM gpt_num) {
  XGPT_CONFIG config;
  XGPT_CLK_DIV clkDiv = XGPT_CLK_DIV_1;
  XGPT_Reset(XGPT3);
  config.num          = gpt_num;
  config.mode         = XGPT_FREE_RUN;
  config.clkDiv       = clkDiv;
  config.bIrqEnable   = TRUE;
  config.u4Compare    = 0;
  if (XGPT_Config(config) == FALSE) return;                    
}

struct st_parcel {
  unsigned long time;
  unsigned long id;
} parcel;

static int sampletrigger_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {

  if(cmd&0x20) {

	// get argument from user
    if(copy_from_user((void *)&parcel,(const void __user *)arg, sizeof(struct st_parcel))!=0) 
      parcel.id=666;
      
    parcel.time = sampletrigger_handler(parcel.id,cmd);

	// put sample result back to user    
    if(copy_to_user((void __user *)arg,(void *)&parcel,sizeof(struct st_parcel))!=0) 
      parcel.id=666;
      
  } else (void)sampletrigger_handler(arg,cmd);
  
  return 0;
}

unsigned long sampletrigger(int showlog, unsigned long state, int cmd) {
  return sampletrigger_handler(state,cmd|(showlog<<4));
}

/* cmd : bit 0,1,2,3 : command / 4: show log / 5: return value / others: not defined */
unsigned long sampletrigger_handler(unsigned long state, int cmd) {
  unsigned long microsecond = 0;
  sampletrigger_state = state;
  switch(cmd&0xf) {
    case 0:  /* SAMPLETRIGGER start XGPT3 */
      XGPTConfig(XGPT3);
      microsecond = 0;
      XGPT_Start(XGPT3);
      break;
    case 1:  /* SAMPLETRIGGER stop, read, restart XGPT3 */
      XGPT_Stop(XGPT3);
      microsecond = XGPT_GetCounter(XGPT3)*30;
      XGPT_Restart(XGPT3);
      break;
    case 2:  /* SAMPLETRIGGER stop, read XGPT3 */
      XGPT_Stop(XGPT3);
      microsecond = XGPT_GetCounter(XGPT3)*30;
      break;
    // Shu-Hsin: 20090910 add read function for non-stop timer       
    case 3:  /* SAMPLETRIGGER read XGPT2 */
      microsecond = hw_timer_us();
      break;
    default:
      break;
  }
  if(cmd&0x10) printk("sampletrigger: state=%lu us=%lu , ms=%lu\n",state, (unsigned long)microsecond, (unsigned long)microsecond/1000);
  return microsecond;
}

unsigned int sampletrigger_profile_returnValue(int state, int cmd, unsigned int *returnValue) {
  unsigned long count_value = 0;
  unsigned long microsecond = 0;
  sampletrigger_state = state;
  switch(cmd) {
    case 0:  /* SAMPLETRIGGER start XGPT3 */
      XGPTConfig(XGPT3);
      microsecond = 0;
      XGPT_Start(XGPT3);
      break;
    case 1:  /* SAMPLETRIGGER stop, read, restart XGPT3 */
      XGPT_Stop(XGPT3);
      count_value = XGPT_GetCounter(XGPT3);
      microsecond = count_value*30;
      XGPT_Restart(XGPT3);
      break;
    case 2:  /* SAMPLETRIGGER stop, read XGPT3 */
      XGPT_Stop(XGPT3);
      count_value = XGPT_GetCounter(XGPT3);
      microsecond = count_value*30;
      break;
    // Shu-Hsin: 20090910 add read function for non-stop timer 
    case 3:  /* SAMPLETRIGGER read XGPT2 */
      microsecond = hw_timer_us();      
    default:
      break;
  }
  *returnValue = microsecond/1000;
  //printk("sampletrigger: state=%d ms=%lu\n",state, (unsigned long)microsecond);
  return 0;
}

static int sampletrigger_proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
  char *p = page;
  int len = 0; 
  unsigned timer_1_val = 0;
  unsigned timer_2_val = 0;  

  p += sprintf(p, "\n\rSample Trigger Timer Summary\n\r" );
  p += sprintf(p, "=========================================\n\r" );    
  p += sprintf(p, "timer 1 : XGPT3 (32768Hz)\n\r" );     
  p += sprintf(p, "timer 2 : XGPT2 (13MHz)\n\r" );     
       

  p += sprintf(p, "\n\rSample Trigger Timer Status\n\r" );
  p += sprintf(p, "=========================================\n\r" );  

  if(XGPT_IsStart(XGPT3))
  {		p += sprintf(p, "timer 1 (enable):\n\r" );   
  }
  else
  {		p += sprintf(p, "timer 1 (disable):\n\r" );   
  }
  timer_1_val = XGPT_GetCounter(XGPT3);
  timer_1_val = timer_1_val*30;  
  p += sprintf(p, "        %d (us)\n\r",timer_1_val);        
  p += sprintf(p, "        %d (ms)\n\r",timer_1_val/1000);     
  p += sprintf(p, "        %d (s)\n\r",timer_1_val/1000000);      

  if(XGPT_IsStart(XGPT2))
  {		p += sprintf(p, "timer 2 (enable):\n\r" );   
  }
  else
  {		p += sprintf(p, "timer 2 (disable):\n\r" );   
  }  
  timer_2_val = get_timestamp();
  p += sprintf(p, "        %d (us)\n\r",timer_2_val);        
  p += sprintf(p, "        %d (ms)\n\r",timer_2_val/1000);     
  p += sprintf(p, "        %d (s)\n\r",timer_2_val/1000000);     
  p += sprintf(p, "\n\r" ); 


  p += sprintf(p, "\n\rSample Trigger Handler Usage\n\r" );
  p += sprintf(p, "=========================================\n\r" );    
  p += sprintf(p, "cmd 0 : reset and start 'timer 1'\n\r" );          
  p += sprintf(p, "cmd 1 : read 'timer 1'(us)\n\r" );          
  p += sprintf(p, "cmd 2 : stop and read 'timer 1'(us)\n\r" );    
  p += sprintf(p, "cmd 3 : read 'timer 2'(us) (ps: timer 2 never reset, and may overflow)\n\r" );   
  
  *start = page + off;

  len = p - page;
  if (len > off)
		len -= off;
  else
		len = 0;

  return len < count ? len  : count;    
  
}

//should release some resources when failure. but kirby is too lazy.maybe some other time.
static int __init sampletrigger_init(void) {

	
  int err;
  struct proc_dir_entry *sampleEntry;
  sampletrigger_state = 0;

  printk("[sampletrigger] initializing sample trigger module...\n");
  /*if(alloc_chrdev_region(&devno, 0, 1, "sampletrigger")<0) {
    printk(KERN_WARNING "sampletrigger: alloc_chrdev_region failed.\n");
    return -1;
  }*/
  // fixme : fixed dev no should not be allowed.. 
  devno=MKDEV(181,0);
  cdev_init(&(stdev.cdev), &sampletrigger_fops);
  stdev.cdev.owner = THIS_MODULE;
  stdev.cdev.ops   = &sampletrigger_fops;
  if((err=cdev_add(&(stdev.cdev),devno,1))!=0) {
    printk(KERN_WARNING "[sampletrigger] error %d in cdev_add\n",err);
    return -1;
  }
  st_class = class_create(THIS_MODULE, "sampletrigger");
  if (IS_ERR(st_class)) {
      int ret = PTR_ERR(st_class);
      printk(KERN_ERR "[sampletrigger] Unable to create class, err = %d\n", ret);
      return ret;        
  }

  // Shu-Hsin: 20090910 Crate proc entry at /proc/sampletrigger_stat
  sampleEntry = create_proc_read_entry("sampletrigger_stat", S_IRUGO, NULL, sampletrigger_proc, NULL);
  // 
  
  st_device = device_create(st_class, NULL, devno, NULL, "sampletrigger");    
  printk("[sampletrigger] sample trigger module initialized. major=%d, minor=%d\n",MAJOR(devno),MINOR(devno));

  return 0;

}

static void __exit sampletrigger_exit(void) {
  cdev_del(&(stdev.cdev));
  unregister_chrdev_region(devno,1);
  printk("sample trigger module removed.\n");
}

module_init(sampletrigger_init);
module_exit(sampletrigger_exit);
