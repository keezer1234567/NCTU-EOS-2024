#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h> // copy_from_user(), copy_to_user()
#include <linux/slab.h> //kmalloc

#define GPIO_6 (6)
#define GPIO_15 (15)
#define GPIO_5 (5)
#define GPIO_23 (23)
#define GPIO_24 (24)
#define GPIO_17 (17)
#define GPIO_27 (27)
#define GPIO_22 (22)

/*************** Driver functions **********************/
static int led_open(struct inode *inode, struct file *file);
static int led_release(struct inode *inode, struct file *file);
static ssize_t led_read(struct file *filp, 
char __user *buf, size_t len,loff_t * off);
static ssize_t led_write(struct file *filp, 
const char *buf, size_t len, loff_t * off);
/******************************************************/

struct file_operations led_fops =
{ 
	.read = led_read, 
	.write = led_write, 
	.open = led_open, 
	.release = led_release
};

dev_t dev = 0; 					// device number (包含major&minor)
static struct cdev led_cdev;	// character device
static struct class *dev_class;	// device class

// File Operations
// driver to user
static ssize_t led_read(struct file *fp, char *buf, size_t len, loff_t *fpos) { 
	uint8_t gpio_state = 0;
	
	//reading GPIO value
	gpio_state = gpio_get_value(GPIO_23);
	
	//write to user
	len = 1;
	if(copy_to_user(buf, &gpio_state, len) > 0)
	{
		pr_err("ERROR: Not all the bytes have been copied to user\n");
	}
	
	pr_info("Read function : GPIO_23 = %d \n", gpio_state);
	
	return 0;
} 

// user to driver
int rec_buf[1] = {0};
int data = 0;
static ssize_t led_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{	
    if(copy_from_user(rec_buf, buf, len) > 0)
    {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
        return -EFAULT;
    }

	data = rec_buf[0];
	pr_err("recubf = %d\n" , data);
    if(data == 0)
    {
		gpio_set_value(GPIO_6, 0);
		gpio_set_value(GPIO_15, 0);
		gpio_set_value(GPIO_5, 0);
		gpio_set_value(GPIO_23, 0);
		gpio_set_value(GPIO_24, 0);
		gpio_set_value(GPIO_17, 0);
		gpio_set_value(GPIO_27, 0);
		gpio_set_value(GPIO_22, 0);
    }

    if(data == 1)
    {
		gpio_set_value(GPIO_6, 0);
		gpio_set_value(GPIO_15, 0);
		gpio_set_value(GPIO_5, 0);
		gpio_set_value(GPIO_23, 0);
		gpio_set_value(GPIO_24, 0);
		gpio_set_value(GPIO_17, 0);
		gpio_set_value(GPIO_27, 0);
		gpio_set_value(GPIO_22, 1);
    }

    if(data == 2)
    {
		gpio_set_value(GPIO_6, 0);
		gpio_set_value(GPIO_15, 0);
		gpio_set_value(GPIO_5, 0);
		gpio_set_value(GPIO_23, 0);
		gpio_set_value(GPIO_24, 0);
		gpio_set_value(GPIO_17, 0);
		gpio_set_value(GPIO_27, 1);
		gpio_set_value(GPIO_22, 1);
    }

    if(data == 3)
    {
		gpio_set_value(GPIO_6, 0);
		gpio_set_value(GPIO_15, 0);
		gpio_set_value(GPIO_5, 0);
		gpio_set_value(GPIO_23, 0);
		gpio_set_value(GPIO_24, 0);
		gpio_set_value(GPIO_17, 1);
		gpio_set_value(GPIO_27, 1);
		gpio_set_value(GPIO_22, 1);
    }

    if(data == 4)
    {
		gpio_set_value(GPIO_6, 0);
		gpio_set_value(GPIO_15, 0);
		gpio_set_value(GPIO_5, 0);
		gpio_set_value(GPIO_23, 0);
		gpio_set_value(GPIO_24, 1);
		gpio_set_value(GPIO_17, 1);
		gpio_set_value(GPIO_27, 1);
		gpio_set_value(GPIO_22, 1);
    }

    if(data == 5)
    {
		gpio_set_value(GPIO_6, 0);
		gpio_set_value(GPIO_15, 0);
		gpio_set_value(GPIO_5, 0);
		gpio_set_value(GPIO_23, 1);
		gpio_set_value(GPIO_24, 1);
		gpio_set_value(GPIO_17, 1);
		gpio_set_value(GPIO_27, 1);
		gpio_set_value(GPIO_22, 1);
    }

    if(data == 6)
    {
		gpio_set_value(GPIO_6, 0);
		gpio_set_value(GPIO_15, 0);
		gpio_set_value(GPIO_5, 1);
		gpio_set_value(GPIO_23, 1);
		gpio_set_value(GPIO_24, 1);
		gpio_set_value(GPIO_17, 1);
		gpio_set_value(GPIO_27, 1);
		gpio_set_value(GPIO_22, 1);
    }

    if(data == 7)
    {
		gpio_set_value(GPIO_6, 0);
		gpio_set_value(GPIO_15, 1);
		gpio_set_value(GPIO_5, 1);
		gpio_set_value(GPIO_23, 1);
		gpio_set_value(GPIO_24, 1);
		gpio_set_value(GPIO_17, 1);
		gpio_set_value(GPIO_27, 1);
		gpio_set_value(GPIO_22, 1);
    }

    if(data == 8)
    {
		gpio_set_value(GPIO_6, 1);
		gpio_set_value(GPIO_15, 1);
		gpio_set_value(GPIO_5, 1);
		gpio_set_value(GPIO_23, 1);
		gpio_set_value(GPIO_24, 1);
		gpio_set_value(GPIO_17, 1);
		gpio_set_value(GPIO_27, 1);
		gpio_set_value(GPIO_22, 1);
    }
	return len;
}

static int led_open(struct inode *inode, struct file *fp)
{ 
	pr_info("Device File Opened...!!!\n");
	return 0;
}
static int led_release(struct inode *inode, struct file *fp)
{ 
	pr_info("Device File Closed...!!!\n");
	return 0;
}
 
#define DEVICE_NAME "led_Dev"

/* Module Init function */ 
static int led_driver_init(void)
{
	pr_info("%s: %s: call init\n", __FILE__, __func__);
    
    /*Allocating Major number*/ 
	// 由kernel動態分配
	if((alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME)) <0){ 
		pr_err("%s: %s: Cannot allocate major number\n", __FILE__, __func__);
		goto r_unreg; 
	} 
    pr_info("%s: %s: Major = %d Minor = %d \n", __FILE__, __func__,MAJOR(dev), MINOR(dev));
    /*Creating cdev structure*/ 
	// 將cdev的ops指標指到fops
	cdev_init(&led_cdev,&led_fops);

    /*Adding character device to the system*/
	if((cdev_add(&led_cdev,dev,1)) < 0){ 
		pr_err("%s: %s: Cannot add the device to the system\n", __FILE__, __func__);
		goto r_del; 
	}

    /*Creating struct class*/ 
	if((dev_class = class_create(THIS_MODULE,"LED_Bar_class")) == NULL){ 
		pr_err("%s: %s: Cannot create the struct class\n", __FILE__, __func__);
		goto r_class; 
	} 
    
	/*Creating device*/ 
	if((device_create(dev_class,NULL,dev,NULL,"led_Dev")) == NULL){ 
		pr_err("%s: %s: Cannot create the Device\n", __FILE__, __func__);
		goto r_device; 
	} 
    
	//Checking the GPIO is valid or not
	if(gpio_is_valid(GPIO_6) == false)
	{
		pr_err("GPIO %d is not valid\n", GPIO_6);
		goto r_device;
	}

	if(gpio_is_valid(GPIO_15) == false)
	{
		pr_err("GPIO %d is not valid\n", GPIO_15);
		goto r_device;
	}

	if(gpio_is_valid(GPIO_5) == false)
	{
		pr_err("GPIO %d is not valid\n", GPIO_5);
		goto r_device;
	}

	if(gpio_is_valid(GPIO_23) == false)
	{
		pr_err("GPIO %d is not valid\n", GPIO_23);
		goto r_device;
	}

	if(gpio_is_valid(GPIO_24) == false)
	{
		pr_err("GPIO %d is not valid\n", GPIO_24);
		goto r_device;
	}

	if(gpio_is_valid(GPIO_17) == false)
	{
		pr_err("GPIO %d is not valid\n", GPIO_17);
		goto r_device;
	}

	if(gpio_is_valid(GPIO_27) == false)
	{
		pr_err("GPIO %d is not valid\n", GPIO_27);
		goto r_device;
	}

	if(gpio_is_valid(GPIO_22) == false)
	{
		pr_err("GPIO %d is not valid\n", GPIO_22);
		goto r_device;
	}
	
	//Requesting the GPIO
	if(gpio_request(GPIO_6,"GPIO_6") < 0)
	{
		pr_err("ERROR: GPIO %d request\n", GPIO_6);
		goto r_gpio;
	}

	if(gpio_request(GPIO_15,"GPIO_15") < 0)
	{
		pr_err("ERROR: GPIO %d request\n", GPIO_15);
		goto r_gpio;
	}

	if(gpio_request(GPIO_5,"GPIO_5") < 0)
	{
		pr_err("ERROR: GPIO %d request\n", GPIO_5);
		goto r_gpio;
	}

	if(gpio_request(GPIO_23,"GPIO_23") < 0)
	{
		pr_err("ERROR: GPIO %d request\n", GPIO_23);
		goto r_gpio;
	}

	if(gpio_request(GPIO_24,"GPIO_24") < 0)
	{
		pr_err("ERROR: GPIO %d request\n", GPIO_24);
		goto r_gpio;
	}

	if(gpio_request(GPIO_17,"GPIO_17") < 0)
	{
		pr_err("ERROR: GPIO %d request\n", GPIO_17);
		goto r_gpio;
	}

	if(gpio_request(GPIO_27,"GPIO_27") < 0)
	{
		pr_err("ERROR: GPIO %d request\n", GPIO_27);
		goto r_gpio;
	}

	if(gpio_request(GPIO_22,"GPIO_22") < 0)
	{
		pr_err("ERROR: GPIO %d request\n", GPIO_22);
		goto r_gpio;
	}
 
	//configure the GPIO as output
	gpio_direction_output(GPIO_6, 0);
	gpio_direction_output(GPIO_15, 0);
	gpio_direction_output(GPIO_5, 0);
	gpio_direction_output(GPIO_23, 0);
	gpio_direction_output(GPIO_24, 0);
	gpio_direction_output(GPIO_17, 0);
	gpio_direction_output(GPIO_27, 0);
	gpio_direction_output(GPIO_22, 0);
	
	/* Using this call the GPIO 21 will be visible in /sys/class/gpio/
	** Now you can change the gpio values by using below commands also.
	** echo 1 > /sys/class/gpio/gpio21/value (turn ON the LED)
	** echo 0 > /sys/class/gpio/gpio21/value (turn OFF the LED)
	** cat /sys/class/gpio/gpio21/value (read the value LED)
	** 
	** the second argument prevents the direction from being changed.
	*/
	gpio_export(GPIO_6, false);
	gpio_export(GPIO_15, false);
	gpio_export(GPIO_5, false);
	gpio_export(GPIO_23, false);
	gpio_export(GPIO_24, false);
	gpio_export(GPIO_17, false);
	gpio_export(GPIO_27, false);
	gpio_export(GPIO_22, false);
	
	pr_info("Device Driver Insert...Done!!!\n");
	return 0;
	// Error: 註銷流程
	r_gpio:
		gpio_free(GPIO_6);
		gpio_free(GPIO_15);
		gpio_free(GPIO_5);
		gpio_free(GPIO_23);
		gpio_free(GPIO_24);
		gpio_free(GPIO_17);
		gpio_free(GPIO_27);
		gpio_free(GPIO_22);
	r_device: 
		device_destroy(dev_class,dev); 
	r_class: 
		class_destroy(dev_class); 
	r_del: 
		cdev_del(&led_cdev); 
	r_unreg: 
		unregister_chrdev_region(dev, 1); 
	return -1;
} 

static void led_driver_exit(void)
{
    pr_info("%s: %s: call exit\n", __FILE__, __func__);
	// Release the GPIO
	device_destroy(dev_class,dev);
	class_destroy(dev_class); 
	cdev_del(&led_cdev);
	unregister_chrdev_region(dev, 1);
	pr_info("%s: %s: Device Driver Remove...Done!!\n", __FILE__, __func__); 
} 

module_init(led_driver_init);
module_exit(led_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("howard");
MODULE_DESCRIPTION("EOS HW1 led_driver");