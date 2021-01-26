#include "file_ops.h"
#include "device_specific.h"
#include "pcie_ctrl.h"

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/types.h>


const struct file_operations file_ops = {
    .owner = THIS_MODULE,
    .read = read,
    .write = write,
    .llseek = llseek,
    .open = open,
    .release = release,
    .mmap = mmap,
    .unlocked_ioctl = ioctl,
};


DECLARE_COMPLETION(ioctl_completion);


//This scruct is defined here since it should not be used outside
//of this file. This structure is mirrored in prime.c but uses
//the stdint.h integer definitions (uint32_t).
struct ioctl_struct {
    u32 start_val;
    u32 search_result;
};


/*
    Function for non-standard I/O and control functions. In this driver
    it is used to activate a blocking prime search where the function
    will wait for the FPGA to finish its search before returning.

    Paramater:
        filp    -> Pointer to the devices file sturcture.
        cmd     -> Command ID. Currently the only valid command ID is 0.
        arg     -> Argument value. What this value represents can change
                   based on use case but in this driver it is a pointer to
                   an ioctl_struct in userspace.

    Return:
        Returns 0 on success and a negative value on failure.
*/
long int ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    
    //Case 0 variables
    u32 start_value;
    int completion_status;
    struct ioctl_struct kernel_space_struct;
    unsigned long not_copied_count;
    struct ioctl_struct __user *user_space_ptr;
    
    //Pring logging data
    printk(KERN_INFO "IOCTL: %d\n", cmd);
    printk(KERN_INFO "%IOCTL ARG: lu\n", arg);


    switch(cmd) {
        
        //0 -> blocking prime search operation
        case 0:

            //The arg is a pointer to a userspace structure containing
            //the start value for the search and an additionaly field for
            //returning the result of the search. 
            user_space_ptr = (struct ioctl_struct*) arg;

            //Check that the userspace pointer is valid
            if(!access_ok(user_space_ptr, sizeof(struct ioctl_struct))) {
                printk(KERN_INFO "Ioctl struct error\n");
                return -1;
            }
            
            //Copy the userspace struct to kernel space
            //not_copied_count = copy_from_user(&kernel_space_struct, user_space_ptr, sizeof(struct ioctl_struct));
            not_copied_count = copy_from_user(&kernel_space_struct, user_space_ptr, sizeof(struct ioctl_struct));

            //Make sure all of the data could be copied
            if(not_copied_count != 0) {
                printk(KERN_INFO "Faild to copy ioctl struct from user space\n");
                return -2;
            }

            start_value = (u32) kernel_space_struct.start_val;
            //Write the start value
            iowrite32(start_value, bar0_ptr + START_NUMBER);
            //Set the start bit
            iowrite32(1, bar0_ptr + START_FLAG);

            //Wait for the interrupt to fire which tells us the task is complete
            if( wait_for_completion_interruptible(&ioctl_completion) != 0 ) {
                return -3;
            }

            //Read back the value and return the result
            kernel_space_struct.search_result = ioread32(bar0_ptr + PRIME_NUMBER);

            //Copy the structure back to user space
            not_copied_count = copy_to_user(user_space_ptr, &kernel_space_struct, sizeof(struct ioctl_struct));

            if(not_copied_count != 0) {
                printk(KERN_INFO "Faild to copy ioctl struct from user space\n");
                return -2;
            }

            return 0;

        default:
            return -1;

    }
}


/*
    Allows the userspace program to map BAR0 into its address space

    Paramaters:
        filep   -> Pointer to the devices file sturcture.
        vma     -> Structure pointer describing the user space
                   processes viritual address region to map
                   BAR0 into.

    Return:
        0 on success and a negative value otherwise.
*/
int mmap(struct file *filep, struct vm_area_struct *vma) {
    int status;

    //Convert the page offset to an address offset
    unsigned long off = vma->vm_pgoff << PAGE_SHIFT;
    
    //The VM_RESERVED flag has been replaced by VM_DONTEXPAND and VM_DONTDUMP in newer kernel versions
    vma->vm_flags = VM_IO | VM_DONTEXPAND | VM_DONTDUMP;

    //Make sure that the memory region is not cached
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    //Actually perform the mapping. NOTE that the offset paramater is in terms of pages which is why the >>PAGE_SHIFT is needed
    //inorder to get back to pages from an address.
    status = io_remap_pfn_range(vma, vma->vm_start, (bar0_start+off)>>PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot);

    //Log the operation
    printk("MMAP STATUS: %d\n", status);
    printk("MMAP START ADDRESS: %lu\n", vma->vm_start);
    return status;
}


/*
    Performs a blocking read from the device's BAR0. NOTE: For simplicity this function
    only reads a single 32bit value regardless of how large the provided buffer is.

    Paramaters:
        filep   -> Pointer to the devices file sturcture.
        buff    -> User space buffer from user space. Cannont be directly accessed
                   in kernel space.
        count   -> Indicates the size of the buffer pointed to by buff.
        offp    -> Offset to read from with in the file. In this case this is
                   the offset to read from with in BAR0.
    Return:
        Returns the number of bytes read during the operation.
*/
ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp) {
    //The way that this works is that the offset is not "remembered" between function calls
    unsigned int val;
    unsigned long not_copied_count;

    //Check that the user space buffer is OK to be used
    if(!access_ok(buff, count)) {
        printk(KERN_INFO "Read buffer error\n");
        return -1;
    }

    //Log the operation in the kernel log
    printk(KERN_INFO "READ\n");
    printk(KERN_INFO "READ OFFSET: %lld", *offp);

    //Read in the value at the provided offset from the BAR0 start.
    val = ioread32(bar0_ptr + *offp);

    //Move the offset by the amount read. This is stored between
    //operations.
    *offp += count;

    not_copied_count = copy_to_user(buff, &val, sizeof(unsigned int));
    return (count - not_copied_count);
}

/*
    Function to write data to the PCIe device's BAR0. NOTE: For simplicity this function
    only writes a single 32bit value regardless of how large the provided buffer is.
    Paramaters:
        filep   -> Pointer to the devices file sturcture.
        buff    -> User space buffer from user space. Cannont be directly accessed
                   in kernel space.
        count   -> Indicates the size of the buffer pointed to by buff.
        offp    -> Offset to write to from with in the file. In this case this is
                   the offset to write to with in BAR0.
    Return:
        Returns the number of bytes written during the operation.
*/
ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp) {

    unsigned long not_copied_count;
    u32 *kernel_ptr;

    //Check that the userspace buffer is valid
    if(!access_ok(buff, count)) {
        printk(KERN_INFO "Write buffer error\n");
        return -1;
    }
    
    //Copy the userspace buffer to a kernel space buffer. This is needed
    //inorder to use the iowrite32 function which needs a kernal space
    //address.
    kernel_ptr = (u32*) kmalloc(count * sizeof(char), GFP_KERNEL);
    not_copied_count = copy_from_user(kernel_ptr, buff, count);

    //Log the operation in the kernel log
    printk(KERN_INFO "WRITE\n");
    printk(KERN_INFO "WRITE OFFSET: %lld", *offp);

    //Read in the value at the provided offset from the BAR0 start.
    iowrite32(kernel_ptr[0], bar0_ptr + *offp);

    //Free the internal buffer
    kfree(kernel_ptr);

    //Move the offset by the amount read. This is stored between
    //operations.
    *offp += count;

    return (count - not_copied_count);
}


/*
    Function to let user space programs open the driver's device file.
    Paramaters:
        inode   -> Pointer to the devices inode sturcture
        filep   -> Pointer to the devices file sturcture
        NOTE: For more detials on these parametrs look up the specific
        structure types.
    Return:
        0 on success negative value on failure.
*/
int open (struct inode *inode, struct file *filp) {
    //Nothing to be done here. Just log that the file was
    //opened and return.
    printk(KERN_INFO "File Opened\n");

    return 0;
}

/*
    Function to let user space programs close the driver's device file.
    Paramaters:
        inode   -> Pointer to the devices inode sturcture
        filep   -> Pointer to the devices file sturcture
        NOTE: For more detials on these parametrs look up the specific
        structure types.
    Return:
        0 on success negative value on failure.
*/
int release(struct inode *inode, struct file *filp) {
    //Nothing to be done here. Just log that the file was
    //closed and return.
    printk(KERN_INFO "File Closed\n");

    return 0;
}

/*
    Allows to set the offset that will be written to or read from.
    Paramaters:
        filep   -> Pointer to the devices file sturcture.
        offset  -> The value used to set the new offset
        whence  -> Indicates where the offset should be set from
                   The options are SEEK_SET or SEEK_CUR. When SEEK_SET
                   is used the offset is set relative to the start
                   position. When SEEK_CUR is used the offset is 
                   set from the current position.
    Return:
        Returns the newly set offset value.
*/
loff_t llseek(struct file *filp, loff_t offset, int whence) {

    //Set the offset relative to the start (In absolute terms).
    if(whence == SEEK_SET) {
        filp->f_pos = offset;
    }
    //Set the offset relative to the current position
    else if(whence == SEEK_CUR){
        filp->f_pos += offset;
    }

    //Log the operation.
    printk(KERN_INFO "SEEK\n");
    printk(KERN_INFO "SEEK OFFSET: %lld\n", filp->f_pos);

    return filp->f_pos;
}

