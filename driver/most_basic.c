/*
 * A very basic character driver to demonstrate using strace to find a bug
 * in the driver.
 * Copyright (C) 2019 IT Dev Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kernel.h>

/*
 * Forward declarations
 */
static ssize_t itdev_example_cdev_read_special_data(
		struct file *file_ptr, struct kobject *kobj,
		struct bin_attribute *battr, char *data, loff_t offset,
		size_t size);
static ssize_t itdev_example_cdev_read(
		struct file *file, char *buf,
		size_t length, loff_t *offset);
static int __init itdev_example_cdev_init(void);
static void __exit itdev_example_cdev_exit(void);

/*
 * Macros
 */
/* Number of devices this driver supports */
#define NUM_MINOR_DEVS 1U

/* Prefix for debug output - makes for easier grepping */
#define TAG "ITDev: "

/* 
 * Define DEBUG to enable some of the trace_printk() output that we
 * imagine the developer using to debug the issue with this code.
 */
#define DEBUG
//#undef DEBUG

/*
 * Globals
 */
/* A small buffer of data to return to the user when device is read */
static const char test_read_msg[] = "ITDev example cdev driver";
static size_t test_read_msg_len = 25U;

/*
 * A small buffer of data to return to the user when the driver's special
 * sysfs file is read.
 */
static const char test_data_block[] = "AABBCCDDEEFFGGHHIIJJKK";
static size_t test_data_block_len = 22U;

/*
 * A character device driver receives these unaltered system calls. We only
 * need to define the useful fields - everything else is implicitly initialised
 * to zero. Our driver is going to be read only for simplicity.
 */
const struct file_operations fops = {
	.read = itdev_example_cdev_read,
};

/*
 * The driver "context". This structure holds data for the device instance(s)
 * in a structure that private to the driver.
 *
 * devnum   - Device major/minor number.
 * pdev     - Character device.
 * battr    - Attributes for sysfs file.
 * sysfsdev - Sysfs device entry - for dyrectory "itdev", under /sys/devices
 */
struct itdev_example_cdev_ctx {
	dev_t devnum;
	struct cdev *pdev;
	struct bin_attribute battr;
	struct device *sysfsdev;
} gbl_ctx = {
	.devnum = 0,
	.pdev = NULL,
	.sysfsdev = NULL
};

/*
 * Functions
 */
/*
 * The read routine for our driver's "special" file, which appears at
 * /sys/devices/itdev/special_file. It will return a block of data of a fixed
 * size.
 */
static ssize_t itdev_example_cdev_read_special_data(
		struct file *file_ptr, struct kobject *kobj,
		struct bin_attribute *battr, char *data, loff_t offset,
		size_t size)
{
#ifdef DEBUG
	trace_printk(TAG "Reading %zu bytes\n", size);
#endif

	if (size < test_data_block_len)
		return -EINVAL;

	memcpy(data, test_data_block, test_data_block_len);
	return test_data_block_len;
}

/*
 * The character device read function. Just returns a dummy string to
 * the user.
 */
static ssize_t itdev_example_cdev_read(
		struct file *file, char *buf,
		size_t length, loff_t *offset)
{
	ssize_t bytes_read = 0;

	if (*offset < test_read_msg_len) {
		bytes_read = test_read_msg_len - *offset;
		if (copy_to_user(buf, test_read_msg + *offset, bytes_read))
			return -EFAULT;

		*offset += bytes_read;
	}

	return bytes_read;
}

/*
 * Function is called when the module is loaded. it will allocate a major and
 * minor number for a new character device, allocate the device and associate
 * it with its maj/min number. It will also setup the character device's
 * file operations - we only implement read. The device will appear as
 * /dev/itdev0. It will also create a sysfs "special" file. This we will
 * imagine is a way for our device to return binary data to userspace. It is
 " only "special" in the sense that the data is something other than wat we
 * would read out of /dev/itdev0.
 */
static int __init itdev_example_cdev_init(void)
{
	int result;

	printk(TAG "ITDev Ltd. example cdev init\n");

	/*
	 * Allocate a device number to use. The major and minor numbers are
	 * returned in `devnum`
	 */
	result = alloc_chrdev_region(
		&gbl_ctx.devnum, 0, NUM_MINOR_DEVS,
		"ITDev Blog Example Driver");
	if (result)
		goto alloc_chrdev_failed;
	printk(TAG "Device major number: %u\n", MAJOR(gbl_ctx.devnum));

	/*
	 * Create a cdev for each device - we only have one in this toy
	 * example
	 */
	gbl_ctx.pdev = cdev_alloc();
	if (!gbl_ctx.pdev) {
		result = -ENOMEM;
		goto alloc_chrdev_failed;
	}

	/*
	 * Tell Linux where the functions to callback for cdev system calls
	 * reside
	 */
	gbl_ctx.pdev->ops = &fops;

	/* Set the rest up */
	gbl_ctx.pdev->owner = THIS_MODULE;

	/* Associate this device with its devnum */
	result = cdev_add(gbl_ctx.pdev, gbl_ctx.devnum, NUM_MINOR_DEVS);
	if (result)
		goto cdev_add_failed;

	/* Create /sys/devices/itdev */
	gbl_ctx.sysfsdev = root_device_register("itdev");
	if (IS_ERR(gbl_ctx.sysfsdev)) {
		result = PTR_ERR(gbl_ctx.sysfsdev);
		goto cdev_add_failed;
	}

	/*
	 * Create a sysfs entry to return some "special" data to user-space
	 *
	 * There is a deliberate problem here. The intent is that the file
	 * should never exhaust as we imagine that the device always has
	 * binary data to return.
	 */
	printk(TAG "Creating the sysfs attributes\n");
	sysfs_bin_attr_init(&gbl_ctx.battr);
	gbl_ctx.battr.attr.name = "special_data";
	gbl_ctx.battr.attr.mode = S_IRUGO;
	gbl_ctx.battr.read = itdev_example_cdev_read_special_data;
	gbl_ctx.battr.write = NULL;
	gbl_ctx.battr.size = test_data_block_len; /* Hint: Problem here! */

	result = sysfs_create_bin_file(
		&gbl_ctx.sysfsdev->kobj, &gbl_ctx.battr);
	if (result)
		goto bin_file_failed;

	return 0;

bin_file_failed:
	root_device_unregister(gbl_ctx.sysfsdev);
cdev_add_failed:
	cdev_del(gbl_ctx.pdev);
alloc_chrdev_failed:
	unregister_chrdev_region(gbl_ctx.devnum, NUM_MINOR_DEVS);
	return result;
}
module_init(itdev_example_cdev_init);

/*
 * Called when the module is removed. Clean up everything we created.
 */
static void __exit itdev_example_cdev_exit(void)
{
	printk(TAG "ITDev Ltd. example cdev exit\n");
	sysfs_remove_bin_file(&gbl_ctx.sysfsdev->kobj, &gbl_ctx.battr);
	root_device_unregister(gbl_ctx.sysfsdev);
	cdev_del(gbl_ctx.pdev);
	unregister_chrdev_region(gbl_ctx.devnum, NUM_MINOR_DEVS);
}
module_exit(itdev_example_cdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ITDev Ltd.");
MODULE_DESCRIPTION("A skeleton cdev driver for a blog.");
