
#if 0
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/i2c.h>
#include <linux/spinlock.h>
#include <mach/mt6516_typedefs.h>
#include <linux/interrupt.h>
#include <linux/list.h>
//#include "matv6326_hw.h"

//#include "matv6326_sw.h"
//#include "mt5192MATV_sw.h" // 20100319
#include <mach/mt6516_gpio.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#include <cust_eint.h>

#include <linux/delay.h>
#include <linux/platform_device.h>
#else
#include "matv.h"
#endif

//#define MATV_DEVNAME "MT6516_MATV"
//#define TEST_MATV_PRINT 0
//#define MATV_READ 1
//#define MATV_WRITE 2
#if 0
#define GPIO_MATV_PWR_ENABLE GPIO124
#define GPIO_MATV_N_RST      GPIO125
#define MATV_I2C_CHANNEL     (2)        //I2C Channel 2
#else
#define GPIO_MATV_PWR_ENABLE GPIO126
#define GPIO_MATV_N_RST      GPIO127
#define MATV_I2C_CHANNEL     (0)        //I2C Channel 1
#endif
#if 1
#define MATV_LOGD printk
#else
#define MATV_LOGD(...)
#endif
#if 1
#define MATV_LOGE printk
#else
#define MATV_LOGE(...)
#endif


static struct class *matv_class = NULL;
static int matv_major = 0;
static dev_t matv_devno;
static struct cdev *matv_cdev;
static spinlock_t g_mATVLock;


int matv_in_data[2] = {1,1};
int matv_out_data[2] = {1,1};
int matv_lcdbk_data[1] = {1};


#define mt5192_SLAVE_ADDR_WRITE	0x82
#define mt5192_SLAVE_ADDR_Read	0x83
#define mt5192_SLAVE_ADDR_FW_Update 0xfa

/* Addresses to scan */
static unsigned short normal_i2c[] = { mt5192_SLAVE_ADDR_WRITE,  I2C_CLIENT_END };
static unsigned short ignore = I2C_CLIENT_END;

static struct i2c_client_address_data addr_data = {
	.normal_i2c = normal_i2c,
	.probe	= &ignore,
	.ignore	= &ignore,
};

static struct i2c_client *new_client = NULL;

static int mt5192_attach_adapter(struct i2c_adapter *adapter);
static int mt5192_detect(struct i2c_adapter *adapter, int address, int kind);
static int mt5192_detach_client(struct i2c_client *client);

/* This is the driver that will be inserted */
static struct i2c_driver mt5192_driver = {
	.attach_adapter	= mt5192_attach_adapter,
	.detach_client	= mt5192_detach_client,
	.driver = 	{
	    .name		    = "mt5192",
	},
};


ssize_t mt5192_read_byte(u8 cmd, u8 *returnData)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int     ret=0;

    cmd_buf[0] = cmd;
    ret = i2c_master_send(new_client, &cmd_buf[0], 1);
    if (ret < 0) {
        MATV_LOGE("[Error]MATV sends command error!! \n");
        return 0;
    }
    ret = i2c_master_recv(new_client, &readData, 1);
    if (ret < 0) {
        MATV_LOGE("[Error]MATV reads data error!! \n");
        return 0;
    } 
    //MATV_LOGD("func mt5192_read_byte : 0x%x \n", readData);
    *returnData = readData;
    
    return 1;
}

ssize_t mt5192_write_byte(u8 cmd, u8 writeData)
{
    char    write_data[2] = {0};
    int    ret=0;
    
    write_data[0] = cmd;         // ex. 0x81
    write_data[1] = writeData;// ex. 0x44
    
    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0) {
        MATV_LOGE("[Error]sends command error!! \n");
        return 0;
    }
    
    return 1;
}

ssize_t mt5192_read_m_byte(u8 cmd, u8 *returnData,U16 len, u8 bAutoInc)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int     ret=0;
    if(len == 0) {
        MATV_LOGE("[Error]MATV Read Len should not be zero!! \n");
        return 0;
    }
        

    cmd_buf[0] = cmd;
    ret = i2c_master_send(new_client, &cmd_buf[0], 1);
    //MATV_LOGD("[MATV_R]I2C send Size = %d\n",ret);
    if (ret < 0) {
        MATV_LOGE("[Error]MATV sends command error!! \n");
        return 0;
    }
    if ((bAutoInc>0)&(len>=256)) {
        MATV_LOGE("[Error][MATV]Exceeds Maximum read size 256!\n");
        return 0;
    }

    //Driver does not allow len>8
    while(len > 8)
    {
        //MATV_LOGD("[MATV]Remain size = %d\n",len);
        ret = i2c_master_recv(new_client, returnData, 8);
        //MATV_LOGD("[MATV_R]I2C recv Size = %d\n",ret);
        if (ret < 0) {
            MATV_LOGE("[Error]MATV reads data error!! \n");
            return 0;
        }
        returnData+=8;
        len -= 8;
        if (bAutoInc){
            cmd_buf[0] = cmd_buf[0]+8;
        }
        ret = i2c_master_send(new_client, &cmd_buf[0], 1);
        //MATV_LOGD("[MATV_R]I2C send Size = %d\n",ret);
        if (ret < 0) {
            MATV_LOGE("[Error]MATV sends command error!! \n");
            return 0;
        }           
    }
    if (len > 0){
        ret = i2c_master_recv(new_client, returnData, len);
        //MATV_LOGD("[MATV]I2C Read Size = %d\n",ret);
        if (ret < 0) {
            MATV_LOGE("[Error]MATV reads data error!! \n");
            return 0;
        }
    }

        
    return 1;
}

ssize_t mt5192_write_m_byte(u8 cmd, u8 *writeData,U16 len, u8 bAutoInc)
{
    char    write_data[8] = {0};
    int    i,ret=0;

    if(len == 0) {
        MATV_LOGE("[Error]MATV Write Len should not be zero!! \n");
        return 0;
    }
        
    write_data[0] = cmd;

    //Driver does not allow (single write length > 8)
    while(len > 7)
    {
        for (i = 0; i<7; i++){
            write_data[i+1] = *(writeData+i);    
        }
        ret = i2c_master_send(new_client, write_data, 7+1);
        //MATV_LOGD("[MATV_R]I2C recv Size = %d\n",ret);
        if (ret < 0) {
            MATV_LOGE("[Error]MATV reads data error!! \n");
            return 0;
        }
        writeData+=7;
        len -= 7;
        if (bAutoInc){
            write_data[0] = write_data[0]+7;
        }   
    }
    if (len > 0){
        for (i = 0; i<len; i++){
            write_data[i+1] = *(writeData+i);    
        }
        ret = i2c_master_send(new_client, write_data, len+1);
        //MATV_LOGD("[MATV]I2C Read Size = %d\n",ret);
        if (ret < 0) {
            MATV_LOGE("[Error]MATV reads data error!! \n");
            return 0;
        }
    }

    return 1;
}


void matv_driver_init(void)
{
    /* Get MATV6326 ECO version */
    kal_uint16 eco_version = 0;
    kal_uint8 tmp8;
    kal_bool result_tmp;
    MATV_LOGD("******** mt5912 matv init\n");

    //PWR Enable
    mt_set_gpio_mode(GPIO_MATV_PWR_ENABLE,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_MATV_PWR_ENABLE, GPIO_DIR_OUT);
    mt_set_gpio_pull_enable(GPIO_MATV_PWR_ENABLE,true);
    mt_set_gpio_out(GPIO_MATV_PWR_ENABLE, GPIO_DATA_OUT_DEFAULT);

    //n_Reset
    mt_set_gpio_mode(GPIO_MATV_N_RST,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_MATV_N_RST, GPIO_DIR_OUT);
    mt_set_gpio_pull_enable(GPIO_MATV_N_RST,true);
    mt_set_gpio_out(GPIO_MATV_N_RST, GPIO_DATA_OUT_DEFAULT);

    //init spin lock
    spin_lock_init(&g_mATVLock);

}

static int matv_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int *user_data_addr;
	int ret = 0;
	U8 *pReadData = 0;
  	U8 *pWriteData = 0;
    U8 *ptr;
    U8 reg8, bAutoInc;
    U16 len;

    switch(cmd)
    {
        case TEST_MATV_PRINT :
			MATV_LOGD("**** mt5192 matv ioctl : test\n");
            break;
		
		case MATV_READ:			

            user_data_addr = (u8 *)arg;
            ret = copy_from_user(matv_in_data, user_data_addr, 4);
            ptr = (U8*)matv_in_data;
            reg8 = ptr[0];
            bAutoInc = ptr[1];
            len = ptr[2];
            len+= ((U16)ptr[3])<<8;
            pReadData = (U8 *)kmalloc(len,GFP_ATOMIC);
            if(!pReadData){
                MATV_LOGE("[Error] kmalloc failed!\n");
                break;
            }
            mt5192_read_m_byte(reg8, pReadData, len, bAutoInc);
            ret = copy_to_user(user_data_addr, pReadData, len);
            if(pReadData)
                kfree(pReadData);
     
            break;	
			
		case MATV_WRITE:			

            user_data_addr = (u8 *)arg;
            ret = copy_from_user(matv_in_data, user_data_addr, 4);
            ptr = (U8*)matv_in_data;
            reg8 = ptr[0];
            bAutoInc = ptr[1];
            len = ptr[2];
            len+= ((U16)ptr[3])<<8;
            pWriteData = (U8 *)kmalloc(len,GFP_ATOMIC);
            if(!pWriteData){
                MATV_LOGE("[Error] kmalloc failed!\n");
                break;
            }
            ret = copy_from_user(pWriteData, ((void*)user_data_addr)+4, len);
            //printk("\n[MATV]Write data = %d\n",*pWriteData);
            mt5192_write_m_byte(reg8, pWriteData, len, bAutoInc);
            //ret = copy_to_user(user_data_addr, pReadData, len);
            if(pWriteData)
                kfree(pWriteData);

            break;
        case MATV_SET_PWR:
			user_data_addr = (int *)arg;
			ret = copy_from_user(matv_in_data, user_data_addr, sizeof(int));
            if(matv_in_data[0]!=0)
                mt_set_gpio_out(GPIO_MATV_PWR_ENABLE, GPIO_OUT_ONE);
            else
                mt_set_gpio_out(GPIO_MATV_PWR_ENABLE, GPIO_OUT_ZERO);
            break;
        case MATV_SET_RST:
			user_data_addr = (int *)arg;
			ret = copy_from_user(matv_in_data, user_data_addr, sizeof(int));
            if(matv_in_data[0]!=0)
                mt_set_gpio_out(GPIO_MATV_N_RST, GPIO_OUT_ONE);
            else
                mt_set_gpio_out(GPIO_MATV_N_RST, GPIO_OUT_ZERO);            
            break;
        default:
            break;
    }

    return 0;
}

static int matv_open(struct inode *inode, struct file *file)
{ 
    MATV_LOGD("******** mt5912 matv open\n");
    spin_lock(&g_mATVLock);
    if(TRUE != hwPowerOn(MT6516_POWER_VCAM_A,VOL_2800,"MT5192"))
    {
        MATV_LOGE("[MATV] Fail to enable analog gain\n");
        return -EIO;
    }
     spin_unlock(&g_mATVLock);
   return 0;
}

static int matv_release(struct inode *inode, struct file *file)
{
    MATV_LOGD("******** mt5912 matv release\n");
    spin_lock(&g_mATVLock);
    mt_set_gpio_out(GPIO_MATV_PWR_ENABLE, GPIO_OUT_ZERO);
    mt_set_gpio_out(GPIO_MATV_N_RST, GPIO_OUT_ZERO);    
    hwPowerDown(MT6516_POWER_VCAM_A,"MT5192");
    spin_unlock(&g_mATVLock);
    return 0;
}

static struct file_operations matv_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= matv_ioctl,
	.open		= matv_open,
	.release	= matv_release,	
};

/* This function is called by i2c_detect */
int mt5192_detect(struct i2c_adapter *adapter, int address, int kind)
{
	struct class_device *class_dev = NULL;
    int err=0;
	int ret=0;

    //MATV_LOGD("[mATV]mt5192_detect !!\n ");

	/* Integrate with META TOOL : START */
	ret = alloc_chrdev_region(&matv_devno, 0, 1, MATV_DEVNAME);
	if (ret) 
		MATV_LOGE("Error: Can't Get Major number for matv \n");
	matv_cdev = cdev_alloc();
    matv_cdev->owner = THIS_MODULE;
    matv_cdev->ops = &matv_fops;
    ret = cdev_add(matv_cdev, matv_devno, 1);
	if(ret)
	    MATV_LOGE("matv Error: cdev_add\n");
	matv_major = MAJOR(matv_devno);
	matv_class = class_create(THIS_MODULE, MATV_DEVNAME);
    class_dev = (struct class_device *)device_create(matv_class, 
													NULL, 
													matv_devno, 
													NULL, 
													MATV_DEVNAME);
	//MATV_LOGD("MATV META Prepare : Done !!\n ");
	/* Integrate with META TOOL : END */

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        goto exit;

    if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }	
    memset(new_client, 0, sizeof(struct i2c_client));

    new_client->addr = address;
    new_client->adapter = adapter;
    new_client->driver = &mt5192_driver;
    new_client->flags = 0;
    strncpy(new_client->name, "mt5192", I2C_NAME_SIZE);

    if ((err = i2c_attach_client(new_client)))
        goto exit_kfree;

    matv_driver_init();

    return 0;

exit_kfree:
    kfree(new_client);
exit:
    return err;
}

static int mt5192_attach_adapter(struct i2c_adapter *adapter)
{
    //MATV_LOGD("[MATV] mt5192_attach_adapter (id=%d)******\n",adapter->id);
    if (adapter->id == MATV_I2C_CHANNEL)
    	return i2c_probe(adapter, &addr_data, mt5192_detect);
    return -1;
}

static int mt5192_detach_client(struct i2c_client *client)
{
	int err;

    device_destroy(matv_class, matv_devno);
    class_destroy(matv_class);
   
    cdev_del(matv_cdev);
    unregister_chrdev_region(matv_devno, 1);

	err = i2c_detach_client(client);
	if (err) {
		dev_err(&client->dev, "Client deregistration failed, client not detached.\n");
		return err;
	}

	kfree(i2c_get_clientdata(client));
	
	return 0;
}

static int matv_probe(struct platform_device *dev)
{ 
    MATV_LOGD("[MATV] probe done\n");
    return 0;
}

static int matv_remove(struct platform_device *dev)
{
    MATV_LOGD("[MATV] remove\n");
    return 0;
}

static void matv_shutdown(struct platform_device *dev)
{
    MATV_LOGD("[MATV] shutdown\n");
}

static int matv_suspend(struct platform_device *dev, pm_message_t state)
{    
    MATV_LOGD("[MATV] suspend\n");
    return 0;
}

static int matv_resume(struct platform_device *dev)
{   
    MATV_LOGD("[MATV] resume\n");
    return 0;
}

static struct platform_driver matv_driver = {
    .probe       = matv_probe,
    .remove      = matv_remove,
    .shutdown    = matv_shutdown,
    .suspend     = matv_suspend,
    .resume      = matv_resume,
    .driver      = {
    .name        = MATV_DEVNAME,
    },
};

static struct platform_device matv_device = {
    .name     = MATV_DEVNAME,
    .id       = 0,
};

static int __init mt5192_init(void)
{
    int ret;
    
    MATV_LOGD("[MATV] mt5192_init ******\n");
    if (i2c_add_driver(&mt5192_driver)){
        MATV_LOGE("[MATV][ERROR] fail to add device into i2c\n");
        ret = -ENODEV;
        return ret;
    }

    if (platform_device_register(&matv_device)){
        MATV_LOGE("[MATV][ERROR] fail to register device\n");
        ret = -ENODEV;
        return ret;
    }
    
    if (platform_driver_register(&matv_driver)){
        MATV_LOGE("[MATV][ERROR] fail to register driver\n");
        platform_device_unregister(&matv_device);
        ret = -ENODEV;
        return ret;
    }

    //MATV_LOGD("[MATV] mt5192_init done******\n");

    return 0;
}

static void __exit mt5192_exit(void)
{
    MATV_LOGD("[MATV] mt5192_exit ******\n");

    platform_driver_unregister(&matv_driver);
    platform_device_unregister(&matv_device);    
	i2c_del_driver(&mt5192_driver);
}
module_init(mt5192_init);
module_exit(mt5192_exit);
   
MODULE_LICENSE("GPL");
//MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("MediaTek MATV mt5192 Driver");
MODULE_AUTHOR("Charlie Lu<charlie.lu@mediatek.com>");

