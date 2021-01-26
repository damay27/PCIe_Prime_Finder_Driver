#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <asm/byteorder.h>
#include <linux/mm.h>

//Defines macros for each register in the prime finder device
#include "file_ops.h"
#include "pcie_ctrl.h"

//Device major and minor numbers
dev_t char_device_numbers;

//Character device representation within the kernel.
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
    //This function will backout the setup steps in the reverse order that they occured
    back_out_char_device();
    printk(KERN_INFO "Shutdown Complete\n");
}

module_init(startup);
module_exit(shutdown);

MODULE_LICENSE("MIT");