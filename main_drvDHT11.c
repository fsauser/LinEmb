

#define GPIO_1W_PIN 4

/** @brief fonction qui initialise le GPIO utilisé par le capteur
 *  @param numéro du GPIO
 */
int8_t initGPIODHT11(int gpio)
{
  //Output GPIO configuration
  //Checking the GPIO is valid or not
  if(gpio_is_valid(gpio) == false)
  {
    pr_err("GPIO %d is not valid\n", gpio);
    return -1;
  }
  //Requesting the GPIO
  if(gpio_request_one(gpio, GPIOF_OPEN_DRAIN,"GPIO_1W_PIN") < 0)
  {
    pr_err("ERROR: GPIO %d request\n", gpio);
    return -1;
  } 
 	gpio_direction_output(gpio, 1);

  return 0;
}

/** @brief fonction qui libère le GPIO utilisé par le capteur
 *  @param numéro du GPIO
 */
void deInitGPIODHT11(int gpio)
{
	gpio_free(gpio);
}

/** @brief fonction initialise un début de trame pour le capteur
 *  @param numéro du GPIO
 */
int8_t start1W(int gpio) 
{
	unsigned char read;
	unsigned long int dTime;
	struct timespec64 ts1, ts2;

	gpio_direction_output(gpio, 0);
  mdelay(19);
  ts1 = ns_to_timespec64(ktime_get_ns());
	gpio_direction_input(gpio);
	// wait 20-40us for '0' 
	do
  {
  	read = gpio_get_value(gpio)&0x1;  	
		ts2 = ns_to_timespec64(ktime_get_ns());
		dTime = ts2.tv_nsec - ts1.tv_nsec;
  }
  while(dTime<40000 && read!=0);
	if (read!=0) 
	{
		gpio_direction_output(gpio, 1);
		return -1;
	}
	
	// wait 80us for '1'
	ts1 = ns_to_timespec64(ktime_get_ns());
	udelay(40);
 	do
  {
  	read = gpio_get_value(gpio)&0x01;  	
		ts2 = ns_to_timespec64(ktime_get_ns());
		dTime = ts2.tv_nsec - ts1.tv_nsec;
  }
  while(dTime<85000 && read==0);
  if (read==0) 
  {
		gpio_direction_output(gpio, 1);
		return -2;
	};
  
  // wait 100us for '0'
  ts1 = ns_to_timespec64(ktime_get_ns());
  udelay(30);
	do
  {
  	read = gpio_get_value(gpio)&0x01;  	
		ts2 = ns_to_timespec64(ktime_get_ns());
		dTime = ts2.tv_nsec - ts1.tv_nsec;
  }
  while(dTime<100000 && read!=0);
  if (read!=0) 
 	{
		gpio_direction_output(gpio, 1);
		return -3;
	};
  return 0;
}

/** @brief fonction qui lit un bit de la trame du capteur
 *  @param numéro du GPIO
 */
uint8_t readBit(int gpio) 
{
	unsigned char read;
	unsigned long int dTime;
	struct timespec64 ts1, ts2;

  // wait 50us for '1'
	ts1 = ns_to_timespec64(ktime_get_ns());
 	do
  {
  	read = gpio_get_value(gpio)&0x01;  	
		ts2 = ns_to_timespec64(ktime_get_ns());
		dTime = ts2.tv_nsec - ts1.tv_nsec;  	
  }
  while(dTime<50000 && read==0);  
  
	// lenght of '1'
	ts1 = ns_to_timespec64(ktime_get_ns());
 	do
  {
  	read = gpio_get_value(gpio)&0x01;  	
  	ts2 = ns_to_timespec64(ktime_get_ns());
		dTime = ts2.tv_nsec - ts1.tv_nsec;  
  }
  while(dTime<70000 && read!=0);  
	
  if (dTime<40000) return 0;
	else return 1;
}

/** @brief fonction qui lit les données du capteur
 *  @param pointeur pour les bytes lus
 *  @param numéro du GPIO
 */
int8_t read40bits(uint8_t *data, int gpio) 
{
	uint8_t byte;
	uint8_t temp;
	int i, j;
	int8_t ret;
	int8_t nbr;

	do
	{
		ret = start1W(gpio); 
		nbr++;
	} while(ret<0 && nbr <5);
	
  if (ret==0) 
  {
		for (j = 0; j < 5; j++) 
		{
			byte = 0;
			for (i = 7; i >= 0; i--) 
			{
				temp = readBit(gpio);
				byte = data | (temp << i);
			}
			data[j]=byte;
		}
		udelay(200);
		gpio_direction_output(gpio, 1);  
  }
  else pr_err("%d\n", ret);
 	
	//pr_info("%02x;%02x;%02x;%02x;%02x\n", data[0], data[1], data[2], data[3], byte);
	return ret;
}

