#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>

#include "prime.h"

int main(int argc, char *argv[]) {
    int count;

    //Open the device file and check that it was opened correctly
    int fd = open("/dev/prime_finder", O_RDWR);
    if(fd < 0) {
        printf("Failed to open device file\n");
        return -1;
    }


    //Determine the number that the prime number search should start from
    //If a start number was provided on the command line then use that
    unsigned int start_number;
    if(argc >= 2) {
        start_number = atol(argv[1]);
    }
    //Otherwise ask the user to provide one
    else {
        printf("Enter the start number: ");
        scanf("%ud", &start_number);
    }

    //Clear all of the registers on the device and then start the prime number search
    clear_registers(fd);
    start_search(fd, start_number);

    //Busy loop until the prime search completes
    while(check_complete(fd) != 1) {
        usleep(250000);
    }

    uint64_t cycle_count = read_cycle_count(fd);
    printf("Cycle count: %lu\n", cycle_count);

    //Read the result back from the device and print it
    printf("%u\n", read_result(fd));

    return 0;
}