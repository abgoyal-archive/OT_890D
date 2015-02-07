


#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/core.h>
#include "pmac.h"

static struct pmac_keywest *keywest_ctx;


static int keywest_attach_adapter(struct i2c_adapter *adapter);
static int keywest_detach_client(struct i2c_client *client);

struct i2c_driver keywest_driver = {  
	.driver = {
		.name = "PMac Keywest Audio",
	},
	.attach_adapter = &keywest_attach_adapter,
	.detach_client = &keywest_detach_client,
};


#ifndef i2c_device_name
#define i2c_device_name(x)	((x)->name)
#endif

static int keywest_attach_adapter(struct i2c_adapter *adapter)
{
	int err;
	struct i2c_client *new_client;

	if (! keywest_ctx)
		return -EINVAL;

	if (strncmp(i2c_device_name(adapter), "mac-io", 6))
		return 0; /* ignored */

	new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (! new_client)
		return -ENOMEM;

	new_client->addr = keywest_ctx->addr;
	i2c_set_clientdata(new_client, keywest_ctx);
	new_client->adapter = adapter;
	new_client->driver = &keywest_driver;
	new_client->flags = 0;

	strcpy(i2c_device_name(new_client), keywest_ctx->name);
	keywest_ctx->client = new_client;
	
	/* Tell the i2c layer a new client has arrived */
	if (i2c_attach_client(new_client)) {
		snd_printk(KERN_ERR "tumbler: cannot attach i2c client\n");
		err = -ENODEV;
		goto __err;
	}

	return 0;

 __err:
	kfree(new_client);
	keywest_ctx->client = NULL;
	return err;
}

static int keywest_detach_client(struct i2c_client *client)
{
	if (! keywest_ctx)
		return 0;
	if (client == keywest_ctx->client)
		keywest_ctx->client = NULL;

	i2c_detach_client(client);
	kfree(client);
	return 0;
}

/* exported */
void snd_pmac_keywest_cleanup(struct pmac_keywest *i2c)
{
	if (keywest_ctx && keywest_ctx == i2c) {
		i2c_del_driver(&keywest_driver);
		keywest_ctx = NULL;
	}
}

int __init snd_pmac_tumbler_post_init(void)
{
	int err;
	
	if (!keywest_ctx || !keywest_ctx->client)
		return -ENXIO;

	if ((err = keywest_ctx->init_client(keywest_ctx)) < 0) {
		snd_printk(KERN_ERR "tumbler: %i :cannot initialize the MCS\n", err);
		return err;
	}
	return 0;
}

/* exported */
int __init snd_pmac_keywest_init(struct pmac_keywest *i2c)
{
	int err;

	if (keywest_ctx)
		return -EBUSY;

	keywest_ctx = i2c;

	if ((err = i2c_add_driver(&keywest_driver))) {
		snd_printk(KERN_ERR "cannot register keywest i2c driver\n");
		return err;
	}
	return 0;
}
