/*
    Inputdummy is a linux kernel module that offers a virtual mouse and keyboard.
    In /dev virtual_kbd and virtual_mouse will be created. Event values sent to those
    device files will be generated on these virtual input devices.
    Copyright (C) 2014  Arkadiusz Guzinski <kermit@ag.de1.cc>, based on 
    "virtual_touchscreen" (https://github.com/vi/virtual_touchscreen) by Vitaly Shukela.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/delay.h>

#define MODNAME "inputdummy"

#define DEVICE_NAME "inputdummy"
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int Major;            /* Major number assigned to our device driver */
static int Device_Open[2] = {0,0};  /* Is device open?  Used to prevent multiple access to the device */

struct class * cl;
struct device * devK;
struct device * devM;

struct file_operations fops = {
       read: device_read,
       write: device_write,
       open: device_open,
       release: device_release
};			

static struct input_dev *vkbd_dev;
static struct input_dev *vmouse_dev;

static int __init inputdummy_init(void)
{
	int err, i;

	/* Keyboard */

	vkbd_dev = input_allocate_device();
	if (!vkbd_dev)
		return -ENOMEM;

	vkbd_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_LED);
	for (i = 0; i < BITS_TO_LONGS(KEY_CNT) ; i++)
		vkbd_dev->keybit[i] = 0xffffffffffffffff;
	vkbd_dev->ledbit[0] = LED_NUML | LED_CAPSL | LED_SCROLLL;
	
	vkbd_dev->name = "Virtual Keyboard Device";
	vkbd_dev->phys = "inputdummy/input0";

	err = input_register_device(vkbd_dev);
    printk ("inputdummy: registered=%d\n", err);
	if (err)
		goto fail1;

	/* Mouse */

	vmouse_dev = input_allocate_device();
	if (!vmouse_dev)
		return -ENOMEM;

	vmouse_dev->evbit[0] = BIT_MASK(EV_REL) | BIT_MASK(EV_KEY);
	vmouse_dev->keybit[BIT_WORD(BTN_MOUSE)] =	BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE) |
												BIT_MASK(BTN_SIDE) | BIT_MASK(BTN_EXTRA) | BIT_MASK(BTN_FORWARD) |
												BIT_MASK(BTN_BACK) | BIT_MASK(BTN_TASK);
//	vmouse_dev->relbit[0] = REL_X | REL_Y | REL_HWHEEL | REL_WHEEL;
	vmouse_dev->relbit[0] = 0xfff;
	//vmouse_dev->relbit[BIT_WORD(REL_HWHEEL)] = BIT_MASK(REL_HWHEEL) | BIT_MASK(REL_WHEEL);
	
	vmouse_dev->name = "Virtual Mouse Device";
	vmouse_dev->phys = "inputdummy/input1";

	err = input_register_device(vmouse_dev);
    printk ("inputdummy: registered=%d\n", err);
	if (err)
		goto fail2;


    /* Above is evdev part. Below is character device part */

    Major = register_chrdev(0, "virtual_kbd", &fops);	
    if (Major < 0) {
	printk ("Registering the character device failed with %d\n", Major);
	    goto fail1;
    }
    printk ("inputdummy: Major=%d\n", Major);

    cl = class_create(THIS_MODULE, "virtual_kbd");
    if (!IS_ERR(cl)) {
	    devK = device_create(cl, NULL, MKDEV(Major,0), NULL, "virtual_kbd");
	    devM = device_create(cl, NULL, MKDEV(Major,1), NULL, "virtual_mouse");
    }


	return 0;

 fail2:	input_free_device(vmouse_dev);
 fail1:	input_free_device(vkbd_dev);
	return err;

}

static int device_open(struct inode *inode, struct file *filp) {
    if (Device_Open[0] && Device_Open[1]) return -EBUSY;
    ++Device_Open[iminor(filp->f_path.dentry->d_inode)];
    return 0;
}
	
static int device_release(struct inode *inode, struct file *filp) {
    --Device_Open[iminor(filp->f_path.dentry->d_inode)];
    return 0;
}
	
static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset) {
    const char* message = 
        " Just feed it input events ( ,    ,    ) \n";
    const size_t msgsize = strlen(message);
    loff_t off = *offset;
    if (off >= msgsize) {
        return 0;
    }
    if (length > msgsize - off) {
        length = msgsize - off;
    }
    if (copy_to_user(buffer, message+off, length) != 0) {
        return -EFAULT;
    }

    *offset+=length;
    return length;
}
	
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t *off) {
    unsigned int command;
    unsigned int arg1;
	int arg2;
	struct input_dev *dev = (iminor(filp->f_path.dentry->d_inode)) ? vmouse_dev : vkbd_dev;

	int *myinput;
    int i; 
	for(i=0; i<len; i += ( 3 * sizeof(int) )) {
			myinput = buff + i;
			command = myinput[0];
			arg1 = myinput[1];
			arg2 = myinput[2];
            input_event(dev, command, arg1, arg2);
			input_sync(dev);
			//printk("vkbd_dev: %d %d %d\n", command, arg1, arg2);
    }

    return len;
}

static void __exit inputdummy_exit(void)
{
	input_unregister_device(vkbd_dev);
	input_unregister_device(vmouse_dev);

    if (!IS_ERR(cl)) {
	    device_destroy(cl, MKDEV(Major,0));
	    device_destroy(cl, MKDEV(Major,1));
	    class_destroy(cl);
    }
    unregister_chrdev(Major, DEVICE_NAME);
}

module_init(inputdummy_init);
module_exit(inputdummy_exit) ;

MODULE_AUTHOR("Arkadiusz Guzinski, kermit@ag.de1.cc, based on code from Vitaly Shukela, vi0oss@gmail.com");
MODULE_DESCRIPTION("Virtual keyboard and mouse driver");
MODULE_LICENSE("GPL");


