/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Your Name Here"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    struct aesd_dev *dev;

    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    //TODO: ulocking setup
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    //struct aesd_buffer_entry *tmp_entry;
    ssize_t retval = 0;
    size_t entry_offset;
    size_t bytes_to_copy;
    //tmp_entry->buffptr = NULL;
    //tmp_entry->size = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */   
     struct aesd_buffer_entry *tmp_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &entry_offset);
     if (tmp_entry != NULL)
     {
        if ((tmp_entry->size - entry_offset) < count)
        {
            bytes_to_copy = tmp_entry->size - entry_offset;
        }
        else
        {
            bytes_to_copy = count;
        }
        if (copy_to_user(buf, tmp_entry->buffptr + entry_offset, bytes_to_copy))
        {
            return -EFAULT; //copy failed
        }
        *f_pos += bytes_to_copy;
        retval = bytes_to_copy;
     }
     else
     {
        retval = 0;
     }
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    //get memory to hold 
    char *usr_tmp = kmalloc(count, GFP_KERNEL);
    if (usr_tmp == NULL)
    {
        return retval;
    }
    //copy user to kernel
    if (copy_from_user(usr_tmp, buf, count))
    {
        kfree(usr_tmp);
        return -EFAULT; //copy failed
    }
    //put new user command into working buff
    char *mybuf = krealloc(dev->working_buffer, dev->work_buff_size + count, GFP_KERNEL);
    if (mybuf == NULL)
    {
        kfree(usr_tmp);
        return retval;
    }
    dev->working_buffer = mybuf;
    memcpy(dev->working_buffer+ dev->work_buff_size, usr_tmp, count); //move pointer to the end of data in working buffer
    dev->work_buff_size += count; //update size
    PDEBUG("updated working buff size %zu", dev->work_buff_size);
    kfree(usr_tmp);

    char *result = memchr(dev->working_buffer, '\n', dev->work_buff_size);
    if (result != NULL) //found new line
    {
        PDEBUG("New line found!");
        //put into circular buffer
        struct aesd_buffer_entry newEntry;
        newEntry.buffptr = dev->working_buffer;
        newEntry.size = dev->work_buff_size;
        const char *entry_ptr = aesd_circular_buffer_add_entry(&dev->buffer, &newEntry);
        if (entry_ptr != NULL)
        {
            kfree(entry_ptr);
        }
        dev->work_buff_size = 0;
        dev->working_buffer = NULL;
    }
    return count;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.buffer); //initialize circular buffer
    aesd_device.work_buff_size = 0;
    aesd_device.working_buffer = NULL;

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    uint8_t idx;
    struct aesd_buffer_entry *entry;
    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, idx)
    {
        kfree(entry->buffptr);
    }
    kfree(aesd_device.working_buffer);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
