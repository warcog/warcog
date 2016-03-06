#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool cmpbit(uint8_t a, uint8_t b, uint8_t bit)
{
    return (a & bit) != (b & bit);
}

uint64_t checksum(void *data, uint32_t len)
{
    uint64_t checksum, v, *d;

    checksum = 0;
    d = data;
    while (len >= 8) {
        checksum ^= *d++;
        len -= 8;
    }

    v = 0;
    memcpy(&v, d, len);
    checksum ^= v;

    return checksum;
}

void* file_raw(const char *path, uint32_t *ret_len)
{
    FILE *file;
    uint32_t len;
    void *data;

    file = fopen(path, "rb");
    if (!file)
        return 0;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = malloc(len);
    if (data)
        len = fread(data, 1, len, file);

    fclose(file);

    if (ret_len)
        *ret_len = len;
    return data;
}

bool file_read(const char *path, void *dest, uint32_t len)
{
    FILE *file;

    file = fopen(path, "rb");
    if (!file)
        return 0;

    fseek(file, 0, SEEK_END);
    if (ftell(file) != len)
        goto fail;
    fseek(file, 0, SEEK_SET);

    if (fread(dest, len, 1, file) != 1)
        goto fail;

    fclose(file);
    return 1;

fail:
    fclose(file);
    return 0;
}
