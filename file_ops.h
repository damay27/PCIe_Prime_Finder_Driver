#ifndef FILE_OPS_H
#define FILE_OPS_H

#include <linux/fs.h>

int mmap(struct file *filep, struct vm_area_struct *vma);
ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);
int open (struct inode *inode, struct file *filp);
int release(struct inode *inode, struct file *filp);
loff_t llseek(struct file *filp, loff_t offset, int whence);

//This structure holds all of the file operations that the the driver supports
extern const struct file_operations file_ops;

#endif