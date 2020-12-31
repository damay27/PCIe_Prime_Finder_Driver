#include "pcie_ctrl.h"

//Add data about supported devices to the module table so the kernel
//knows what devices this drives should be paired with.
MODULE_DEVICE_TABLE(pci, pci_id_array);


//Pointer to the start of the BAR0 address space AFTER it has been 
//mapped into the virtual address space.
char *pci_ptr;
//Data related to bar 0 on the device
unsigned long bar0_size;
unsigned long bar0_start;

u8 interrupt_number;

//Interrupt handler function
static irqreturn_t interrupt_handler(int irq, void *dev) {
    printk(KERN_INFO "INTERRUPT: %d\n", irq);
    return IRQ_HANDLED;
}

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

    //TODO: Add error checking for this.

    //All PCI values are big endian so the conversion to the cpu byte ordering
    //is required to make sure this works on all platforms.
    printk(KERN_INFO "%d\n", be16_to_cpu(vendor_id));

    //Get the start and end addresses of the devices BAR0 memory region.
    pci_ptr_int_start = pci_resource_start(dev, 0);
    pci_ptr_int_end = pci_resource_end(dev, 0);

    bar0_size = pci_ptr_int_end - pci_ptr_int_start;
    bar0_start = pci_ptr_int_start;

    //Map the BAR0 memory region of the device into the virtual address space.
    pci_ptr = (char*) ioremap(pci_ptr_int_start, bar0_size);

        pci_set_master(dev);

        //pci_enable_msi(dev);

        int xxx = pci_alloc_irq_vectors(dev, 1, 1, PCI_IRQ_MSI);
        printk("XXX: %d\n", xxx);

        interrupt_number = pci_irq_vector(dev, 0);
        printk("YYY: %d\n", interrupt_number);

        int zzz = request_irq(interrupt_number, interrupt_handler, IRQF_SHARED, DEVICE_NAME, dev);
        // request_irq(interrupt_number, interrupt_handler, 0, DEVICE_NAME, NULL);
        printk(KERN_INFO "ZZZ %d\n", zzz);
        // pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &interrupt_number);
        // interrupt_number = dev->irq;


    return status;
}

//This function is called when the device is removed.
void pci_remove (struct pci_dev *dev) {
                    free_irq(interrupt_number, dev);
                    pci_free_irq_vectors(dev);
    iounmap(pci_ptr);
    pci_disable_device(dev);
    printk(KERN_INFO "PCI REMOVE\n");
}