#include <stdint.h>

uint32_t decompress(void *dest, const void *src, uint32_t dest_size, uint32_t src_len);

int utf8_len(const char *data);
int utf8_unlen(const char *data);
char* utf8_next(const char *data, uint32_t *res, int *size);
int unicode_to_utf8(char *p, uint32_t ch);
