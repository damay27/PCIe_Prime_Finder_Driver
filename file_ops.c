#include "file_ops.h"
#include "device_specific.h"
#include "pcie_ctrl.h"

#include <linux/uaccess.h>
#include <linux/slab.h>


const struct file_operations file_ops = {
    .owner = THIS_MODULE,
    .read = read,
    .write = write,
    .llseek = llseek,
    .open = open,
    .release = release,
    .mmap = mmap,
};



int mmap(struct file *filep, struct vm_area_struct *vma) {
    int status;
    unsigned long off = vma->vm_pgoff << PAGE_SHIFT;
    
    vma->vm_flags = VM_IO | VM_DONTEXPAND | VM_DONTDUMP;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    status = io_remap_pfn_range(vma, vma->vm_start, (bar0_start+off)>>PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot);
    printk("MMAP STATUS: %d\n", status);
    printk("%lu\n", vma->vm_start);
    return status;
}

ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp) {
    //The way that this works is that the offset is not "remembered" between function calls
    unsigned int val;
    unsigned long not_copied_count;

    if(!access_ok(buff, count)) {
        printk(KERN_INFO "Read buffer error\n");
        return -1;
    }

    // printk(KERN_INFO "READ\n");
    // printk(KERN_INFO "READ COUNT: %ld\n", count);
    // printk(KERN_INFO "READ OFFSET: %lld", *offp);

    val = ioread32(bar0_ptr + AXI_OFFSET + *offp);

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


    iowrite32(kernel_ptr[0], bar0_ptr + AXI_OFFSET + *offp);

    kfree(kernel_ptr);

    *offp += count;

    return (count - not_copied_count);
}

int open (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "File Opened\n");

    printk("INTERRUPT NUMBER: %d\n", interrupt_number);
                // int x = request_irq(interrupt_number, interrupt_handler, 0, DEVICE_NAME, NULL);
                // // request_irq(interrupt_number, interrupt_handler, 0, DEVICE_NAME, NULL);
                // printk(KERN_INFO "XXXXX %d\n", x);
    //TODO: Error handling for irq_request here

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
