#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SD_MAX_BYTES 65536UL
#define SD_CHUNK 512U

typedef struct {
    unsigned long address;
    unsigned long length;
    unsigned long region_lower;
    unsigned long region_upper;
    unsigned long crc32;
    const char *output;
} SDResult;

static unsigned long crc32_update(unsigned long crc, const unsigned char *buf, size_t len)
{
    size_t i;
    unsigned int bit;
    crc &= 0xFFFFFFFFUL;
    for (i = 0; i < len; ++i) {
        crc ^= (unsigned long)buf[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
    return crc & 0xFFFFFFFFUL;
}

static int parse_ulong(const char *s, unsigned long *out)
{
    char *end = NULL;
    unsigned long v;
    if (s == NULL || *s == '\0') return 0;
    v = strtoul(s, &end, 0);
    if (end == s || *end != '\0') return 0;
    *out = v;
    return 1;
}

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
extern struct ExecBase *SysBase;

static int find_region(unsigned long address, unsigned long length,
                       unsigned long *lower, unsigned long *upper)
{
    struct MemHeader *mh;
    unsigned long end;
    int found = 0;
    if (length == 0 || address > 0xFFFFFFFFUL - (length - 1UL)) return 0;
    end = address + length;
    Forbid();
    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh != NULL && mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        unsigned long lo = (unsigned long)mh->mh_Lower;
        unsigned long hi = (unsigned long)mh->mh_Upper;
        if (hi > lo && address >= lo && end <= hi) {
            *lower = lo;
            *upper = hi;
            found = 1;
            break;
        }
    }
    Permit();
    return found;
}

static void read_bytes(unsigned long address, unsigned char *buf, size_t len)
{
    memcpy(buf, (const void *)address, len);
}
#else
static int find_region(unsigned long address, unsigned long length,
                       unsigned long *lower, unsigned long *upper)
{
    unsigned long end;
    if (length == 0 || address > ~0UL - length) return 0;
    end = address + length;
    if (address >= 0x1000UL && end <= 0x1100UL) {
        *lower = 0x1000UL;
        *upper = 0x1100UL;
        return 1;
    }
    return 0;
}

static void read_bytes(unsigned long address, unsigned char *buf, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i)
        buf[i] = (unsigned char)((address + (unsigned long)i) & 0xFFUL);
}
#endif

static int dump_range(unsigned long address, unsigned long length,
                      const char *output, SDResult *result)
{
    FILE *f;
    unsigned char buf[SD_CHUNK];
    unsigned long done = 0;
    unsigned long crc = 0xFFFFFFFFUL;
    unsigned long lower = 0, upper = 0;

    if (!find_region(address, length, &lower, &upper)) return 5;
    f = fopen(output, "wb");
    if (f == NULL) return 20;

    while (done < length) {
        size_t n = (size_t)((length - done) > SD_CHUNK ? SD_CHUNK : (length - done));
        read_bytes(address + done, buf, n);
        if (fwrite(buf, 1, n, f) != n) {
            fclose(f);
            remove(output);
            return 20;
        }
        crc = crc32_update(crc, buf, n);
        done += (unsigned long)n;
    }
    if (fclose(f) != 0) {
        remove(output);
        return 20;
    }

    result->address = address;
    result->length = length;
    result->region_lower = lower;
    result->region_upper = upper;
    result->crc32 = (~crc) & 0xFFFFFFFFUL;
    result->output = output;
    return 0;
}

static void print_result(const SDResult *r, int kv)
{
    if (kv) {
        printf("tool=SampleDump\n");
        printf("schema=amiforensics.sampledump.kv/1\n");
        printf("mode=bounded-memory-dump\n");
        printf("address=%08lX\n", r->address & 0xFFFFFFFFUL);
        printf("length=%lu\n", r->length);
        printf("region_lower=%08lX\n", r->region_lower & 0xFFFFFFFFUL);
        printf("region_upper=%08lX\n", r->region_upper & 0xFFFFFFFFUL);
        printf("crc32=%08lX\n", r->crc32 & 0xFFFFFFFFUL);
        printf("output=%s\n", r->output);
        printf("complete=true\n");
    } else {
        printf("SampleDump: %lu bytes from %08lX -> %s\n",
               r->length, r->address & 0xFFFFFFFFUL, r->output);
        printf("Region: %08lX-%08lX CRC32=%08lX\n",
               r->region_lower & 0xFFFFFFFFUL, r->region_upper & 0xFFFFFFFFUL,
               r->crc32 & 0xFFFFFFFFUL);
    }
}

static void usage(void)
{
    fprintf(stderr, "Usage: SampleDump --address HEX --length N --output FILE [--kv]\n");
    fprintf(stderr, "Maximum dump size: %lu bytes; range must fit one Exec MemHeader region.\n", SD_MAX_BYTES);
}

int main(int argc, char **argv)
{
    unsigned long address = 0, length = 0;
    int have_address = 0, have_length = 0, kv = 0;
    const char *output = NULL;
    SDResult result;
    int i, rc;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) kv = 1;
        else if (strcmp(argv[i], "--address") == 0 && i + 1 < argc) {
            have_address = parse_ulong(argv[++i], &address);
            if (!have_address) { usage(); return 10; }
        } else if (strcmp(argv[i], "--length") == 0 && i + 1 < argc) {
            have_length = parse_ulong(argv[++i], &length);
            if (!have_length) { usage(); return 10; }
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else {
            usage();
            return 10;
        }
    }

    if (!have_address || !have_length || output == NULL || length == 0 || length > SD_MAX_BYTES) {
        usage();
        return 10;
    }

    rc = dump_range(address, length, output, &result);
    if (rc == 5) {
        fprintf(stderr, "SampleDump: requested range is not wholly inside one known memory region\n");
        return 5;
    }
    if (rc != 0) {
        fprintf(stderr, "SampleDump: dump failed\n");
        return rc;
    }
    print_result(&result, kv);
    return 0;
}
