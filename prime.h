////////////////////////////////////////////////////
//Low-level API
////////////////////////////////////////////////////
int clear_registers(int fd);
int read_register(int fd, int reg_offset, uint32_t *value);
int write_register(int fd, int reg_offset, uint32_t value);

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
int start_search(int fd, uint32_t start_val);

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
int check_complete(int fd, uint32_t *search_status);

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
int read_result(int fd, uint32_t *result);

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
int read_cycle_count(int fd, uint64_t *cycles);

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
int find_prime(int fd, uint32_t start_val, uint32_t *search_result);