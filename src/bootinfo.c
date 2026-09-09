#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOTBLOCK_SIZE 1024U
#define BOOT_CODE_OFFSET 12U

static unsigned long read_be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) |
           ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) |
           (unsigned long)p[3];
}

static unsigned long boot_checksum_sum(const unsigned char *buf)
{
    unsigned long sum = 0;
    unsigned int i;

    for (i = 0; i < BOOTBLOCK_SIZE; i += 4) {
        unsigned long old = sum;
        sum += read_be32(buf + i);
        sum &= 0xFFFFFFFFUL;
        if (sum < old)
            sum = (sum + 1UL) & 0xFFFFFFFFUL;
    }
    return sum;
}

static int contains_ascii_ci(const unsigned char *buf, size_t len, const char *needle)
{
    size_t nlen = strlen(needle);
    size_t i, j;

    if (nlen == 0 || nlen > len)
        return 0;

    for (i = 0; i + nlen <= len; ++i) {
        for (j = 0; j < nlen; ++j) {
            unsigned char a = buf[i + j];
            unsigned char b = (unsigned char)needle[j];
            if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
            if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
            if (a != b)
                break;
        }
        if (j == nlen)
            return 1;
    }
    return 0;
}

static unsigned int count_nonzero(const unsigned char *buf, size_t len)
{
    unsigned int count = 0;
    size_t i;
    for (i = 0; i < len; ++i)
        if (buf[i] != 0)
            ++count;
    return count;
}

static void print_hexdump(const unsigned char *buf, size_t len)
{
    size_t i, j;
    for (i = 0; i < len; i += 16) {
        printf("%04lX: ", (unsigned long)i);
        for (j = 0; j < 16; ++j) {
            if (i + j < len)
                printf("%02X ", (unsigned int)buf[i + j]);
            else
                printf("   ");
        }
        printf(" ");
        for (j = 0; j < 16 && i + j < len; ++j) {
            unsigned char c = buf[i + j];
            putchar((c >= 32 && c <= 126) ? (int)c : '.');
        }
        putchar('\n');
    }
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [--hex] [--kv] <bootblock-or-image>\n", prog);
}

int main(int argc, char **argv)
{
    FILE *fp;
    unsigned char boot[BOOTBLOCK_SIZE];
    size_t got;
    const char *path = NULL;
    int hex = 0, kv = 0;
    int i;
    int has_dos;
    unsigned int dostype = 0;
    unsigned long stored_checksum;
    unsigned long root_block;
    unsigned long sum;
    int checksum_valid;
    unsigned int code_nonzero;
    int ind_trackdisk, ind_exec, ind_doslib, ind_rexx;
    unsigned int indicator_count = 0;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--hex") == 0)
            hex = 1;
        else if (strcmp(argv[i], "--kv") == 0)
            kv = 1;
        else if (path == NULL)
            path = argv[i];
        else {
            usage(argv[0]);
            return 10;
        }
    }

    if (path == NULL) {
        usage(argv[0]);
        return 10;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "BootInfo: cannot open '%s'\n", path);
        return 20;
    }

    got = fread(boot, 1, sizeof(boot), fp);
    if (ferror(fp)) {
        fprintf(stderr, "BootInfo: read error on '%s'\n", path);
        fclose(fp);
        return 20;
    }
    fclose(fp);

    if (got < BOOTBLOCK_SIZE) {
        fprintf(stderr, "BootInfo: need at least %u bytes, got %lu\n",
                (unsigned int)BOOTBLOCK_SIZE, (unsigned long)got);
        return 20;
    }

    has_dos = boot[0] == 'D' && boot[1] == 'O' && boot[2] == 'S';
    if (has_dos)
        dostype = (unsigned int)boot[3];

    stored_checksum = read_be32(boot + 4);
    root_block = read_be32(boot + 8);
    sum = boot_checksum_sum(boot);
    checksum_valid = (sum == 0xFFFFFFFFUL);
    code_nonzero = count_nonzero(boot + BOOT_CODE_OFFSET,
                                 BOOTBLOCK_SIZE - BOOT_CODE_OFFSET);

    ind_trackdisk = contains_ascii_ci(boot + BOOT_CODE_OFFSET,
                                      BOOTBLOCK_SIZE - BOOT_CODE_OFFSET,
                                      "trackdisk.device");
    ind_exec = contains_ascii_ci(boot + BOOT_CODE_OFFSET,
                                 BOOTBLOCK_SIZE - BOOT_CODE_OFFSET,
                                 "exec.library");
    ind_doslib = contains_ascii_ci(boot + BOOT_CODE_OFFSET,
                                   BOOTBLOCK_SIZE - BOOT_CODE_OFFSET,
                                   "dos.library");
    ind_rexx = contains_ascii_ci(boot + BOOT_CODE_OFFSET,
                                 BOOTBLOCK_SIZE - BOOT_CODE_OFFSET,
                                 "rexxmast");

    indicator_count = (unsigned int)(ind_trackdisk + ind_exec + ind_doslib + ind_rexx);

    if (kv) {
        printf("file=%s\n", path);
        printf("dos_signature=%s\n", has_dos ? "yes" : "no");
        if (has_dos)
            printf("dos_type=%u\n", dostype);
        printf("stored_checksum=%08lX\n", stored_checksum & 0xFFFFFFFFUL);
        printf("checksum_sum=%08lX\n", sum & 0xFFFFFFFFUL);
        printf("checksum_valid=%s\n", checksum_valid ? "yes" : "no");
        printf("root_block=%lu\n", root_block);
        printf("bootcode_nonzero_bytes=%u\n", code_nonzero);
        printf("indicator_count=%u\n", indicator_count);
        printf("indicator_trackdisk=%s\n", ind_trackdisk ? "yes" : "no");
        printf("indicator_exec_library=%s\n", ind_exec ? "yes" : "no");
        printf("indicator_dos_library=%s\n", ind_doslib ? "yes" : "no");
        printf("indicator_rexxmast=%s\n", ind_rexx ? "yes" : "no");
    } else {
        printf("File: %s\n", path);
        printf("DOS signature: %s\n", has_dos ? "yes" : "no");
        if (has_dos)
            printf("DOS type: %u\n", dostype);
        printf("Stored checksum: %08lX\n", stored_checksum & 0xFFFFFFFFUL);
        printf("Checksum sum: %08lX\n", sum & 0xFFFFFFFFUL);
        printf("Checksum valid: %s\n", checksum_valid ? "yes" : "no");
        printf("Root block: %lu\n", root_block);
        printf("Bootcode non-zero bytes: %u/%u\n", code_nonzero,
               (unsigned int)(BOOTBLOCK_SIZE - BOOT_CODE_OFFSET));
        printf("Indicators: %u\n", indicator_count);
        if (ind_trackdisk) printf("  trackdisk.device reference\n");
        if (ind_exec)      printf("  exec.library reference\n");
        if (ind_doslib)    printf("  dos.library reference\n");
        if (ind_rexx)      printf("  RexxMast reference\n");
    }

    if (hex)
        print_hexdump(boot, BOOTBLOCK_SIZE);

    return 0;
}
