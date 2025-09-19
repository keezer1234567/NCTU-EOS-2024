#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h> // copy_from user()、copy_to user()

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
static int __init my_driver_init(void);
static void __exit my_driver_exit(void);
/*************** Driver functions **********************/
static int my_open(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t my_write(struct file *filp, const char *buf, size_t len, loff_t * off);
/******************************************************/

//File operation structure
struct file_operations fops = 
{
    .open = my_open,
    .read = my_read,
    .write = my_write,
};

static char data[16];
uint16_t seg_for_c[27] = 
{
    0b1111001100010001, // A
    0b0000011100000101, // b
    0b1100111100000000, // C
    0b0000011001000101, // d
    0b1000011100000001, // E
    0b1000001100000001, // F
    0b1001111100010000, // G
    0b0011001100010001, // H
    0b1100110001000100, // I
    0b1100010001000100, // J
    0b0000000001101100, // K
    0b0000111100000000, // L
    0b0011001110100000, // M
    0b0011001110001000, // N
    0b1111111100000000, // O
    0b1000001101000001, // P
    0b0111000001010000, // q
    0b1110001100011001, // R
    0b1101110100010001, // S
    0b1100000001000100, // T
    0b0011111100000000, // U
    0b0000001100100010, // V
    0b0011001100001010, // W
    0b0000000010101010, // X
    0b0000000010100100, // Y
    0b1100110000100010, // Z
    0b0000000000000000
};

static int my_open(struct inode *inode, struct file *fp)
{
    printk("call open\n");
    return 0;
}

static ssize_t my_read(struct file *fp, char *buf, size_t len, loff_t *fpos) 
{
    if(copy_to_user(buf, data, len) > 0)
    {
        pr_err("driver cannot write data to reader");
    }
    return len;
}

static ssize_t my_write(struct file *fp,const char *buf, size_t len, loff_t *fpos) 
{
    char letter[1];
    if(copy_from_user(letter, buf, len ) > 0) 
    {
		pr_err("writer cannot write data to driver\n");
    }

	int index = letter[0] - 'A';
	uint16_t ch = seg_for_c[index];

    uint16_t input_value = seg_for_c[index];
    for(int i = 15; i >= 0; i--)
    {
        if(((input_value >> i) & 1) == 1)
        {
            data[15 - i] = '1';
        }
        else
        {
            data[15 - i] = '0';
        }
    }
    return len;
}
static int __init my_driver_init(void)
{
    printk("call init\n");

    /*Allocating Major number*/
    if((alloc_chrdev_region(&dev, 0, 1, "my_dev")) <0)
	{
		pr_err("Cannot allocate major number\n");
		goto r_unreg;
	}
	pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

	cdev_init(&etx_cdev,&fops); /*Creating cdev structure*/

    /*Adding character device to the system*/
	if((cdev_add(&etx_cdev,dev,1)) < 0)
	{
		pr_err("Cannot add the device to the system\n");
		goto r_del;
	}

     /*Creating struct class*/
	if((dev_class = class_create(THIS_MODULE,"etx_class")) == NULL)
	{
		pr_err("Cannot create the struct class\n");
		goto r_class;
	}

    // 註冊設備
	if((device_create(dev_class,NULL,dev,NULL,"my_dev")) == NULL)
	{
		pr_err( "Cannot create the Device \n");
		goto r_device;
	}

    return 0;

	r_device:
	device_destroy(dev_class,dev);
	r_class:
	class_destroy(dev_class);
	r_del:
	cdev_del(&etx_cdev);
	r_unreg:
	unregister_chrdev_region(dev,1);
	return -1;
}

static void __exit my_driver_exit(void) 
{
    device_destroy(dev_class,dev);
	class_destroy(dev_class);
	cdev_del(&etx_cdev);
	unregister_chrdev_region(dev, 1);
	pr_info("call exit\n");
}

module_init(my_driver_init);
module_exit(my_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("howard");
MODULE_DESCRIPTION("EOS Lab4");