#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/cdev.h>

MODULE_LICENSE("MIT");

#define DRIVER_NAME "ivshmem_plain"
#define VENDOR_ID 0x1AF4
#define DEVICE_ID 0x1110

static struct pci_device_id ivshmem_ids[ ] = {
    {PCI_DEVICE(VENDOR_ID, DEVICE_ID)},
    {0, },
};

MODULE_DEVICE_TABLE(pci, ivshmem_ids);

// pci driver vars
static void *shmem_addr;
static size_t mem_size;
// chardev vars
static dev_t dev_num;

int ivshmem_probe(struct pci_dev *dev, const struct pci_device_id *id){
    int ret;
    unsigned int shmem_addr_end, shmem_addr_start;
    ret = pci_enable_device(dev);
    if(ret){
        printk(KERN_ERR "IVSHMEM_PLAIN: Error when enable device %d\n", ret);
        goto enable_err;
    }

    shmem_addr_start = pci_resource_start(dev, 2);
    shmem_addr_end = pci_resource_end(dev, 2);
    mem_size = (size_t)(shmem_addr_end - shmem_addr_start + 1);
    printk(KERN_INFO "IVSHMEM_PLAIN: Available mem %ld (start=%x, end=%x)", mem_size, shmem_addr_start, shmem_addr_end);
    if(mem_size == 0){
        goto mem_err;
    }
    shmem_addr = pci_iomap(dev, 2, mem_size);
    if(!shmem_addr){
        goto mem_err;
    }

    printk(KERN_INFO "IVSHMEM_PLAIN: Enable device");

    return 0;

mem_err:
    pci_disable_device(dev);
enable_err:
    return ret;
}

ssize_t ivshmem_read(struct file *filp, char __user *buff, size_t count, loff_t *offp){
    uint32_t num1, num2, num3;
    num1 = readl(shmem_addr);
    num2 = readl(shmem_addr + 4);
    num3 = readl(shmem_addr + 8);
    printk(KERN_INFO "Numbers are: %d %d %d\n", num1, num2, num3);
    return filp->private_data--;
}

int ivshmem_open(struct inode *inode, struct file *filp){
    filp->private_data = 1;
    printk(KERN_INFO "IVSHMEM_PLAIN: Chardev is open");
    return 0;
}

int ivshmem_release(struct inode *inode, struct file *filp){
    printk(KERN_INFO "IVSHMEM_PLAIN: Chardev is close");
    return 0;
}

struct file_operations ivshmem_fops = {
    .owner = THIS_MODULE,
    .open = ivshmem_open,
    .release = ivshmem_release,
    .read = ivshmem_read,
};

struct ivshmem_cdev {
    struct semaphore sem;
    struct cdev cdev;
};

struct ivshmem_cdev icdev;

void ivshmem_remove(struct pci_dev *dev){
    cdev_del(&icdev.cdev);
    unregister_chrdev_region(dev_num, 1);
    pci_iounmap(dev, shmem_addr);
    pci_disable_device(dev);
    printk(KERN_INFO "IVSHMEM_PLAIN: Remove device");
}

static struct pci_driver pci_driver = {
    .name = DRIVER_NAME,
    .id_table = ivshmem_ids,
    .probe = ivshmem_probe,
    .remove = ivshmem_remove,
};


static int __init ivshmem_init(void){
    int ret;
    printk(KERN_DEBUG "IVSHMEM_PLAIN: Init\n");
    ret = pci_register_driver(&pci_driver);
    if(ret){
        printk(KERN_INFO "IVSHMEM_PLAIN: Error on pci register %d\n", ret);
    }
    ret = alloc_chrdev_region(&dev_num, 0, 1, DRIVER_NAME);
    if(ret < 0){
        printk(KERN_INFO "IVSHMEM_PLAIN: Failed to alloc cdev %d\n", ret);
        goto unreg_pci;
    }
    printk(KERN_INFO "IVSHMEM_PLAIN: Chardev major number is %d\n", MAJOR(dev_num));
    cdev_init(&icdev.cdev, &ivshmem_fops);
    icdev.cdev.owner = THIS_MODULE;
    icdev.cdev.ops = &ivshmem_fops;
    ret = cdev_add(&icdev.cdev, dev_num, 1);
    if(ret){
        printk(KERN_INFO "IVSHMEM_PLAIN: Failed add cdev %d\n", ret);
        goto unreg_cdev;
    }

    return 0;
unreg_cdev:
    unregister_chrdev_region(dev_num, 1);
unreg_pci:
    pci_unregister_driver(&pci_driver);
error_ret:
    return ret;
}

static void __exit ivshmem_exit(void){
    pci_unregister_driver(&pci_driver);
    printk(KERN_DEBUG "IVSHMEM_PLAIN: Exit\n");
}

module_init(ivshmem_init);
module_exit(ivshmem_exit);
