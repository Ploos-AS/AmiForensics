#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HUNK_HEADER 0x000003F3UL
#define READ_BUFFER_SIZE 4096

static unsigned long crc32_update(unsigned long crc, const unsigned char *buf, size_t len)
{
    size_t i;
    unsigned int bit;

    crc = crc ^ 0xFFFFFFFFUL;
    for (i = 0; i < len; ++i) {
        crc ^= (unsigned long)buf[i];
        for (bit = 0; bit < 8; ++bit) {
            if (crc & 1UL)
                crc = (crc >> 1) ^ 0xEDB88320UL;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

static unsigned long read_be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) |
           ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) |
           (unsigned long)p[3];
}

static const char *classify_file(const unsigned char *head, size_t head_len)
{
    if (head_len >= 4 && read_be32(head) == HUNK_HEADER)
        return "Amiga HUNK executable";

    if (head_len >= 4 && head[0] == 'D' && head[1] == 'O' &&
        head[2] == 'S' && head[3] <= 7)
        return "Amiga DOS disk image / bootblock data";

    return "unknown/raw";
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <file>\n", prog);
}

int main(int argc, char **argv)
{
    FILE *fp;
    unsigned char buf[READ_BUFFER_SIZE];
    unsigned char head[16];
    size_t n;
    size_t head_len = 0;
    unsigned long total = 0;
    unsigned long crc = 0;

    if (argc != 2) {
        usage(argv[0]);
        return 10;
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        fprintf(stderr, "FileInfo: cannot open '%s'\n", argv[1]);
        return 20;
    }

    while ((n = fread(buf, 1, sizeof(buf), fp)) != 0) {
        size_t copy_len = 0;

        if (head_len < sizeof(head)) {
            copy_len = sizeof(head) - head_len;
            if (copy_len > n)
                copy_len = n;
            memcpy(head + head_len, buf, copy_len);
            head_len += copy_len;
        }

        crc = crc32_update(crc, buf, n);
        total += (unsigned long)n;
    }

    if (ferror(fp)) {
        fprintf(stderr, "FileInfo: read error on '%s'\n", argv[1]);
        fclose(fp);
        return 20;
    }

    fclose(fp);

    printf("File: %s\n", argv[1]);
    printf("Size: %lu bytes\n", total);
    printf("CRC32: %08lX\n", crc & 0xFFFFFFFFUL);
    printf("Type: %s\n", classify_file(head, head_len));

    return 0;
}
