

#include "cxgb3i.h"

#define DRV_MODULE_NAME         "cxgb3i"
#define DRV_MODULE_VERSION	"1.0.1"
#define DRV_MODULE_RELDATE	"Jan. 2009"

static char version[] =
	"Chelsio S3xx iSCSI Driver " DRV_MODULE_NAME
	" v" DRV_MODULE_VERSION " (" DRV_MODULE_RELDATE ")\n";

MODULE_AUTHOR("Karen Xie <kxie@chelsio.com>");
MODULE_DESCRIPTION("Chelsio S3xx iSCSI Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_MODULE_VERSION);

static void open_s3_dev(struct t3cdev *);
static void close_s3_dev(struct t3cdev *);

static cxgb3_cpl_handler_func cxgb3i_cpl_handlers[NUM_CPL_CMDS];
static struct cxgb3_client t3c_client = {
	.name = "iscsi_cxgb3",
	.handlers = cxgb3i_cpl_handlers,
	.add = open_s3_dev,
	.remove = close_s3_dev,
};

static void open_s3_dev(struct t3cdev *t3dev)
{
	static int vers_printed;

	if (!vers_printed) {
		printk(KERN_INFO "%s", version);
		vers_printed = 1;
	}

	cxgb3i_sdev_add(t3dev, &t3c_client);
	cxgb3i_adapter_add(t3dev);
}

static void close_s3_dev(struct t3cdev *t3dev)
{
	cxgb3i_adapter_remove(t3dev);
	cxgb3i_sdev_remove(t3dev);
}

static int __init cxgb3i_init_module(void)
{
	int err;

	err = cxgb3i_sdev_init(cxgb3i_cpl_handlers);
	if (err < 0)
		return err;

	err = cxgb3i_iscsi_init();
	if (err < 0)
		return err;

	err = cxgb3i_pdu_init();
	if (err < 0)
		return err;

	cxgb3_register_client(&t3c_client);

	return 0;
}

static void __exit cxgb3i_exit_module(void)
{
	cxgb3_unregister_client(&t3c_client);
	cxgb3i_pdu_cleanup();
	cxgb3i_iscsi_cleanup();
	cxgb3i_sdev_cleanup();
}

module_init(cxgb3i_init_module);
module_exit(cxgb3i_exit_module);
