#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/device.h>
#include <asm/page.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Prasoon");
MODULE_DESCRIPTION("IOCTL Driver for VA to PA translation and PA writes");

#define DEVICE_NAME "ioctl_device"
#define IOCTL_MAGIC 'X'
#define IOCTL_VA_TO_PA _IOWR(IOCTL_MAGIC, 1, unsigned long)
#define IOCTL_WRITE_PA _IOW(IOCTL_MAGIC, 2, unsigned long)

struct ioctl_data{
    unsigned long paddr;
    unsigned int value;
};

static int major;
static struct class *ioctl_class;
static struct device *ioctl_device;
static unsigned long vaddress, paddress;

static long ioctl_handler(struct file *file, unsigned int cmd, unsigned long arg){
    struct ioctl_data data;
    void *vaddr;
    switch(cmd){
        case IOCTL_VA_TO_PA:
            if(copy_from_user(&vaddress, (void __user *)arg, sizeof(vaddress))){
                return -EFAULT;
            }
            paddress = virt_to_phys((volatile void*)vaddress);
            if(copy_to_user((void __user *)arg, &paddress, sizeof(paddress))){
                return -EFAULT;
            }
            break;
        case IOCTL_WRITE_PA:
            if(copy_from_user(&data, (void __user *)arg, sizeof(data))){
                return -EFAULT;
            }
            vaddr = phys_to_virt(data.paddr);
            *(unsigned int *)vaddr = data.value;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static int ioctl_open(struct inode *inode, struct file *file){
    pr_info("IOCTL device opened.\n");
    return 0;
}

static int ioctl_release(struct inode *inode, struct file *file){
    pr_info("IOCTL device closed.\n");
    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = ioctl_open,
    .release = ioctl_release,
    .unlocked_ioctl = ioctl_handler,
};

static int __init ioctl_init(void){
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if(major < 0){
        pr_err("Failed to register character device.\n");
        return major;
    }
    ioctl_class = class_create(THIS_MODULE, "ioctl_class");
    if(IS_ERR(ioctl_class)){
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(ioctl_class);
    }
    ioctl_device = device_create(ioctl_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if(IS_ERR(ioctl_device)){
        class_destroy(ioctl_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(ioctl_device);
    }
    pr_info("IOCTL driver loaded.\n");
    return 0;
}

static void __exit ioctl_exit(void){
    device_destroy(ioctl_class, MKDEV(major, 0));
    class_destroy(ioctl_class);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("IOCTL driver unloaded.\n");
}

module_init(ioctl_init);
module_exit(ioctl_exit);

