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

#ifndef INPUTDUMMY_H
#define INPUTDUMMY_H

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/poll.h>

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static unsigned int device_poll(struct file *filp, poll_table *wait);



#endif