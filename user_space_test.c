#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>

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

    ///////////////////////////////////////////////////////////////////////////////////////////
    //To run the test using polling uncomment this code and commend the below code
    ///////////////////////////////////////////////////////////////////////////////////////////

    int status;
    
    status = start_search(fd, start_number);
    if(status != 0) {
        printf("Error starting search\n");
        return -1;
    }

    //Busy loop until the prime search completes
    uint32_t complete;
    do {
        status = check_complete(fd, &complete);
        if(status != 0) {
            printf("Error checking search completion\n");
            return -1;
        }
        usleep(250000);
    } while(complete != 1);

    uint64_t cycle_count;
    status = read_cycle_count(fd, &cycle_count);
    if(status != 0) {
        printf("Error reading cycle count\n");
        return -1;
    }
    printf("Cycle count: %lu\n", cycle_count);

    uint32_t result;
    status = read_result(fd, &result);
    if(status != 0) {
        printf("Error reading search result\n");
        return -1;
    }
    printf("Prime search result: %u\n", result);
    

    ///////////////////////////////////////////////////////////////////////////////////////////
    //Test using blocking
    ///////////////////////////////////////////////////////////////////////////////////////////

    // uint32_t prime;
    // find_prime(fd, start_number, &prime);
    // printf("%d\n", prime);

    return 0;
}