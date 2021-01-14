////////////////////////////////////////////////////
//Low-level API
////////////////////////////////////////////////////
int clear_registers(int fd);
uint32_t read_register(int fd, int reg_offset);
int write_register(int fd, int reg_offset, uint32_t value);

////////////////////////////////////////////////////
//High-level API
////////////////////////////////////////////////////
int start_search(int fd, uint32_t start_val);
int check_complete(int fd);
uint32_t read_result(int fd);
uint64_t read_cycle_count(int fd);
int find_prime(int fd, uint32_t start_val, uint32_t *search_result);