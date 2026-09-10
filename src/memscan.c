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

typedef struct {
    unsigned int invalid_range;
    unsigned int zero_size;
    unsigned int free_exceeds_range;
    unsigned int odd_boundary;
    unsigned int unnamed;
    unsigned int count;
    unsigned int score;
} MSHeuristics;

static void copy_name(char *dst, size_t size, const char *src)
{
    if (size == 0) return;
    if (src == NULL || src[0] == '\0') src = "<unnamed>";
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
    add_record(s, 0x3000UL, 0x01000001UL, 0x01000001UL,
               1UL, 0x00000000UL, NULL);
}
#endif

static unsigned long region_size(const MSRecord *r)
{
    return r->upper >= r->lower ? r->upper - r->lower : 0;
}

static unsigned long used_bytes(const MSRecord *r)
{
    unsigned long size = region_size(r);
    return r->free_bytes <= size ? size - r->free_bytes : 0;
}

static MSHeuristics evaluate(const MSRecord *r)
{
    MSHeuristics h;
    unsigned long size = region_size(r);
    memset(&h, 0, sizeof(h));

    if (r->upper < r->lower) {
        h.invalid_range = 1;
        h.score += 100;
    }
    if (size == 0) {
        h.zero_size = 1;
        h.score += 60;
    }
    if (r->free_bytes > size) {
        h.free_exceeds_range = 1;
        h.score += 100;
    }
    if ((r->lower & 1UL) != 0 || (r->upper & 1UL) != 0) {
        h.odd_boundary = 1;
        h.score += 20;
    }
    if (strcmp(r->name, "<unnamed>") == 0) {
        h.unnamed = 1;
        h.score += 10;
    }

    h.count = h.invalid_range + h.zero_size + h.free_exceeds_range +
              h.odd_boundary + h.unnamed;
    return h;
}

static const char *risk_level(const MSHeuristics *h)
{
    if (h->score >= 100) return "high";
    if (h->score >= 40) return "medium";
    if (h->score > 0) return "low";
    return "none";
}

static int is_suspicious(const MSRecord *r)
{
    MSHeuristics h = evaluate(r);
    return h.count != 0;
}

static void print_indicator_human(const MSHeuristics *h)
{
    int first = 1;
    if (h->count == 0) {
        printf("none");
        return;
    }
#define PRINT_IND(flag, text) do { if (h->flag) { if (!first) printf(","); printf("%s", text); first = 0; } } while (0)
    PRINT_IND(invalid_range, "invalid-range");
    PRINT_IND(zero_size, "zero-size");
    PRINT_IND(free_exceeds_range, "free-exceeds-range");
    PRINT_IND(odd_boundary, "odd-boundary");
    PRINT_IND(unnamed, "unnamed");
#undef PRINT_IND
}

static void print_human(const MSSnapshot *s, int suspicious_only)
{
    unsigned int i;
    unsigned int shown = 0;
    printf("MemScan memory-region snapshot\n");
    for (i = 0; i < s->count; ++i) {
        const MSRecord *r = &s->records[i];
        MSHeuristics h = evaluate(r);
        if (suspicious_only && h.count == 0) continue;
        printf("%08lX %08lX-%08lX size=%lu free=%lu used=%lu attr=%08lX risk=%s score=%u indicators=",
               r->header_address, r->lower, r->upper, region_size(r),
               r->free_bytes, used_bytes(r), r->attributes,
               risk_level(&h), h.score);
        print_indicator_human(&h);
        printf(" %s\n", r->name);
        ++shown;
    }
    printf("record_count=%u source_record_count=%u truncated=%s\n", shown, s->count,
           s->truncated ? "true" : "false");
}

static void print_kv(const MSSnapshot *s, int suspicious_only)
{
    unsigned int i;
    unsigned int shown = 0;
    printf("tool=MemScan\n");
    printf("schema=amiforensics.snapshot.kv/1\n");
    printf("mode=memory-region-inventory\n");
    printf("filter_suspicious=%s\n", suspicious_only ? "true" : "false");
    for (i = 0; i < s->count; ++i) {
        const MSRecord *r = &s->records[i];
        MSHeuristics h = evaluate(r);
        if (suspicious_only && h.count == 0) continue;
        printf("record.%u.kind=memory-region\n", shown);
        printf("record.%u.name=%s\n", shown, r->name);
        printf("record.%u.address=%08lX\n", shown, r->header_address);
        printf("record.%u.lower=%08lX\n", shown, r->lower);
        printf("record.%u.upper=%08lX\n", shown, r->upper);
        printf("record.%u.size=%lu\n", shown, region_size(r));
        printf("record.%u.free=%lu\n", shown, r->free_bytes);
        printf("record.%u.used=%lu\n", shown, used_bytes(r));
        printf("record.%u.attributes=%08lX\n", shown, r->attributes);
        printf("record.%u.suspicious=%s\n", shown, h.count ? "true" : "false");
        printf("record.%u.risk_level=%s\n", shown, risk_level(&h));
        printf("record.%u.risk_score=%u\n", shown, h.score);
        printf("record.%u.indicator_count=%u\n", shown, h.count);
        printf("record.%u.indicator.invalid_range=%s\n", shown, h.invalid_range ? "true" : "false");
        printf("record.%u.indicator.zero_size=%s\n", shown, h.zero_size ? "true" : "false");
        printf("record.%u.indicator.free_exceeds_range=%s\n", shown, h.free_exceeds_range ? "true" : "false");
        printf("record.%u.indicator.odd_boundary=%s\n", shown, h.odd_boundary ? "true" : "false");
        printf("record.%u.indicator.unnamed=%s\n", shown, h.unnamed ? "true" : "false");
        ++shown;
    }
    printf("record_count=%u\n", shown);
    printf("source_record_count=%u\n", s->count);
    printf("truncated=%s\n", s->truncated ? "true" : "false");
}

static void usage(void)
{
    fprintf(stderr, "Usage: MemScan [--kv] [--suspicious]\n");
}

int main(int argc, char **argv)
{
    MSSnapshot *snapshot;
    int kv = 0;
    int suspicious_only = 0;
    int i;
    int rc;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) kv = 1;
        else if (strcmp(argv[i], "--suspicious") == 0) suspicious_only = 1;
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

    if (kv) print_kv(snapshot, suspicious_only);
    else print_human(snapshot, suspicious_only);

    rc = snapshot->truncated ? 5 : 0;
    free(snapshot);
    return rc;
}
