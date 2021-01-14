#include "pcie_ctrl.h"
#include "file_ops.h"

//Add data about supported devices to the module table so the kernel
//knows what devices this drives should be paired with.
MODULE_DEVICE_TABLE(pci, pci_id_array);


//Pointer to the start of the BAR0 address space AFTER it has been 
//mapped into the virtual address space.
char *bar0_ptr;
//Data related to bar 0 on the device
unsigned long bar0_size;
unsigned long bar0_start;

u8 interrupt_number;

//Interrupt handler function
static irqreturn_t interrupt_handler(int irq, void *dev) {
    printk(KERN_INFO "INTERRUPT: %d\n", irq);
    complete(&ioctl_completion);
    return IRQ_HANDLED;
}

//This function is called when the driver and device are paired together.
int pci_probe (struct pci_dev *dev, const struct pci_device_id *id) {
    int status;
    u16 vendor_id;

    //Store the addres of both the start and end of the PCIe memory region
    unsigned long bar0_ptr_int_start;
    unsigned long bar0_ptr_int_end;

    printk(KERN_INFO "PCI PROBE\n");
    status = pci_enable_device(dev);

    //Read the vendor ID from the configuration space of the device.
    pci_read_config_word(dev, PCI_VENDOR_ID, &vendor_id);

    //TODO: Add error checking for this.

    //All PCI values are big endian so the conversion to the cpu byte ordering
    //is required to make sure this works on all platforms.
    printk(KERN_INFO "%d\n", be16_to_cpu(vendor_id));

    //Get the start and end addresses of the devices BAR0 memory region.
    bar0_ptr_int_start = pci_resource_start(dev, 0);
    bar0_ptr_int_end = pci_resource_end(dev, 0);

    bar0_size = bar0_ptr_int_end - bar0_ptr_int_start;
    bar0_start = bar0_ptr_int_start;

    //Map the BAR0 memory region of the device into the virtual address space.
    bar0_ptr = (char*) ioremap(bar0_ptr_int_start, bar0_size);

    //Make the device a bus master so that it can raise interrupts
    pci_set_master(dev);

    //Allocate a single interrupt vector
    int vector_count = pci_alloc_irq_vectors(dev, 1, 1, PCI_IRQ_MSI);
    printk(KERN_INFO "Allocated Vector Count: %d\n", vector_count);

    //Get the IRQ number for the vector
    interrupt_number = pci_irq_vector(dev, 0);
    printk(KERN_INFO "Assigned IRO: %d\n", interrupt_number);

    //Attach a handler to the IRQ number
    int irq_request_status = request_irq(interrupt_number, interrupt_handler, IRQF_SHARED, DEVICE_NAME, dev);
    printk(KERN_INFO "IRQ Request Status: %d\n", irq_request_status);



    return status;
}

//This function is called when the device is removed.
void pci_remove (struct pci_dev *dev) {
    free_irq(interrupt_number, dev);
    pci_free_irq_vectors(dev);
    iounmap(bar0_ptr);
    pci_disable_device(dev);
    printk(KERN_INFO "PCI REMOVE\n");
}