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
#include <linux/module.h>
#include <linux/interrupt.h>

#define MODNAME "inputdummy"

#define DEVICE_NAME "inputdummy"
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int Major;            /* Major number assigned to our device driver */
static int Device_Open[2] = {0,0};  /* Is device open?  Used to prevent multiple access to the device */

struct class * cl;
struct device * dev;

struct file_operations fops = {
       read: device_read,
       write: device_write,
       open: device_open,
       release: device_release
};			

static struct input_dev *vkbd_dev;
static struct input_dev *vmouse_dev;
static struct input_dev *vtablet_dev;
static struct input_dev *vjoy_dev;

#define MAP_TO_LONG(i) (i / (sizeof(long) * BITS_PER_BYTE) )

void setAllBits(unsigned long * bits, unsigned long min, unsigned long max)
{
	max--;
//  	printk ("inputdummy: setAllBits: min=%lu, max=%lu\n", min, max);
	unsigned long i, j, jmax, b;
	for (i = MAP_TO_LONG(min); i <= MAP_TO_LONG(max) ; i++)
	{
// 		printk ("inputdummy: MIN=%lu, MAX =%lu, i=%lu\n", MAP_TO_LONG(min), MAP_TO_LONG(max), i);
		if (i == MAP_TO_LONG(min) || i == MAP_TO_LONG(max))
		{
			int bmin = 0, bmax = 63;
			if (i == MAP_TO_LONG(min))
				bmin = min - (MAP_TO_LONG(min) * sizeof(long) * BITS_PER_BYTE);
			if (i == MAP_TO_LONG(max))
				bmax = max - ((MAP_TO_LONG(max)) * sizeof(long) * BITS_PER_BYTE);
			if (bmin != 0 || bmax != 63)
				b = GENMASK_ULL(bmax, bmin);
			else
				b = ~0L;
//  			printk ("inputdummy: min=%lu, max =%lu, b=%lu\n", bmin, bmax, b);
			bits[i] = b;
		}
		else
			bits[i] = ~0L;
	}
}

static int __init inputdummy_init(void)
{
	int err, i;

	vkbd_dev = input_allocate_device();
	if (!vkbd_dev)
		return -ENOMEM;
	vmouse_dev = input_allocate_device();
	if (!vmouse_dev)
		return -ENOMEM;
	vtablet_dev = input_allocate_device();
	if (!vtablet_dev)
		return -ENOMEM;
	vjoy_dev = input_allocate_device();
	if (!vjoy_dev)
		return -ENOMEM;
	
	// Keyboard
	vkbd_dev->evbit[0] = BIT_MASK(EV_KEY)| BIT_MASK(EV_LED);
	setAllBits(vkbd_dev->keybit, KEY_RESERVED, KEY_CNT);
// 	setAllBits(vkbd_dev->keybit, KEY_RESERVED, KEY_MICMUTE);
// 	setAllBits(vkbd_dev->keybit, KEY_OK, KEY_LIGHTS_TOGGLE);
 	setAllBits(vkbd_dev->ledbit, LED_NUML, LED_SCROLLL);
	vkbd_dev->name = "Virtual Keyboard";
	vkbd_dev->phys = "inputdummy/input0";
	
	err = input_register_device(vkbd_dev);
    printk ("inputdummy: registered=%d\n", err);
	if (err)
		goto fail1;


	// Mouse
	vmouse_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	setAllBits(vmouse_dev->keybit, BTN_MISC, BTN_GEAR_UP);
  	setAllBits(vmouse_dev->relbit, REL_X, REL_CNT);
	vmouse_dev->name = "Virtual Mouse";
	vmouse_dev->phys = "inputdummy/input1";

	err = input_register_device(vmouse_dev);
    printk ("inputdummy: registered=%d\n", err);
	if (err)
		goto fail2;


	// Tablet
	vtablet_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	setAllBits(vtablet_dev->keybit, BTN_MISC, BTN_GEAR_UP);
 	setAllBits(vtablet_dev->absbit, ABS_X, ABS_MISC);
	for (i = ABS_X; i <= ABS_MISC; i++)
	{
 		input_set_abs_params(vtablet_dev, i, 0, 32000, 0, 0);
 		input_set_abs_params(vtablet_dev, i, 0, 20000, 0, 0);
	}
	vtablet_dev->name = "Virtual Tablet";
	vtablet_dev->phys = "inputdummy/input2";

	err = input_register_device(vtablet_dev);
    printk ("inputdummy: registered=%d\n", err);
	if (err)
		goto fail3;

	// Joystick
	vjoy_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	setAllBits(vjoy_dev->keybit, BTN_JOYSTICK, BTN_DIGI);
 	setAllBits(vjoy_dev->absbit, ABS_X, ABS_MISC);
	for (i = ABS_X; i <= ABS_MISC; i++)
	{
 		input_set_abs_params(vjoy_dev, i, -255, 255, 0, 0);
 		input_set_abs_params(vjoy_dev, i, -255, 255, 0, 0);
	}
	vjoy_dev->name = "Virtual Joystick";
	vjoy_dev->phys = "inputdummy/input3";

	err = input_register_device(vjoy_dev);
    printk ("inputdummy: registered=%d\n", err);
	if (err)
		goto fail4;


    /* Above is evdev part. Below is character device part */

    Major = register_chrdev(0, "virtual_input", &fops);	
    if (Major < 0) {
		printk ("Registering the character device failed with %d\n", Major);
	    goto fail1;
    }
    printk ("inputdummy: Major=%d\n", Major);

    cl = class_create(THIS_MODULE, "virtual_input");
    if (!IS_ERR(cl)) {
	    dev = device_create(cl, NULL, MKDEV(Major,0), NULL, "virtual_kbd");
	    dev = device_create(cl, NULL, MKDEV(Major,1), NULL, "virtual_mouse");
	    dev = device_create(cl, NULL, MKDEV(Major,2), NULL, "virtual_tablet");
	    dev = device_create(cl, NULL, MKDEV(Major,3), NULL, "virtual_joystick");
    }


	return 0;

  fail4:	input_free_device(vjoy_dev);
  fail3:	input_free_device(vtablet_dev);
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

	struct input_dev *dev;
	int minor = (iminor(filp->f_path.dentry->d_inode));
	switch (minor)
	{
		case 0:
			dev = vkbd_dev;
			break;
		case 1:
			dev = vmouse_dev;
			break;
		case 2:
			dev = vtablet_dev;
			break;
		case 3:
			dev = vjoy_dev;
			break;
		default:
			printk("inputdumy: minor number of %d requested", minor);
			return -1;
	}
	int *myinput;
    int i; 
	for(i=0; i<len; i += ( 3 * sizeof(int) )) {
			myinput = buff + i;
			command = myinput[0];
			arg1 = myinput[1];
			arg2 = myinput[2];
            input_event(dev, command, arg1, arg2);
			if (minor < 2)
				input_sync(dev);
// 			printk("vkbd_dev: %d %d %d\n", command, arg1, arg2);
    }

    return len;
}

static void __exit inputdummy_exit(void)
{
	input_unregister_device(vkbd_dev);
	input_unregister_device(vmouse_dev);
	input_unregister_device(vtablet_dev);
	input_unregister_device(vjoy_dev);

    if (!IS_ERR(cl)) {
	    device_destroy(cl, MKDEV(Major,0));
	    device_destroy(cl, MKDEV(Major,1));
	    device_destroy(cl, MKDEV(Major,2));
	    device_destroy(cl, MKDEV(Major,3));
	    class_destroy(cl);
    }
    unregister_chrdev(Major, DEVICE_NAME);
}

module_init(inputdummy_init);
module_exit(inputdummy_exit) ;

MODULE_AUTHOR("Arkadiusz Guzinski, kermit@ag.de1.cc, based on code from Vitaly Shukela, vi0oss@gmail.com");
MODULE_DESCRIPTION("Virtual keyboard and mouse driver");
MODULE_LICENSE("GPL");


