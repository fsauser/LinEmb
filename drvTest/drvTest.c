/**
 * @file   	drvTest.c
 * @author 	FSA
 * @date   	26.03.2023
 * @version 0.2
 * @brief   An introductory character driver 
 *
 */
 
#include <linux/module.h>     	/* Needed by all modules 																	*/
#include <linux/kernel.h>     	/* Needed for KERN_INFO 																	*/
#include <linux/init.h>       	/* Needed for the macros 																	*/
#include <linux/fs.h>           /* Header for the Linux file system support 							*/
#include <linux/uaccess.h>      /* Required for the copy to user function									*/
#include <linux/moduleparam.h> 	/* Needed for module parameters 													*/
#include <linux/device.h>       /* Header to support the kernel Driver Model							*/
#include <linux/cdev.h>
#include <linux/kdev_t.h>
 
#define  DEVICE_NAME "drvTest"	/* The device will appear at /dev/drvTest using this value*/
#define  CLASS_NAME  "hearc"   	/* The device class -- this is a character device driver	*/

MODULE_LICENSE("GPL");         	
MODULE_AUTHOR("FSA");    		
MODULE_DESCRIPTION("A simple Linux char driver for RPi4"); 	
MODULE_VERSION("0.2");           

static char   message[256] = {0};   /* Memory for the string that is passed from userspace*/
static short  size_of_msg;					/* Used to remember the size of the string stored			*/
static int    nbrOpens = 0;         /* Counts the number of times the device is opened		*/
static struct class*  drvTestClass  = NULL; /* The device-driver class struct pointer			*/
static struct device* drvTestDevice = NULL; /* The device-driver device struct pointer		*/
static struct cdev drvTestCdev;
dev_t dev = 0;

// Prototype functions for the character driver
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

/** @brief Devices are represented as file structure in the kernel. 
 *  The file_operations structure from /linux/fs.h lists the callback functions that you wish to 
 *  associated with your file operations using a C99 syntax structure. 
 *  char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
	.owner  	= THIS_MODULE,
  .open 		= dev_open,
  .read 		= dev_read,
  .write 		= dev_write,
  .release 	= dev_release,
};

/** @brief The initialization function
 *  The static keyword restricts the visibility of the function to within this C file. 
 *  The __init macro means that for a built-in driver the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init drvTest_init(void)
{
  int ret;
	/* Allocating Major number */
  if((alloc_chrdev_region(&dev, 0, 1, "drvTest")) <0)
	{
    pr_err("Cannot allocate major number\n");
    goto err_reg;
  }
  pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

  /* Creating struct class */
  drvTestClass = class_create(THIS_MODULE, "drvTestClass");
  if(IS_ERR(drvTestClass))
	{
    pr_err("Cannot create the struct class\n");
    goto err_class;
  }

  /* Creating cdev structure */
  cdev_init(&drvTestCdev, &fops);

  /* Adding character device to the system */
  ret = cdev_add(&drvTestCdev, dev, 1);
  if( ret < 0)
	{
    pr_err("Cannot add the device to the system\n");
    goto err_cdev;
  }

  /* Creating device */
  drvTestDevice = device_create(drvTestClass, NULL, dev, NULL,"drvTest");
  if( drvTestDevice == NULL)
	{
    pr_err( "Cannot create the Device \n");
    goto err_device;
  }
	pr_info("drvTest: Initializing the drvTest driver\n");   
	return 0;

err_device:
  device_destroy(drvTestClass, dev);
err_cdev:
  cdev_del(&drvTestCdev);
err_class:
  class_destroy(drvTestClass);	
err_reg:
  unregister_chrdev_region(dev, 1);
  
  return -1;
}

/** @brief The cleanup function
 *  Similar to the initialization function, it is static. 
 *  The __exit macro notifies that if this code is used for a built-in driver 
 *  that this function is not required.
 */
static void __exit drvTest_exit(void)
{
  device_destroy(drvTestClass, dev);	  // remove the device
  cdev_del(&drvTestCdev);               // remove character device to the system 
  class_destroy(drvTestClass);          // remove the device class
  unregister_chrdev_region(dev, 1);     // unregister major number
  pr_info("drvTest: Goodbye!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the nbrOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep)
{
  nbrOpens++;
  pr_info("drvTest: Device has been opened %d time(s)\n", nbrOpens);
  return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
  int error_count = 0;
  // copy_to_user has the format ( * to, *from, size) and returns 0 on success
  error_count = copy_to_user(buffer, message, size_of_msg);

  if (error_count==0)
  {            // if true then have success
    pr_info("drvTest: Sent %d characters to the user\n", size_of_msg);
    return (size_of_msg=0);  // clear the position to the start and return 0
  }
  else 
  {
    pr_info("drvTest: Failed to send %d characters to the user\n", error_count);
    return -EFAULT;          // Failed -- return a bad address message (i.e. -14)
  }
}

/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
  sprintf(message, "%s(%zu letters)", buffer, len);  	// appending received string with its length
  size_of_msg = strlen(message);                 			// store the length of the stored message
  pr_info("drvTest: Received %zu characters from the user\n", len);
  return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep)
{
  pr_info("drvTest: Device closed\n");
  return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(drvTest_init);
module_exit(drvTest_exit);
