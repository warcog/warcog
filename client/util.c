#include "util.h"
#include <xz/xz.h>

uint32_t decompress(void *dest, const void *src, uint32_t dest_size, uint32_t src_len)
{
    //int res;
    struct xz_dec *dec;
    struct xz_buf buf = {
        .in = (void*) src,
        .in_pos = 0,
        .in_size = src_len,

        .out = dest,
        .out_pos = 0,
        .out_size = dest_size,
    };

    xz_crc32_init();

    dec = xz_dec_init(XZ_SINGLE, 0);
    if (!dec)
        return 0;

    xz_dec_run(dec, &buf); //res =
    xz_dec_end(dec);

    /* out_pos is only set on success */
    return buf.out_pos;
}

/* assumes utf8 is valid and argument is correctly used */
int utf8_len(const char *data)
{
    char c;
    int len;

    c = data[0];
    if (!(c & 0x80))
        return 1;

    for (len = 2; (c & 0x20) && len < 6; c = c << 1, len++); //len limit for unrolling?
    return len;
}

int utf8_unlen(const char *data)
{
    int len;
    if (!(*(data - 1) & 0x80))
        return 1;

    for (len = 2; !(*(data - len) & 0x40) && len < 6; len++);
    return len;
}

char* utf8_next(const char *data, uint32_t *res, int *size)
{
    uint32_t ch;
    int len;

    ch = *data++;
    if (!ch)
        return 0;

    len = 1;
    if (!(ch & 0x80))
        goto finish;

    for (; (*data & 0xC0) == 0x80 && len < 6; data++, len++)
        ch = (ch << 6) | (*data & 0x3f);
    ch &= ~(~0u << (1 + len * 5));
finish:
    *res = ch;
    if (size)
        *size = len;
    return (char*) data;
}

//#include <stdio.h>
int unicode_to_utf8(char *p, uint32_t ch)
{
#define f(x, y) ((((x) >> ((y) * 6)) & 0x3f) | 0x80)
#define g(x, y) (((x) >> ((y) * 6)) | (0xFF << (7 - (y))))
    uint32_t c;
    int i, len;
    //printf("start %u %u %u\n", ch, g(ch, 1), f(ch, 0));

    if (ch <= 0x7F) {
        p[0] = ch;
        return 1;
    }

    for (len = 2, c = ch >> 11; c; c = c >> 5, len++);

    //printf("len: %u\n", len);

    i = len - 1;
    *p++ = g(ch, i);
    while (--i >= 0)
        *p++ = f(ch, i);

    return len;
#undef f
#undef g
}
