#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//Defines macros for each register in the prime finder device
// #include "device_registers.h"
#include "prime.h"

// /*
//     Write zero to each of the user writable registers on the device.

//     Parameters:
//         fd : File descriptor of the device file.
// */
// int clear_registers(int fd) {
//     uint32_t data = 0;
//     int status;

//     //Move the file pointer to the start flag register then write zero to it
//     //and check for errors.
//     lseek(fd, START_FLAG, SEEK_SET);
//     status = write(fd, &data, sizeof(data));
//     if(status == -1) return -1;

//     //There is no need to seek to the next register value since the previous write
//     //operation will already have moved the file pointer
//     status = write(fd, &data, sizeof(data));
//     if(status == -1) return -1;

//     return 0;
// }


// int start_search(int fd, uint32_t start_val) {
//     const uint32_t start_flag = 1;
//     int status;

//     lseek(fd, 4, SEEK_SET);
//     status = write(fd, &start_val, sizeof(start_val));
//     if(status == -1) return -1;
//     lseek(fd, 0, SEEK_SET);
//     status = write(fd, &start_flag, sizeof(start_flag));
//     if(status == -1) return -1;

//     return 0;
// }

// uint32_t read_register(int fd, int reg_offset) {
//     //Set to -1 by default so that if reg_select doesn't match
//     //any of the registers -1 will get returned.
//     uint32_t return_val;
//     int read_count;

//     lseek(fd, reg_offset, SEEK_SET);

//     read_count = read(fd, &return_val, sizeof(return_val));
//     if(read_count != 4) return -1;

//     return return_val;
// }

// int check_complete(int fd) {
//     uint32_t flag_register_val = read_register(fd, DONE_FLAG);
//     if(flag_register_val == 1) {
//         return 1;
//     }
//     else {
//         return 0;
//     }
// }

// uint32_t read_result(int fd) {
//     return read_register(fd, PRIME_NUMBER);
// }

int main(int argc, char *argv[]) {

    //Open the device file and check that it was opened correctly
    int fd = open("/dev/prime_finder", O_RDWR);
    if(fd < 0) {
        printf("Failed to open device file\n");
        return -1;
    }

    int data_file = open("./cycle_count.csv", O_CREAT | O_WRONLY);
    if(data_file < 0) {
        printf("Could not open/create data file\n");
        return -1;
    }

    // //Determine the number that the prime number search should start from
    // //If a start number was provided on the command line then use that
    // unsigned int start_number;
    // if(argc >= 2) {
    //     start_number = atol(argv[1]);
    // }
    // //Otherwise ask the user to provide one
    // else {
    //     printf("Enter the start number: ");
    //     scanf("%ud", &start_number);
    // }

    srand(time(NULL));

    unsigned int count = 0;
    while(count < 1000) {
        
        // printf("-----------------------------------------\n");

        char data_string[128];

        uint32_t start_number = (uint32_t) (rand() % 1000000 + 800000);
        // printf("Start number: %u\n", start_number);

        //Clear all of the registers on the device and then start the prime number search
        clear_registers(fd);
        start_search(fd, start_number);

        //Busy loop until the prime search completes
        while(check_complete(fd) != 1) {
            usleep(250000);
        }

        uint64_t cycle_count = read_cycle_count(fd);
        // printf("Cycle count: %lu\n", cycle_count);

        //Read the result back from the device and print it
        uint32_t prime_number = read_result(fd);
        // printf("Prime number: %u\n", prime_number);

        sprintf(data_string, "%u, %u, %lu\n", start_number, prime_number, cycle_count);
        write(data_file, data_string, strlen(data_string));

        count++;
        printf("%d\n", count);
    }

    close(fd);
    close(data_file);

    return 0;
}