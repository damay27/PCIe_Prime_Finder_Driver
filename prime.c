#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include "device_registers.h"

////////////////////////////////////////////////////
//Low-level API
////////////////////////////////////////////////////

/*
    Write zero to each of the user writable registers on the device.

    Parameters:
        fd : File descriptor of the device file.
*/
int clear_registers(int fd) {
    uint32_t data = 0;
    int status;

    //Move the file pointer to the start flag register then write zero to it
    //and check for errors.
    lseek(fd, START_FLAG, SEEK_SET);
    status = write(fd, &data, sizeof(data));
    if(status == -1) return -1;

    //There is no need to seek to the next register value since the previous write
    //operation will already have moved the file pointer
    status = write(fd, &data, sizeof(data));
    if(status == -1) return -1;

    return 0;
}

uint32_t read_register(int fd, int reg_offset) {
    //Set to -1 by default so that if reg_select doesn't match
    //any of the registers -1 will get returned.
    uint32_t return_val;
    int read_count;

    lseek(fd, reg_offset, SEEK_SET);

    read_count = read(fd, &return_val, sizeof(return_val));
    if(read_count != 4) return -1;

    return return_val;
}

int write_register(int fd, int reg_offset, uint32_t value) {
    
    int write_count;

    lseek(fd, reg_offset, SEEK_SET);
    write_count = write(fd, &value, sizeof(value));

    if(write_count != 4) return -1;
    else return 0;

}

////////////////////////////////////////////////////
//High-level API
////////////////////////////////////////////////////


int start_search(int fd, uint32_t start_val) {
    const uint32_t start_flag = 1;
    int status;

    // lseek(fd, 4, SEEK_SET);
    // status = write(fd, &start_val, sizeof(start_val));
    // if(status == -1) return -1;
    // lseek(fd, 0, SEEK_SET);
    // status = write(fd, &start_flag, sizeof(start_flag));
    // if(status == -1) return -1;

    status = write_register(fd, START_NUMBER, start_val);
    if(status == -1) return -1;
    status = write_register(fd, START_FLAG, start_flag);
    if(status == -1) return -1;

    return 0;
}

int check_complete(int fd) {
    uint32_t flag_register_val = read_register(fd, DONE_FLAG);
    if(flag_register_val == 1) {
        return 1;
    }
    else {
        return 0;
    }
}

uint32_t read_result(int fd) {
    return read_register(fd, PRIME_NUMBER);
}