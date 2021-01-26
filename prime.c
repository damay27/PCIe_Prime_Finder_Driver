#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "device_specific.h"

////////////////////////////////////////////////////
//Low-level API
////////////////////////////////////////////////////

/*
    Write zero to each of the user writable registers on the device.

    Parameters:
        fd  -> File descriptor of the device file.

    Return:
        0 on success and a negative value otherwise.
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

/*
    Reads the value of the register at a given offset.
    Paramaters:
        fd          -> File descriptor of the device file.
        reg_offset  -> Offset of the register to be read.
        value       -> Pointer to where the value read from
                       the register should be stored.
    Return:
        0 on success and a negative value otherwise.
*/
int read_register(int fd, int reg_offset, uint32_t *value) {
    int read_count;

    //Move to the offset of the register
    lseek(fd, reg_offset, SEEK_SET);

    //Read the value
    read_count = read(fd, value, sizeof(uint32_t));
    //Check that the correct amount of data was read. (32 bit == 4 bytes)
    if(read_count != 4) return -1;

    return 0;
}

/*
    Writes a given value to the register at a given offset.
    Paramaters:
        fd          -> File descriptor of the device file.
        reg_offset  -> Offset of the register to be written to.
        value       -> Value to be written to the register.
    Return:
        0 on success and a negative value otherwise.
*/
int write_register(int fd, int reg_offset, uint32_t value) {
    
    int write_count;

    //Move to the correct register offset
    lseek(fd, reg_offset, SEEK_SET);
    //Write the value
    write_count = write(fd, &value, sizeof(uint32_t));

    //Check that the correct amount of data was written. (32 bit == 4 bytes)
    if(write_count != 4) return -1;
    else return 0;

}

////////////////////////////////////////////////////
//High-level API
////////////////////////////////////////////////////


/*
    Starts a search by writting to the start search register
    on the device.

    Paramaters:
        fd              -> File descriptor of the drivers device file.
        start_val       -> Value to start the prime search from.
    Return:
        On success zero is returned, on failure a negative
        value is returned.
*/
int start_search(int fd, uint32_t start_val) {
    const uint32_t start_flag = 1;
    int status;

    status = write_register(fd, START_NUMBER, start_val);
    if(status == -1) return -1;
    status = write_register(fd, START_FLAG, start_flag);
    if(status == -1) return -1;

    return 0;
}

/*
    Checks if a previously startes search has completed.

    Paramaters:
        fd              -> File descriptor of the drivers device file.
        search_status   -> Pointer to where the result of the query
                           should be stored.

    Return:
        On success zero is returned, on failure a negative
        value is returned.
*/
int check_complete(int fd, uint32_t *search_status) {
    uint32_t flag_register_val;
    int status = read_register(fd, DONE_FLAG, &flag_register_val);

    if(status != 0) {
        return -1;
    }

    if(flag_register_val == 1) {
        *search_status = 1;
    }
    else {
        *search_status = 0;
    }

    return 0;
}

/*
    Reads the result of the prime number search from the devices
    result register.

    Paramaters:
        fd              -> File descriptor of the drivers device file.
        result          -> Pointer to where the result of the query
                           should be stored.

    Return:
        On success zero is returned, on failure a negative
        value is returned.
*/
int read_result(int fd, uint32_t *result) {
    return read_register(fd, PRIME_NUMBER, result);
}


/*
    Reads the number of cycles taken to complete the previous prime
    number search.

    Paramaters:
        fd              -> File descriptor of the drivers device file.
        cycles          -> Pointer to where the result of the query
                           should be stored.

    Return:
        On success zero is returned, on failure a negative
        value is returned.
*/
int read_cycle_count(int fd, uint64_t *cycles) {
    uint32_t upper_bits = 0, lower_bits = 0;
    int status = 0;

    status = read_register(fd, CYCLE_COUNT_HIGH, &upper_bits);
    if(status != 0) {
        return -1;
    }

    status = read_register(fd, CYCLE_COUNT_LOW, &lower_bits);
    if(status != 0) {
        return -1;
    }

    //Combine the upper and lower register values
    *cycles = ( ((uint64_t)upper_bits << 32) | lower_bits );

    return 0;
}

//This scruct is defined here since it should not be used outside
//of this file. This structure is mirrored in file_ops.c but uses
//the kernels internal integer definitions (u32).
struct ioctl_struct {
    uint32_t start_val;
    uint32_t search_result;
};

/*
    Starts a blocking prime search where the search completion will be
    signaled by an interrupt.

    Paramaters:
        fd              -> File descriptor of the drivers device file.
        search_result   -> Pointer to where the result of the search
                           should be stored.
    Return:
        On success zero is returned, on failure a negative
        value is returned.
*/
int find_prime(int fd, uint32_t start_val, uint32_t *search_result) {
    int status;

    //Fill in the start value field of the structure
    struct ioctl_struct user_space_struct;
    user_space_struct.start_val = start_val;

    //This function will block until the device raises an
    //interrupt to indicate the search is complete.
    status = ioctl(fd, 0, &user_space_struct);

    if(status == 0) {
        //Retreive the search result from the structure.
        *search_result = user_space_struct.search_result;
        return 0;
    }
    else {
        return -1;
    }
}