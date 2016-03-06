#include <stdint.h>
#include <stdbool.h>

#define no_default default: __builtin_unreachable()

bool cmpbit(uint8_t a, uint8_t b, uint8_t bit);
uint64_t checksum(void *data, uint32_t len); /* data 8 byte aligned */
void* file_raw(const char *path, uint32_t *ret_len);
bool file_read(const char *path, void *dest, uint32_t len);
