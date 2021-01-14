#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
// #include <linux/fs.h>
#include <linux/cdev.h>
// #include <linux/slab.h>
// #include <linux/uaccess.h>
#include <linux/semaphore.h>

// #include <linux/types.h>
#include <linux/vmalloc.h>
// #include <linux/pci.h>
#include <asm/byteorder.h>
// #include <linux/interrupt.h>
#include <linux/mm.h>

//Defines macros for each register in the prime finder device
// #include "device_registers.h"
#include "file_ops.h"
#include "pcie_ctrl.h"

// #define DEVICE_NAME "prime_finder_driver"
#define BUFFER_SIZE 100

//Controls whether profiling specific featurs should be included in the driver
#define PROFILING_MODE

#ifdef PROFILING_MODE
long long cycle_start_count = 0;
long long cycle_delta = 0;
#endif

//Device major and minor numbers
dev_t char_device_numbers;

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

                // int x = request_irq(interrupt_number, interrupt_handler, 0, DEVICE_NAME, NULL);
                // int x = request_irq(11, interrupt_handler, 0, DEVICE_NAME, NULL);

                // printk(KERN_INFO "WWWWW %d\n", interrupt_number);
                // printk(KERN_INFO "XXXXX %d\n", x);
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