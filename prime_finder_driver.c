#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>

#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/pci.h>
#include <asm/byteorder.h>

//Defines macros for each register in the prime finder device
#include "device_registers.h"

#define DEVICE_NAME "prime_finder_driver"
#define BUFFER_SIZE 100

#define AXI_OFFSET 0x0000

//Controls whether profiling specific featurs should be included in the driver
#define PROFILING_MODE

#ifdef PROFILING_MODE
long long cycle_start_count = 0;
long long cycle_delta = 0;
#endif

////////////////////////////////////
//PCI stuff
////////////////////////////////////

//Pointer to the start of the BAR0 address space AFTER it has been 
//mapped into the virtual address space.
char *pci_ptr;

//This function is called when the driver and device are paired together.
int pci_probe (struct pci_dev *dev, const struct pci_device_id *id) {
    int status;
    u16 vendor_id;

    //Store the addres of both the start and end of the PCIe memory region
    unsigned long pci_ptr_int_start;
    unsigned long pci_ptr_int_end;

    printk(KERN_INFO "PCI PROBE\n");
    status = pci_enable_device(dev);

    //Read the vendor ID from the configuration space of the device.
    pci_read_config_word(dev, PCI_VENDOR_ID, &vendor_id);
    //All PCI values are big endian so the conversion to the cpu byte ordering
    //is required to make sure this works on all platforms.
    printk(KERN_INFO "%d\n", be16_to_cpu(vendor_id));

    //Get the start and end addresses of the devices BAR0 memory region.
    pci_ptr_int_start = pci_resource_start(dev, 0);
    pci_ptr_int_end = pci_resource_end(dev, 0);

    //Map the BAR0 memory region of the device into the virtual address space.
    pci_ptr = (char*) ioremap(pci_ptr_int_start, pci_ptr_int_end - pci_ptr_int_start);

    return status;
}

//This function is called when the device is removed.
void pci_remove (struct pci_dev *dev) {
    iounmap(pci_ptr);
    pci_disable_device(dev);
    printk(KERN_INFO "PCI REMOVE\n");
}

//This array contains the several PCI device id structures. These structures
//have several feilds but in this case only the vendor id and device id are used.
//PCI_DEVICE is a helper macro for initializing a structure instance.
//It is important that this array end with a NULL entry which in this case
//is {0, }
static struct pci_device_id pci_id_array[] = {
    { PCI_DEVICE(0x10EE, 0x7014)},
    { 0, }    
};

//Add data about supported devices to the module table so the kernel
//knows what devices this drives should be paired with.
MODULE_DEVICE_TABLE(pci, pci_id_array);


//Maps various PCI related functions and values into the struct.
//This is then used to register the driver with the PCI subsystem.
//There are additional feilds in the sturcture but this are the 
//minimum required feilds.
static struct pci_driver pci_driver_struct = {
    .name = DEVICE_NAME,
    .id_table = pci_id_array,
    .probe = pci_probe,
    .remove = pci_remove
};
////////////////////////////////////



//Device major and minor numbers
dev_t char_device_numbers;

//File operations
ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp) {
    //The way that this works is that the offset is not "remembered" between function calls
    unsigned int val;
    unsigned long not_copied_count;

    if(!access_ok(buff, count)) {
        printk(KERN_INFO "Read buffer error\n");
        return -1;
    }

    printk(KERN_INFO "READ\n");
    printk(KERN_INFO "READ COUNT: %ld\n", count);
    printk(KERN_INFO "READ OFFSET: %lld", *offp);

    val = ioread32(pci_ptr + AXI_OFFSET + *offp);

    #ifdef PROFILING_MODE
    //Detect when '1' is written to to the start register
    if(*offp == DONE_FLAG && val == 1) {
        cycle_delta = rdtsc() - cycle_start_count;
        printk(KERN_INFO "Stoping cycle counter: %lld\n", cycle_delta);
    }
    #endif

    *offp += count;

    not_copied_count = copy_to_user(buff, &val, sizeof(unsigned int));
    return (count - not_copied_count);
}


ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp) {

    unsigned long not_copied_count;
    u32 *kernel_ptr;

    //Check that the userspace buffer is valid
    if(!access_ok(buff, count)) {
        printk(KERN_INFO "Write buffer error\n");
        return -1;
    }
    
    //Copy the userspace buffer to a kernel space buffer
    kernel_ptr = (u32*) kmalloc(count * sizeof(char), GFP_KERNEL);
    not_copied_count = copy_from_user(kernel_ptr, buff, count);

    printk(KERN_INFO "WRITE\n");
    printk(KERN_INFO "WRITE COUNT: %ld\n", count);
    printk(KERN_INFO "WRITE OFFSET: %lld", *offp);

    #ifdef PROFILING_MODE
    //Detect when '1' is written to to the start register
    if(*offp == START_FLAG && *kernel_ptr & 1) {
        printk(KERN_INFO "Starting cycle counter\n");
        cycle_start_count = rdtsc();
    }
    #endif


    // iowrite8_rep(pci_ptr + *offp, kernel_ptr, count);
    iowrite32(kernel_ptr[0], pci_ptr + AXI_OFFSET + *offp);

    kfree(kernel_ptr);

    *offp += count;

    return (count - not_copied_count);
}

int open (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "File Opened\n");
    return 0;
}

int release(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "File Closed\n");
    return 0;
}

loff_t llseek(struct file *filp, loff_t offset, int whence) {
    if(whence == SEEK_SET) {
        filp->f_pos = offset;
    }
    else if(whence == SEEK_CUR){
        filp->f_pos += offset;
    }

    printk(KERN_INFO "SEEK\n");
    printk(KERN_INFO "SEEK OFFSET: %lld\n", filp->f_pos);

    return filp->f_pos;
}

//This structure holds all of the file operations that the the driver supports
struct file_operations file_ops = {
    .owner = THIS_MODULE,
    .read = read,
    .write = write,
    .llseek = llseek,
    .open = open,
    .release = release,

};

struct cdev char_device;

//This variable tracks what setup steps have been completed so that these
//steps can be undone in case of an error or when the driver is unloaded
unsigned int setup_status;

void back_out_char_device(void) {

    //Runs through the steps in reverse order that they were done during setup
    switch(setup_status) {
        case 3:
            pci_unregister_driver(&pci_driver_struct);
        case 2:
            cdev_del(&char_device);
        case 1:
            unregister_chrdev_region(char_device_numbers, 1);
    }

}

//Function for seting up the driver
static int __init startup(void) {
    int err;

    //Setup status is declared above and is used to track setup steps
    //so that they can be undone later
    setup_status = 0;

    printk(KERN_INFO "Startup\n");

    //Get major and minor numbers for the charater device
    err = alloc_chrdev_region(&char_device_numbers, 0, 1, DEVICE_NAME);
    if(err < 0) {
        printk(KERN_WARNING "Falid to allocate defice numbers\n");
        back_out_char_device();
        return -1;
    }
    setup_status++;

    //Register the character device
    cdev_init(&char_device, &file_ops);
    // char_device.owner = THIS_MODULE;
    // char_device.ops = &file_ops;
    // char_device.dev = char_device_numbers;
    //Once the character device is added it is considered to be live
    err = cdev_add(&char_device, char_device_numbers, 1);
    if(err < 0) {
        printk(KERN_WARNING "Falid to add the character device\n");
        back_out_char_device();
        return -1;
    }
    setup_status++;

    //Register this driver with the PCI subsystem.
    err = pci_register_driver(&pci_driver_struct);
    if(err < 0) {
        printk("Failed to register PCI device\n");
        back_out_char_device();
        return -1;
    }
    setup_status++;


    printk(KERN_INFO "Startup Complete\n");

    return 0;
}

static void __exit shutdown(void) {
    printk(KERN_INFO "Shutdown\n");
    back_out_char_device();
    printk(KERN_INFO "Shutdown Complete\n");
}

module_init(startup);
module_exit(shutdown);

MODULE_LICENSE("GPL");