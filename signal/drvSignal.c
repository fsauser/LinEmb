/***************************************************************************//**
*  \file       drvSignal.c
*
*  \details    Linux driver with signals
*
*  \author     HE-ARC2024
*
* *******************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>    
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/err.h>
#include <linux/timer.h>
#include <linux/jiffies.h>	

#define SIGCOMM 44
 
#define REG_CURRENT_TASK _IOW('a','a',int32_t*)
 
// Signaling to Application
static struct task_struct *appTask = NULL;
static int signum = 0;
   
dev_t dev = 0;
static struct class* drvSignalClass = NULL;
static struct device* drvSignalDevice = NULL;
static struct cdev drvSignalCdev;

void timerSig_fct(struct timer_list *);
struct timer_list timerSig;

static int __init drvSignal_init(void);
static void __exit drvSignal_exit(void);
static int drvSignal_open(struct inode *inode, struct file *file);
static int drvSignal_release(struct inode *inode, struct file *file);
static ssize_t drvSignal_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t drvSignal_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static long drvSignal_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
 
static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = drvSignal_read,
        .write          = drvSignal_write,
        .open           = drvSignal_open,
        .unlocked_ioctl = drvSignal_ioctl,
        .release        = drvSignal_release,
};

void timerSig_fct(struct timer_list *timer)
{
  struct kernel_siginfo info;

  // Sending signal to app
  memset(&info, 0, sizeof(struct kernel_siginfo));
  info.si_signo = SIGCOMM;
  info.si_code = SI_QUEUE;
  info.si_int = 1;

  if (appTask != NULL) 
  {
    pr_info("Sending signal to app\n");
    if(send_sig_info(SIGCOMM, &info, appTask) < 0) 
    {
      pr_info("Error: Sending signal\n");
    }
  }
  mod_timer(timer, jiffies + (msecs_to_jiffies(500)));
}
 
static int drvSignal_open(struct inode *inode, struct file *file)
{
  pr_info("drvSignal opened\n");
  return 0;
}
 
static int drvSignal_release(struct inode *inode, struct file *file)
{
  struct task_struct *refTask = get_current();
  pr_info("drvSignal closed\n");
    
  // delete the app. task
  if(refTask == appTask) 
  {
    appTask = NULL;
  }
  return 0;
}
 
static ssize_t drvSignal_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
  pr_info("Read function\n");
  return 0;
}

static ssize_t drvSignal_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
  pr_info("Write function\n");
  return len;
}
 
static long drvSignal_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  if (cmd == REG_CURRENT_TASK) 
  {
    pr_info("REG_CURRENT_TASK\n");
    appTask = get_current();
    signum = SIGCOMM;
  }
  return 0;
}
 
 
static int __init drvSignal_init(void)
{
  int ret;
  // Allocating Major number
  if((alloc_chrdev_region(&dev, 0, 1, "drvSignal")) <0)
  {
    pr_info("Cannot allocate major number\n");
    return -1;
  }
  pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

  // Creating struct class
  drvSignalClass = class_create(THIS_MODULE, "drvSignalClass");
  if(IS_ERR(drvSignalClass))
  {
    pr_info("Cannot create the struct class\n");
    goto r_class;
  }

  // Creating cdev structure
  cdev_init(&drvSignalCdev, &fops);

  ret = cdev_add(&drvSignalCdev, dev, 1);
  if( ret < 0)
	{
    pr_err("Cannot add the device\n");
    goto r_cdev;
  }

  // Creating device
  drvSignalDevice = device_create(drvSignalClass, NULL, dev, NULL,"drvSignal");
  if(IS_ERR(drvSignalDevice))
  {
    pr_info("Cannot create the device\n");
    goto r_device;
  }

  //set timer
  timer_setup(&timerSig, timerSig_fct, 0);
  mod_timer(&timerSig, jiffies + msecs_to_jiffies(500));

  pr_info("drvSignal inserted\n");
  return 0;

r_device:
  class_destroy(drvSignalClass);
r_cdev:
  cdev_del(&drvSignalCdev);
r_class:
  unregister_chrdev_region(dev, 1);
  return -1;
}
 
static void __exit drvSignal_exit(void)
{
  del_timer(&timerSig);
  device_destroy(drvSignalClass, dev);
  class_destroy(drvSignalClass);
  cdev_del(&drvSignalCdev);
  unregister_chrdev_region(dev, 1);
  pr_info("drvSignal removed\n");
}
 
module_init(drvSignal_init);
module_exit(drvSignal_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HE-ARC <>");
MODULE_DESCRIPTION("Linux driver with signals");
MODULE_VERSION("1.31");

