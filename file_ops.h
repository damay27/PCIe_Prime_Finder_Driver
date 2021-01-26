#ifndef FILE_OPS_H
#define FILE_OPS_H

#include <linux/fs.h>
#include <linux/completion.h>

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
int mmap(struct file *filep, struct vm_area_struct *vma);

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
ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp);

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
ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);

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
int open (struct inode *inode, struct file *filp);

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
int release(struct inode *inode, struct file *filp);

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
loff_t llseek(struct file *filp, loff_t offset, int whence);

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
long int ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

//This structure holds all of the file operations that the the driver supports
extern const struct file_operations file_ops;

extern struct completion ioctl_completion;

#endif