#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MS_MAX_RECORDS 128
#define MS_MAX_RESIDENTS 128
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
    unsigned long address;
    unsigned long end_skip;
    unsigned long init;
    unsigned int flags;
    unsigned int version;
    unsigned int type;
    int priority;
    char name[MS_NAME_LEN];
} MSResident;

typedef struct {
    MSRecord records[MS_MAX_RECORDS];
    unsigned int count;
    MSResident residents[MS_MAX_RESIDENTS];
    unsigned int resident_count;
    unsigned int truncated;
    unsigned int residents_truncated;
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

static void add_resident(MSSnapshot *s, unsigned long address,
                         unsigned long end_skip, unsigned long init,
                         unsigned int flags, unsigned int version,
                         unsigned int type, int priority, const char *name)
{
    MSResident *r;
    if (s->resident_count >= MS_MAX_RESIDENTS) {
        s->residents_truncated = 1;
        return;
    }
    r = &s->residents[s->resident_count++];
    r->address = address;
    r->end_skip = end_skip;
    r->init = init;
    r->flags = flags;
    r->version = version;
    r->type = type;
    r->priority = priority;
    copy_name(r->name, sizeof(r->name), name);
}

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <proto/exec.h>
extern struct ExecBase *SysBase;

static void take_snapshot(MSSnapshot *s)
{
    struct MemHeader *mh;
    struct Resident **mods;
    unsigned int i;

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

    mods = (struct Resident **)SysBase->ResModules;
    if (mods != NULL) {
        for (i = 0; i < 256 && mods[i] != NULL; ++i) {
            struct Resident *r = mods[i];
            if (r->rt_MatchWord != RTC_MATCHWORD || r->rt_MatchTag != r) continue;
            add_resident(s,
                         (unsigned long)r,
                         (unsigned long)r->rt_EndSkip,
                         (unsigned long)r->rt_Init,
                         (unsigned int)r->rt_Flags,
                         (unsigned int)r->rt_Version,
                         (unsigned int)r->rt_Type,
                         (int)r->rt_Pri,
                         (const char *)r->rt_Name);
            if (s->residents_truncated) break;
        }
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
    add_resident(s, 0x00010000UL, 0x00010100UL, 0x00010040UL,
                 0x01U, 40U, 9U, 5, "host-resident-stub");
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

static int containing_region(const MSSnapshot *s, unsigned long address)
{
    unsigned int i;
    for (i = 0; i < s->count; ++i) {
        const MSRecord *r = &s->records[i];
        if (r->upper > r->lower && address >= r->lower && address < r->upper)
            return (int)i;
    }
    return -1;
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

static void print_residents_human(const MSSnapshot *s)
{
    unsigned int i;
    printf("MemScan resident/code discovery\n");
    for (i = 0; i < s->resident_count; ++i) {
        const MSResident *r = &s->residents[i];
        int region = containing_region(s, r->address);
        printf("%08lX end=%08lX init=%08lX ver=%u type=%u flags=%02X pri=%d region=%d %s\n",
               r->address, r->end_skip, r->init, r->version, r->type,
               r->flags, r->priority, region, r->name);
    }
    printf("record_count=%u truncated=%s\n", s->resident_count,
           s->residents_truncated ? "true" : "false");
}

static void print_residents_kv(const MSSnapshot *s)
{
    unsigned int i;
    printf("tool=MemScan\n");
    printf("schema=amiforensics.snapshot.kv/1\n");
    printf("mode=resident-code-discovery\n");
    printf("discovery_source=exec-resmodules\n");
    for (i = 0; i < s->resident_count; ++i) {
        const MSResident *r = &s->residents[i];
        int region = containing_region(s, r->address);
        printf("record.%u.kind=resident-code\n", i);
        printf("record.%u.name=%s\n", i, r->name);
        printf("record.%u.address=%08lX\n", i, r->address);
        printf("record.%u.end_skip=%08lX\n", i, r->end_skip);
        printf("record.%u.init=%08lX\n", i, r->init);
        printf("record.%u.flags=%02X\n", i, r->flags);
        printf("record.%u.version=%u\n", i, r->version);
        printf("record.%u.type=%u\n", i, r->type);
        printf("record.%u.priority=%d\n", i, r->priority);
        printf("record.%u.match_valid=true\n", i);
        printf("record.%u.region_index=%d\n", i, region);
        if (region >= 0) {
            const MSRecord *mr = &s->records[(unsigned int)region];
            printf("record.%u.region_lower=%08lX\n", i, mr->lower);
            printf("record.%u.region_upper=%08lX\n", i, mr->upper);
        }
    }
    printf("record_count=%u\n", s->resident_count);
    printf("source_region_count=%u\n", s->count);
    printf("truncated=%s\n", s->residents_truncated ? "true" : "false");
}

static void usage(void)
{
    fprintf(stderr, "Usage: MemScan [--kv] [--suspicious | --discover-residents]\n");
}

int main(int argc, char **argv)
{
    MSSnapshot *snapshot;
    int kv = 0;
    int suspicious_only = 0;
    int discover_residents = 0;
    int i;
    int rc;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) kv = 1;
        else if (strcmp(argv[i], "--suspicious") == 0) suspicious_only = 1;
        else if (strcmp(argv[i], "--discover-residents") == 0) discover_residents = 1;
        else {
            usage();
            return 10;
        }
    }
    if (suspicious_only && discover_residents) {
        usage();
        return 10;
    }

    snapshot = (MSSnapshot *)malloc(sizeof(*snapshot));
    if (snapshot == NULL) {
        fprintf(stderr, "MemScan: out of memory\n");
        return 20;
    }
    memset(snapshot, 0, sizeof(*snapshot));
    take_snapshot(snapshot);

    if (discover_residents) {
        if (kv) print_residents_kv(snapshot);
        else print_residents_human(snapshot);
        rc = snapshot->residents_truncated ? 5 : 0;
    } else {
        if (kv) print_kv(snapshot, suspicious_only);
        else print_human(snapshot, suspicious_only);
        rc = snapshot->truncated ? 5 : 0;
    }

    free(snapshot);
    return rc;
}
