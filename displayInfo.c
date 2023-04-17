#include <linux/module.h>   	/* Needed by all modules 	*/
#include <linux/kernel.h>   	/* Needed for KERN_INFO 	*/
#include <linux/init.h>     	/* Needed for the macros 	*/
#include <linux/smp.h>     		/* get_cpu(), put_cpu()  	*/
#include <linux/cpufreq.h> 		/* cpufreq_get()		   	  */
#include <linux/cpumask.h>  	/* cpumask_{first,next}()	*/
				                      /* cpu_online_mas			    */
#include <linux/netdevice.h>	/* for_each_netdev		  	*/
#include <linux/inetdevice.h>

///< The license type -- this affects runtime behavior
MODULE_LICENSE("GPL");

///< The author -- visible when you use modinfo
MODULE_AUTHOR("JSE");

///< The description -- see modinfo
MODULE_DESCRIPTION("GetInfo LKM!");

///< The version of the module
MODULE_VERSION("0.2");

static int ip_info_print(void)
{
	u32 ip;
	struct net_device *dev;
  for_each_netdev(&init_net, dev) 
  {
    pr_info("Interface %s with MAC address %02x:%02x:%02x:%02x:%02x:%02x\n"
              , dev->name
              , dev->dev_addr[0]
              , dev->dev_addr[1]
              , dev->dev_addr[2]
              , dev->dev_addr[3]
              , dev->dev_addr[4]
              , dev->dev_addr[5]);
    if (dev->ip_ptr!=NULL) 
    {
		  ip=((dev->ip_ptr)->ifa_list)->ifa_address;
		  pr_info("Interface %s with IP address %d:%d:%d:%d\n"
		          , dev->name
		          , (ip&0xff)
		          , (ip&0xff00)>>8                
		          , (ip&0xff0000)>>16
		          , (ip&0xff000000)>>24);   
		}         
  }
  return 0;
}

static int cpu_info_print(void)
{
  unsigned cpu = cpumask_first(cpu_online_mask);

  while (cpu < nr_cpu_ids) 
  {
		pr_info("CPU: %u, freq: %u kHz\n", cpu, cpufreq_get(cpu));
    cpu = cpumask_next(cpu, cpu_online_mask);
  }
  return 0;
}

static int __init displayInfo(void)
{
  pr_info("Hello displayInfo\n");
  ip_info_print();
  cpu_info_print();

  return 0;
}

static void __exit bye_displayInfo(void)
{
 	pr_info("Bye displayInfo\n");
}

module_init(displayInfo);
module_exit(bye_displayInfo);

