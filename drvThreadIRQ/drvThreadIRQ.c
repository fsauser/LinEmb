/***************************************************************************//**
*  \file       drvThreadIRQ.c
*
*  \details    Linux driver with threaded IRQ
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
#include <linux/gpio.h>


#define  DEVICE_NAME "drvThreadIRQ"  /* The device will appear at /dev/drvThreadIRQ using this value*/
#define  CLASS_NAME  "hearc"    /* The device class -- this is a character device driver	*/

#define GPIO_IN 21

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FSA");
MODULE_DESCRIPTION("A Linux driver IRQ for RPi4"); 	
MODULE_VERSION("0.2");           

static int    nbrOpens = 0;         /* Counts the number of times the device is opened		*/
static struct class*  drvThreadIRQClass  = NULL; /* The device-driver class struct pointer			*/
static struct device* drvThreadIRQDevice = NULL; /* The device-driver device struct pointer		*/
static struct cdev drvThreadIRQCdev;
dev_t dev = 0;

// Prototype functions for the character driver
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static irqreturn_t irq_thread_fn(int irq, void *dev_id);
static irqreturn_t irq_handler(int irq, void *dev_id);


/** @brief Devices are represented as file structure in the kernel. 
 *  The file_operations structure from /linux/fs.h lists the callback functions that you wish to 
 *  associated with your file operations using a C99 syntax structure. 
 *  char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
	.owner    = THIS_MODULE,
  .open     = dev_open,
  .read     = dev_read,
  .write    = dev_write,
  .release  = dev_release,
};

static short int inputsIrq;

static irqreturn_t irq_handler(int irq, void *dev_id)
{
  static ktime_t lastIrqTime = 0;
  ktime_t now, dTime;

  now = ktime_get_ns();
  dTime= ktime_to_ms(ktime_sub(now, lastIrqTime));
  lastIrqTime = now;
    
  if (dTime<100) return IRQ_HANDLED;
 
  return IRQ_WAKE_THREAD;
}

static irqreturn_t irq_thread_fn(int irq, void *dev_id) 
{
  pr_info("IRQ Func.\n");
  return IRQ_HANDLED;
}

/** @brief The initialization function
 *  The static keyword restricts the visibility of the function to within this C file. 
 *  The __init macro means that for a built-in driver the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init drvThreadIRQ_init(void)
{
  int ret;
  /* Allocating Major number */
  if((alloc_chrdev_region(&dev, 0, 1, "drvThreadIRQ")) <0)
  {
    pr_err("Cannot allocate major number\n");
    goto err_reg;
  }
  pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

  /* Creating struct class */
  drvThreadIRQClass = class_create(THIS_MODULE, "drvThreadIRQClass");
  if(IS_ERR(drvThreadIRQClass))
  {
    pr_err("Cannot create the struct class\n");
    goto err_class;
  }

  /* Creating cdev structure */
  cdev_init(&drvThreadIRQCdev, &fops);

  /* Adding character device to the system */
  ret = cdev_add(&drvThreadIRQCdev, dev, 1);
  if( ret < 0)
  {
    pr_err("Cannot add the device to the system\n");
    goto err_cdev;
  }

  /* Creating device */
  drvThreadIRQDevice = device_create(drvThreadIRQClass, NULL, dev, NULL,"drvThreadIRQ");
  if( drvThreadIRQDevice == NULL)
  {
    pr_err( "Cannot create the Device \n");
    goto err_device;
  }
  
  // Requesting the GPIO
  if(gpio_request(GPIO_IN, "GPIO_IN"))
  {
    pr_err("ERROR: GPIO request\n");
    goto r_gpio;
  }

  gpio_direction_input(GPIO_IN);
  inputsIrq = gpio_to_irq(GPIO_IN);

  if (request_threaded_irq( inputsIrq,                  //IRQ number
                            (void *)irq_handler,        //IRQ handler (Top half)
                            irq_thread_fn,              //IRQ Thread handler (Bottom half)
                            IRQF_TRIGGER_RISING         //Handler will be called in raising edge
                            | IRQF_ONESHOT,
                            "IRQ",                      //used to identify the device name using this IRQ
                            NULL))                      //device id for shared IRQ
  {
    pr_err("ERROR: IRQ request\n");
    goto r_gpio;
  }

  pr_info("drvThreadIRQ: Initializing the drvThreadIRQ driver\n");   
  return 0;

r_gpio:
  gpio_free(GPIO_IN);
err_device:
  device_destroy(drvThreadIRQClass, dev);
err_cdev:
  cdev_del(&drvThreadIRQCdev);
err_class:
  class_destroy(drvThreadIRQClass);	
err_reg:
  unregister_chrdev_region(dev, 1);
  
  return -1;
}

/** @brief The cleanup function
 *  Similar to the initialization function, it is static. 
 *  The __exit macro notifies that if this code is used for a built-in driver 
 *  that this function is not required.
 */
static void __exit drvThreadIRQ_exit(void)
{
  free_irq(inputsIrq, NULL);
  gpio_free(GPIO_IN);
  device_destroy(drvThreadIRQClass, dev);    // remove the device
  cdev_del(&drvThreadIRQCdev);               // remove character device to the system 
  class_destroy(drvThreadIRQClass);          // remove the device class
  unregister_chrdev_region(dev, 1);     // unregister major number
  pr_info("drvThreadIRQ: Goodbye!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the nbrOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep)
{
  nbrOpens++;
  pr_info("drvThreadIRQ: Device has been opened %d time(s)\n", nbrOpens);
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
  pr_info("read\n");
  return len;
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
  pr_info("write\n");
  return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep)
{
  pr_info("drvThreadIRQ: Device closed\n");
  return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(drvThreadIRQ_init);
module_exit(drvThreadIRQ_exit);

