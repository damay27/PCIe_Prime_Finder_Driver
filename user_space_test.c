#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

//These hold the byte offsets of each register
#define START_FLAG 0
#define START_NUMBER 4
#define DONE_FLAG 8
#define PRIME_NUMBER 12

int clear_registers(int fd) {
    uint32_t data = 0;
    int status;

    lseek(fd, START_FLAG, SEEK_SET);
    status = write(fd, &data, sizeof(data));
    if(status == -1) return -1;
    status = write(fd, &data, sizeof(data));
    if(status == -1) return -1;

    return 0;
}

int start_search(int fd, uint32_t start_val) {
    const uint32_t start_flag = 1;
    int status;

    lseek(fd, 4, SEEK_SET);
    status = write(fd, &start_val, sizeof(start_val));
    if(status == -1) return -1;
    lseek(fd, 0, SEEK_SET);
    status = write(fd, &start_flag, sizeof(start_flag));
    if(status == -1) return -1;

    return 0;
}

uint32_t read_register(int fd, int reg_offset) {
    //Set to -1 by default so that if reg_select doesn't match
    //any of the registers -1 will get returned.
    uint32_t return_val;
    int read_count;

    // if(reg_select == FLAG_REG) {
    //     lseek(fd, 8, SEEK_SET);
    // }
    // else if (reg_select == PRIME_REG) {
    //     lseek(fd, 12, SEEK_SET);
    // }
    // else {
    //     return -1;
    // }

    lseek(fd, reg_offset, SEEK_SET);

    read_count = read(fd, &return_val, sizeof(return_val));
    if(read_count != 4) return -1;

    return return_val;
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

int main(void) {
    int count;
    int fd = open("/dev/prime_finder", O_RDWR);

    unsigned int start_number;
    printf("Enter the start number: ");
    scanf("%ud", &start_number);

    // lseek(fd, 1, SEEK_CUR);

    uint32_t data[2] = {1, 4};


    printf("%d\n", clear_registers(fd));

    // uint32_t read_val;
    // lseek(fd, 0, SEEK_SET);
    // count = read(fd, &read_val, sizeof(unsigned int));
    printf("%d\n", start_search(fd, start_number));

    while(check_complete(fd) != 1) {
        usleep(250000);
    }
    printf("\n%u\n", read_result(fd));

    // sleep(2);

    // printf("%d\n", check_complete(fd));

    // printf("\n%d\n", read_result(fd));

    // sleep(2);

    // printf("%d\n", read_register(fd, DONE_FLAG));
    // printf("%d\n", read_register(fd, PRIME_NUMBER));

    return 0;
}