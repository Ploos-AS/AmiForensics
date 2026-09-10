#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MS_MAX_RECORDS 128
#define MS_NAME_LEN 80

typedef struct {
    unsigned long header_address;
    unsigned long lower;
    unsigned long upper;
    unsigned long free_bytes;
    unsigned long attributes;
    char name[MS_NAME_LEN];
} MSRecord;

typedef struct {
    MSRecord records[MS_MAX_RECORDS];
    unsigned int count;
    unsigned int truncated;
} MSSnapshot;

static void copy_name(char *dst, size_t size, const char *src)
{
    if (size == 0) return;
    if (src == NULL) src = "<unnamed>";
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

static void add_record(MSSnapshot *s, unsigned long header_address,
                       unsigned long lower, unsigned long upper,
                       unsigned long free_bytes, unsigned long attributes,
                       const char *name)
{
    MSRecord *r;
    if (s->count >= MS_MAX_RECORDS) {
        s->truncated = 1;
        return;
    }
    r = &s->records[s->count++];
    r->header_address = header_address;
    r->lower = lower;
    r->upper = upper;
    r->free_bytes = free_bytes;
    r->attributes = attributes;
    copy_name(r->name, sizeof(r->name), name);
}

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
extern struct ExecBase *SysBase;

static void take_snapshot(MSSnapshot *s)
{
    struct MemHeader *mh;
    Forbid();
    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh != NULL && mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        add_record(s,
                   (unsigned long)mh,
                   (unsigned long)mh->mh_Lower,
                   (unsigned long)mh->mh_Upper,
                   (unsigned long)mh->mh_Free,
                   (unsigned long)mh->mh_Attributes,
                   (const char *)mh->mh_Node.ln_Name);
        if (s->truncated) break;
    }
    Permit();
}
#else
static void take_snapshot(MSSnapshot *s)
{
    add_record(s, 0x1000UL, 0x00000000UL, 0x00200000UL,
               0x00100000UL, 0x00000002UL, "chip memory");
    add_record(s, 0x2000UL, 0x00200000UL, 0x00A00000UL,
               0x00600000UL, 0x00000004UL, "fast memory");
}
#endif

static unsigned long region_size(const MSRecord *r)
{
    return r->upper >= r->lower ? r->upper - r->lower : 0;
}

static const char *risk_hint(const MSRecord *r)
{
    unsigned long size = region_size(r);
    if (r->upper < r->lower) return "invalid-range";
    if (size == 0) return "zero-size";
    if (r->free_bytes > size) return "free-exceeds-range";
    return "none";
}

static void print_human(const MSSnapshot *s)
{
    unsigned int i;
    printf("MemScan memory-region snapshot\n");
    for (i = 0; i < s->count; ++i) {
        const MSRecord *r = &s->records[i];
        printf("%08lX %08lX-%08lX size=%lu free=%lu attr=%08lX risk=%s %s\n",
               r->header_address, r->lower, r->upper, region_size(r),
               r->free_bytes, r->attributes, risk_hint(r), r->name);
    }
    printf("record_count=%u truncated=%s\n", s->count,
           s->truncated ? "true" : "false");
}

static void print_kv(const MSSnapshot *s)
{
    unsigned int i;
    printf("tool=MemScan\n");
    printf("schema=amiforensics.snapshot.kv/1\n");
    printf("mode=memory-region-inventory\n");
    for (i = 0; i < s->count; ++i) {
        const MSRecord *r = &s->records[i];
        printf("record.%u.kind=memory-region\n", i);
        printf("record.%u.name=%s\n", i, r->name);
        printf("record.%u.address=%08lX\n", i, r->header_address);
        printf("record.%u.lower=%08lX\n", i, r->lower);
        printf("record.%u.upper=%08lX\n", i, r->upper);
        printf("record.%u.size=%lu\n", i, region_size(r));
        printf("record.%u.free=%lu\n", i, r->free_bytes);
        printf("record.%u.attributes=%08lX\n", i, r->attributes);
        printf("record.%u.risk_hint=%s\n", i, risk_hint(r));
    }
    printf("record_count=%u\n", s->count);
    printf("truncated=%s\n", s->truncated ? "true" : "false");
}

static void usage(void)
{
    fprintf(stderr, "Usage: MemScan [--kv]\n");
}

int main(int argc, char **argv)
{
    MSSnapshot *snapshot;
    int kv = 0;
    int i;
    int rc;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) kv = 1;
        else {
            usage();
            return 10;
        }
    }

    snapshot = (MSSnapshot *)malloc(sizeof(*snapshot));
    if (snapshot == NULL) {
        fprintf(stderr, "MemScan: out of memory\n");
        return 20;
    }
    memset(snapshot, 0, sizeof(*snapshot));
    take_snapshot(snapshot);

    if (kv) print_kv(snapshot);
    else print_human(snapshot);

    rc = snapshot->truncated ? 5 : 0;
    free(snapshot);
    return rc;
}
