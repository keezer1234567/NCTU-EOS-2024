#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h> // copy_from_user(), copy_to_user()
#include <linux/slab.h>    // kmalloc
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>

#define DEVICE_NAME "button_driver"
#define BUFFER_SIZE 10000

// GPIO pins for buttons
#define GPIO_BTN_2 2
#define GPIO_BTN_3 3
#define GPIO_BTN_4 4
#define GPIO_BTN_5 5
#define GPIO_BTN_6 6

static int gpio_buttons[] = {GPIO_BTN_2, GPIO_BTN_3, GPIO_BTN_4, GPIO_BTN_5, GPIO_BTN_6};
static const char *button_names[] = {"BUTTON_1", "BUTTON_2", "BUTTON_3", "BUTTON_4", "BUTTON_5"};

// Shared buffer
static char button_buffer[BUFFER_SIZE];
static int buffer_pos = 0;
static int irq_numbers[5];
static spinlock_t buffer_lock;

// Device variables
static dev_t dev;
static struct cdev button_cdev;
static struct class *dev_class;

/*************** ISR (Interrupt Service Routine) ******************/
static unsigned long last_jiffies[5] = {0};

static irqreturn_t button_isr(int irq, void *dev_id)
{
    unsigned long flags;
    unsigned long current_jiffies = jiffies;
    int gpio = *(int *)dev_id;

    for(int i = 0; i < 5; i++)
    {
        if(gpio == gpio_buttons[i])
        {
            // 檢查去抖時間
            if(time_before(current_jiffies, last_jiffies[i] + msecs_to_jiffies(100)))
            {
                return IRQ_HANDLED; // 忽略抖動信號
            }
            last_jiffies[i] = current_jiffies;
            spin_lock_irqsave(&buffer_lock, flags);
            buffer_pos += snprintf(button_buffer + buffer_pos, 10, "%s\n", button_names[i]);
            spin_unlock_irqrestore(&buffer_lock, flags);
            pr_info("%s\n", button_names[i]);
        }
    }
    return IRQ_HANDLED;
}

/*************** File Operations ******************/
static ssize_t button_read(struct file *fp, char *buf, size_t len, loff_t *fpos)
{
    unsigned long flags;
    ssize_t ret_len;

    spin_lock_irqsave(&buffer_lock, flags);
    if(*fpos >= buffer_pos)
    {
        spin_unlock_irqrestore(&buffer_lock, flags);
        return 0; // End of file
    }

    ret_len = simple_read_from_buffer(buf, len, fpos, button_buffer, buffer_pos);
    spin_unlock_irqrestore(&buffer_lock, flags);

    return ret_len;
}

static ssize_t button_write(struct file *fp, const char __user *buf, size_t len, loff_t *fpos)
{
    char command[10];
    unsigned long flags;

    // 確保 command array 不會 overflow
    if(len > 9)
    {
        len = 9;
    }

    if(copy_from_user(command, buf, len))
    {
        return -EFAULT;
    }

    command[len] = '\0';

    for(int i = 0; i < 5; i++)
    {
        disable_irq(irq_numbers[i]);
    }

    if(strncmp(command, "clear", 5) == 0)
    {
        // 清空 buffer
        spin_lock_irqsave(&buffer_lock, flags);
        memset(button_buffer, 0, BUFFER_SIZE);
        buffer_pos = 0;

        for(int i = 0; i < 5; i++)
        {
            last_jiffies[i] = 0;
        }

        spin_unlock_irqrestore(&buffer_lock, flags);
        pr_info("Driver buffer cleared (use write).\n");
        return len;
    }
    return -EINVAL; // 無效的指令
}

static int button_open(struct inode *inode, struct file *file)
{
    for(int i = 0; i < 5; i++)
    {
        enable_irq(irq_numbers[i]);
    }
    pr_info("Device File Opened...!!!\n");
    return 0;
}

static int button_release(struct inode *inode, struct file *file)
{
    // unsigned long flags;
    // spin_lock_irqsave(&buffer_lock, flags);
    // memset(button_buffer, 0, BUFFER_SIZE);
    // buffer_pos = 0;

    // for(int i = 0; i < 5; i++)
    // {
    //     last_jiffies[i] = 0;
    // }

    // spin_unlock_irqrestore(&buffer_lock, flags);
    // pr_info("Driver buffer cleared (use release).\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = button_read,
    .open = button_open,
    .write = button_write,
    .release = button_release,
};

/*************** Driver Init and Exit ******************/
static int __init button_driver_init(void)
{
    int i, ret;

    pr_info("%s: Initializing the Button Driver\n", DEVICE_NAME);
    spin_lock_init(&buffer_lock);

    // Allocate device number
    if(alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0)
    {
        pr_err("Cannot allocate major number\n");
        return -1;
    }

    // Initialize and add character device
    cdev_init(&button_cdev, &fops);
    if(cdev_add(&button_cdev, dev, 1) < 0)
    {
        pr_err("Cannot add cdev\n");
        goto r_unreg;
    }

    // Create class and device
    if((dev_class = class_create(THIS_MODULE, "button_class")) == NULL)
    {
        pr_err("Cannot create class\n");
        goto r_del;
    }
    if(device_create(dev_class, NULL, dev, NULL, DEVICE_NAME) == NULL)
    {
        pr_err("Cannot create device\n");
        goto r_class;
    }

    // Request GPIOs and IRQs
    for(i = 0; i < 5; i++)
    {
        if(!gpio_is_valid(gpio_buttons[i]))
        {
            pr_err("GPIO %d is not valid\n", gpio_buttons[i]);
            goto r_gpio;
        }
        gpio_request(gpio_buttons[i], button_names[i]);
        gpio_direction_input(gpio_buttons[i]);


        irq_numbers[i] = gpio_to_irq(gpio_buttons[i]);
        pr_info("GPIO %d mapped to IRQ %d\n", gpio_buttons[i], irq_numbers[i]);

        ret = request_irq(irq_numbers[i], button_isr, IRQF_TRIGGER_FALLING, button_names[i], &gpio_buttons[i]);

        if(ret)
        {
            pr_err("Failed to request IRQ %d for GPIO %d\n", irq_numbers[i], gpio_buttons[i]);
            goto r_irq;
        }
    }

    pr_info("Button Driver Loaded Successfully\n");
    return 0;

r_irq:
    for(i = 0; i < 5; i++)
    {
        free_irq(irq_numbers[i], &gpio_buttons[i]);
    }
r_gpio:
    for(i = 0; i < 5; i++)
    {
        gpio_free(gpio_buttons[i]);
    }
r_class:
    class_destroy(dev_class);
r_del:
    cdev_del(&button_cdev);
r_unreg:
    unregister_chrdev_region(dev, 1);
    return -1;
}

static void __exit button_driver_exit(void)
{
    int i;
    for(i = 0; i < 5; i++)
    {
        free_irq(irq_numbers[i], &gpio_buttons[i]);
        gpio_free(gpio_buttons[i]);
    }
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&button_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Button Driver Removed Successfully\n");
}

module_init(button_driver_init);
module_exit(button_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Howard");
MODULE_DESCRIPTION("EOS Button Driver");