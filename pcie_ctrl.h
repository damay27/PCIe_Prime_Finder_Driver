#ifndef PCIE_CTRL_H
#define PCIE_CTRL_H

#include "device_specific.h"

#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/module.h>

//Interrupt handler function
static irqreturn_t interrupt_handler(int irq, void *dev);

//This function is called when the driver and device are paired together.
int pci_probe (struct pci_dev *dev, const struct pci_device_id *id);

//This function is called when the device is removed.
void pci_remove (struct pci_dev *dev);

//This array contains the several PCI device id structures. These structures
//have several feilds but in this case only the vendor id and device id are used.
//PCI_DEVICE is a helper macro for initializing a structure instance.
//It is important that this array end with a NULL entry which in this case
//is {0, }
static struct pci_device_id pci_id_array[] = {
    { PCI_DEVICE(LITEFURY_VENDOR_ID, LITEFURY_DEVICE_ID)},
    { 0, }    
};


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



//Pointer to the start of the BAR0 address space AFTER it has been 
//mapped into the virtual address space.
extern char *bar0_ptr;
//Data related to bar 0 on the device
extern unsigned long bar0_size;
extern unsigned long bar0_start;

extern u8 interrupt_number;

#endif