#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PV_DEFAULT_LIBRARY "exec.library"
#define PV_DEFAULT_COUNT 32
#define PV_MAX_COUNT 256

typedef struct {
    unsigned int index;
    long lvo;
    unsigned long vector_address;
    unsigned int opcode;
    unsigned long target;
    int direct_jmp;
} PVRecord;

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

static int inspect_vectors(const char *name, unsigned int count, PVRecord *records,
                           unsigned int *out_count)
{
    struct Library *lib;
    int must_close = 0;
    unsigned int i;

    lib = open_target(name, &must_close);
    if (lib == NULL) {
        fprintf(stderr, "PatchView: cannot open %s\n", name);
        return 20;
    }

    for (i = 0; i < count; ++i) {
        unsigned char *v = ((unsigned char *)lib) - (6UL * (i + 1UL));
        PVRecord *r = &records[i];
        r->index = i + 1;
        r->lvo = -(long)(6UL * (i + 1UL));
        r->vector_address = (unsigned long)v;
        r->opcode = read_be16(v);
        r->direct_jmp = (r->opcode == 0x4EF9U);
        r->target = r->direct_jmp ? read_be32(v + 2) : 0;
    }
    *out_count = count;

    if (must_close) CloseLibrary(lib);
    return 0;
}
#else
static int inspect_vectors(const char *name, unsigned int count, PVRecord *records,
                           unsigned int *out_count)
{
    unsigned int i;
    (void)name;
    for (i = 0; i < count; ++i) {
        records[i].index = i + 1;
        records[i].lvo = -(long)(6UL * (i + 1UL));
        records[i].vector_address = 0x1000UL - (6UL * (i + 1UL));
        records[i].opcode = 0x4EF9U;
        records[i].target = 0x2000UL + (unsigned long)(i * 16UL);
        records[i].direct_jmp = 1;
    }
    *out_count = count;
    return 0;
}
#endif

static void print_human(const char *library, const PVRecord *records, unsigned int count)
{
    unsigned int i;
    printf("PatchView library vectors: %s\n", library);
    for (i = 0; i < count; ++i) {
        const PVRecord *r = &records[i];
        printf("%3u LVO %5ld vector=%08lX opcode=%04X", r->index, r->lvo,
               r->vector_address, r->opcode);
        if (r->direct_jmp)
            printf(" target=%08lX JMP", r->target);
        else
            printf(" target=-------- non-JMP");
        putchar('\n');
    }
}

static void print_kv(const char *library, const PVRecord *records, unsigned int count)
{
    unsigned int i;
    printf("tool=PatchView\n");
    printf("library=%s\n", library);
    for (i = 0; i < count; ++i) {
        const PVRecord *r = &records[i];
        printf("record.%u.index=%u\n", i, r->index);
        printf("record.%u.lvo=%ld\n", i, r->lvo);
        printf("record.%u.vector_address=%08lX\n", i, r->vector_address);
        printf("record.%u.opcode=%04X\n", i, r->opcode);
        printf("record.%u.direct_jmp=%s\n", i, r->direct_jmp ? "true" : "false");
        printf("record.%u.target=%08lX\n", i, r->target);
    }
    printf("record_count=%u\n", count);
}

int main(int argc, char **argv)
{
    const char *library = PV_DEFAULT_LIBRARY;
    unsigned int count = PV_DEFAULT_COUNT;
    int kv = 0;
    int i;
    PVRecord *records;
    unsigned int actual = 0;
    int rc;

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

    rc = inspect_vectors(library, count, records, &actual);
    if (rc == 0) {
        if (kv) print_kv(library, records, actual);
        else print_human(library, records, actual);
    }
    free(records);
    return rc;
}
