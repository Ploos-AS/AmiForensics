#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PV_DEFAULT_LIBRARY "exec.library"
#define PV_DEFAULT_COUNT 32
#define PV_MAX_COUNT 256
#define PV_VECTOR_SIZE 6U

typedef struct {
    unsigned int index;
    long lvo;
    unsigned long vector_address;
    unsigned int opcode;
    unsigned long target;
    int direct_jmp;
} PVRecord;

typedef struct {
    unsigned long library_base;
    unsigned int neg_size;
    unsigned int available_vectors;
    unsigned int requested_vectors;
    unsigned int inspected_vectors;
    unsigned long vector_crc32;
    int clipped;
} PVSummary;

static unsigned long crc32_update(unsigned long crc, const unsigned char *data,
                                  unsigned int len)
{
    unsigned int i;
    crc = ~crc;
    for (i = 0; i < len; ++i) {
        unsigned int bit;
        crc ^= (unsigned long)data[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
    return ~crc;
}

static void usage(void)
{
    fprintf(stderr, "Usage: PatchView [--kv] [--library NAME] [--count N]\n");
}

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <proto/exec.h>
extern struct ExecBase *SysBase;

static unsigned int read_be16(const unsigned char *p)
{
    return ((unsigned int)p[0] << 8) | (unsigned int)p[1];
}

static unsigned long read_be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) |
           ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) |
           (unsigned long)p[3];
}

static struct Library *open_target(const char *name, int *must_close)
{
    if (strcmp(name, "exec.library") == 0) {
        *must_close = 0;
        return (struct Library *)SysBase;
    }
    *must_close = 1;
    return OpenLibrary((CONST_STRPTR)name, 0);
}

static int inspect_vectors(const char *name, unsigned int requested,
                           PVRecord *records, PVSummary *summary)
{
    struct Library *lib;
    int must_close = 0;
    unsigned int i;
    unsigned int available;
    unsigned int count;
    unsigned long crc = 0;

    lib = open_target(name, &must_close);
    if (lib == NULL) {
        fprintf(stderr, "PatchView: cannot open %s\n", name);
        return 20;
    }

    available = (unsigned int)lib->lib_NegSize / PV_VECTOR_SIZE;
    count = requested;
    if (count > available) count = available;

    summary->library_base = (unsigned long)lib;
    summary->neg_size = (unsigned int)lib->lib_NegSize;
    summary->available_vectors = available;
    summary->requested_vectors = requested;
    summary->inspected_vectors = count;
    summary->clipped = (count != requested);

    for (i = 0; i < count; ++i) {
        unsigned char *v = ((unsigned char *)lib) - (PV_VECTOR_SIZE * (i + 1UL));
        PVRecord *r = &records[i];
        r->index = i + 1;
        r->lvo = -(long)(PV_VECTOR_SIZE * (i + 1UL));
        r->vector_address = (unsigned long)v;
        r->opcode = read_be16(v);
        r->direct_jmp = (r->opcode == 0x4EF9U);
        r->target = r->direct_jmp ? read_be32(v + 2) : 0;
        crc = crc32_update(crc, v, PV_VECTOR_SIZE);
    }
    summary->vector_crc32 = crc;

    if (must_close) CloseLibrary(lib);
    return 0;
}
#else
static int inspect_vectors(const char *name, unsigned int requested,
                           PVRecord *records, PVSummary *summary)
{
    unsigned int i;
    unsigned long crc = 0;
    (void)name;

    summary->library_base = 0x1000UL;
    summary->neg_size = (unsigned int)(PV_MAX_COUNT * PV_VECTOR_SIZE);
    summary->available_vectors = PV_MAX_COUNT;
    summary->requested_vectors = requested;
    summary->inspected_vectors = requested;
    summary->clipped = 0;

    for (i = 0; i < requested; ++i) {
        unsigned char raw[PV_VECTOR_SIZE];
        unsigned long target = 0x2000UL + (unsigned long)(i * 16UL);
        records[i].index = i + 1;
        records[i].lvo = -(long)(PV_VECTOR_SIZE * (i + 1UL));
        records[i].vector_address = 0x1000UL - (PV_VECTOR_SIZE * (i + 1UL));
        records[i].opcode = 0x4EF9U;
        records[i].target = target;
        records[i].direct_jmp = 1;
        raw[0] = 0x4E; raw[1] = 0xF9;
        raw[2] = (unsigned char)(target >> 24);
        raw[3] = (unsigned char)(target >> 16);
        raw[4] = (unsigned char)(target >> 8);
        raw[5] = (unsigned char)target;
        crc = crc32_update(crc, raw, PV_VECTOR_SIZE);
    }
    summary->vector_crc32 = crc;
    return 0;
}
#endif

static void print_human(const char *library, const PVRecord *records,
                        const PVSummary *summary)
{
    unsigned int i;
    printf("PatchView library vectors: %s\n", library);
    printf("base=%08lX neg_size=%u available=%u inspected=%u crc32=%08lX\n",
           summary->library_base, summary->neg_size,
           summary->available_vectors, summary->inspected_vectors,
           summary->vector_crc32);
    for (i = 0; i < summary->inspected_vectors; ++i) {
        const PVRecord *r = &records[i];
        printf("%3u LVO %5ld vector=%08lX opcode=%04X", r->index, r->lvo,
               r->vector_address, r->opcode);
        if (r->direct_jmp)
            printf(" target=%08lX JMP", r->target);
        else
            printf(" target=-------- non-JMP");
        putchar('\n');
    }
    if (summary->clipped)
        printf("WARNING: requested %u vectors, library exposes only %u complete 6-byte entries\n",
               summary->requested_vectors, summary->available_vectors);
}

static void print_kv(const char *library, const PVRecord *records,
                     const PVSummary *summary)
{
    unsigned int i;
    printf("tool=PatchView\n");
    printf("library=%s\n", library);
    printf("library_base=%08lX\n", summary->library_base);
    printf("neg_size=%u\n", summary->neg_size);
    printf("available_vectors=%u\n", summary->available_vectors);
    printf("requested_vectors=%u\n", summary->requested_vectors);
    printf("inspected_vectors=%u\n", summary->inspected_vectors);
    printf("vector_crc32=%08lX\n", summary->vector_crc32);
    printf("clipped=%s\n", summary->clipped ? "true" : "false");
    for (i = 0; i < summary->inspected_vectors; ++i) {
        const PVRecord *r = &records[i];
        printf("record.%u.index=%u\n", i, r->index);
        printf("record.%u.lvo=%ld\n", i, r->lvo);
        printf("record.%u.vector_address=%08lX\n", i, r->vector_address);
        printf("record.%u.opcode=%04X\n", i, r->opcode);
        printf("record.%u.direct_jmp=%s\n", i, r->direct_jmp ? "true" : "false");
        printf("record.%u.target=%08lX\n", i, r->target);
    }
    printf("record_count=%u\n", summary->inspected_vectors);
}

int main(int argc, char **argv)
{
    const char *library = PV_DEFAULT_LIBRARY;
    unsigned int count = PV_DEFAULT_COUNT;
    int kv = 0;
    int i;
    PVRecord *records;
    PVSummary summary;
    int rc;

    memset(&summary, 0, sizeof(summary));

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) {
            kv = 1;
        } else if (strcmp(argv[i], "--library") == 0 && i + 1 < argc) {
            library = argv[++i];
        } else if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            char *end = NULL;
            unsigned long n = strtoul(argv[++i], &end, 10);
            if (end == NULL || *end != '\0' || n == 0 || n > PV_MAX_COUNT) {
                usage();
                return 10;
            }
            count = (unsigned int)n;
        } else {
            usage();
            return 10;
        }
    }

    records = (PVRecord *)malloc(sizeof(*records) * count);
    if (records == NULL) {
        fprintf(stderr, "PatchView: out of memory\n");
        return 20;
    }

    rc = inspect_vectors(library, count, records, &summary);
    if (rc == 0) {
        if (kv) print_kv(library, records, &summary);
        else print_human(library, records, &summary);
        if (summary.clipped) rc = 5;
    }
    free(records);
    return rc;
}
