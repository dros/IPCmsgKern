#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/list.h>
#include "IPCmsgKernModMsg.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andreas Groth Egebjerg");
MODULE_DESCRIPTION("IPCmsgKern kernel driver.");
MODULE_VERSION("0.01");
#define IPCmsgKern_DEVICE_NAME "IPCmsgKern"


static int major_number;
static struct class* ipc_class = NULL;
static struct device* ipc_device = NULL;




static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define debug(code) code

static struct file_operations file_ops = {
 .read = device_read,
 .write = device_write,
 .open = device_open,
 .release = device_release
};/* When a process reads from our device, this gets called. */

static ssize_t device_read(struct file *pfile, char *buffer, size_t len, loff_t *offset) {
  int bytes_read = 0;
  fdHanle_object *fdHandle = NULL;
  if(pfile->private_data == NULL)
  {
    return -1;
  }
  fdHandle = pfile->private_data;


 // check if the message has the correct size
 if(len == sizeof(IPCmsgKern_sendHeader))
 {
   IPCmsgKern_sendHeader Head;
   copyUserData((char *)&Head, buffer, sizeof(IPCmsgKern_sendHeader));
   if(0 != memcmp(Head.magicMsgGuard, IPCmsgKern_MAGIC_CONST_SEND, sizeof(Head.magicMsgGuard)))
   {
     printk(KERN_INFO "IPCmsgKern device_read received a invalid request\n");
     return 0;
   }

   debug(printk(KERN_INFO "IPCmsgKern Head.msgType %d\n", Head.msgType);)
   if((Head.msgType >= IPCmsgKern_msg_type_create) && (Head.msgType < IPCmsgKern_msg_type_end))
   {
     bytes_read = handle_msg_request(&Head, buffer, fdHandle);
   }
 }

 return bytes_read;
}/* Called when a process tries to write to our device */

static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {
 /* This is a read-only device */
 if(len > 10)
 {
   char test = 0;
//   test[0] = 0;
  // copy_from_user(test, buffer, 10);
  get_user(test,buffer);
   printk(KERN_INFO "IPCmsgKern device_write %d\n", (int)test);
 }
 return len;
}/* Called when a process opens our device */

static int device_open(struct inode *inode, struct file *file) {
 /* If device is open, return busy */
  fdHanle_object *handle = NULL;

  printk(KERN_INFO "IPCmsgKern device_open \n");

  if(!try_module_get(THIS_MODULE))
  {
    printk(KERN_INFO "IPCmsgKern device_open error calling try_module_get\n");
    return -1;
  }

  if ((handle = kmalloc(sizeof(*handle), GFP_KERNEL)) != NULL)
  {
    handle->dead = false;
    {
    	static struct lock_class_key __key;
  	__mutex_init(&(handle->fdHanle_mutex), "fdHanle_mutex", &__key);
    }
    INIT_LIST_HEAD(&(handle->channelList));
    INIT_LIST_HEAD(&(handle->connectionList));
    file->private_data = handle;
    return 0;
  }

 return -1;
}/* Called when a process closes our device */

static int device_release(struct inode *inode, struct file *file) {
  fdHanle_object *fdHandle = NULL;
  printk(KERN_INFO "IPCmsgKern device_release \n");

  if(file->private_data == NULL)
  {
    return -1;
  }
  fdHandle = file->private_data;
  handle_release(fdHandle);

 module_put(THIS_MODULE);
 return 0;
}

static int __init IPCmsgKern_init(void) {
  printk(KERN_INFO "IPCmsgKern init\n");
    major_number = register_chrdev(0, IPCmsgKern_DEVICE_NAME, &file_ops);
    if (major_number < 0) {
        printk(KERN_ALERT "Failed to register a major number\n");
        return major_number;
    }

    ipc_class = class_create(THIS_MODULE, "ipc_class");
    if (IS_ERR(ipc_class)) {
        unregister_chrdev(major_number, IPCmsgKern_DEVICE_NAME);
        printk(KERN_ALERT "Failed to create a device class\n");
        return PTR_ERR(ipc_class);
    }

    ipc_device = device_create(ipc_class, NULL, MKDEV(major_number, 0), NULL, IPCmsgKern_DEVICE_NAME);
    if (IS_ERR(ipc_device)) {
        class_destroy(ipc_class);
        unregister_chrdev(major_number, IPCmsgKern_DEVICE_NAME);
        printk(KERN_ALERT "Failed to create a device\n");
        return PTR_ERR(ipc_device);
    }
  printk(KERN_INFO "IPCmsgKern registered successfully\n");

    return 0;
}

static void __exit IPCmsgKern_exit(void)
{
  device_destroy(ipc_class, MKDEV(major_number, 0));
  class_destroy(ipc_class);
  unregister_chrdev(major_number, IPCmsgKern_DEVICE_NAME);
  printk(KERN_INFO "IPCmsgKern exit successfully\n");
}/* Register module functions */

module_init(IPCmsgKern_init);
module_exit(IPCmsgKern_exit);



#include "IPCmsgKernModMsg.c"