#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POV_MAX_RECORDS 256
#define POV_NAME_LEN 96

typedef struct {
    unsigned long address;
    long priority;
    unsigned int sigbit;
    unsigned long sigtask;
    char name[POV_NAME_LEN];
} POVRecord;

typedef struct {
    POVRecord records[POV_MAX_RECORDS];
    unsigned int count;
    unsigned int truncated;
} POVSnapshot;

static void add_record(POVSnapshot *s, const char *name, unsigned long address,
                       long priority, unsigned int sigbit, unsigned long sigtask)
{
    POVRecord *r;
    if (s->count >= POV_MAX_RECORDS) { s->truncated = 1; return; }
    r = &s->records[s->count++];
    r->address = address;
    r->priority = priority;
    r->sigbit = sigbit;
    r->sigtask = sigtask;
    strncpy(r->name, name ? name : "<unnamed>", sizeof(r->name) - 1);
    r->name[sizeof(r->name) - 1] = '\0';
}

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <exec/execbase.h>
#include <exec/ports.h>
#include <proto/exec.h>
extern struct ExecBase *SysBase;

static void take_snapshot(POVSnapshot *s)
{
    struct MsgPort *port;
    Forbid();
    for (port = (struct MsgPort *)SysBase->PortList.lh_Head;
         port != NULL && port->mp_Node.ln_Succ != NULL;
         port = (struct MsgPort *)port->mp_Node.ln_Succ) {
        add_record(s, (const char *)port->mp_Node.ln_Name,
                   (unsigned long)port,
                   (long)port->mp_Node.ln_Pri,
                   (unsigned int)port->mp_SigBit,
                   (unsigned long)port->mp_SigTask);
    }
    Permit();
}
#else
static void take_snapshot(POVSnapshot *s)
{
    add_record(s, "REXX", 0x1000UL, 0, 5, 0x2000UL);
    add_record(s, "WORKBENCH", 0x1100UL, 0, 6, 0x2100UL);
}
#endif

static int name_matches(const char *filter, const char *name)
{
    if (filter == NULL) return 1;
    if (name == NULL) name = "";
    return strstr(name, filter) != NULL;
}

static void print_human(const POVSnapshot *s, const char *filter)
{
    unsigned int i, out = 0;
    printf("PortView message port snapshot\n");
    for (i = 0; i < s->count; ++i) {
        const POVRecord *r = &s->records[i];
        if (!name_matches(filter, r->name)) continue;
        printf("%08lX pri=%ld sigbit=%u sigtask=%08lX %s\n",
               r->address, r->priority, r->sigbit, r->sigtask, r->name);
        ++out;
    }
    printf("record_count=%u\n", out);
    if (s->truncated) printf("WARNING: snapshot truncated at %u records\n", POV_MAX_RECORDS);
}

static void print_kv(const POVSnapshot *s, const char *filter)
{
    unsigned int i, out = 0;
    printf("tool=PortView\n");
    if (filter != NULL) printf("filter_name=%s\n", filter);
    for (i = 0; i < s->count; ++i) {
        const POVRecord *r = &s->records[i];
        if (!name_matches(filter, r->name)) continue;
        printf("record.%u.address=%08lX\n", out, r->address);
        printf("record.%u.priority=%ld\n", out, r->priority);
        printf("record.%u.sigbit=%u\n", out, r->sigbit);
        printf("record.%u.sigtask=%08lX\n", out, r->sigtask);
        printf("record.%u.name=%s\n", out, r->name);
        ++out;
    }
    printf("record_count=%u\n", out);
    printf("truncated=%s\n", s->truncated ? "true" : "false");
}

static void usage(void)
{
    fprintf(stderr, "Usage: PortView [--kv] [--name TEXT]\n");
}

int main(int argc, char **argv)
{
    POVSnapshot *snapshot;
    const char *filter = NULL;
    int kv = 0, i, rc;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) kv = 1;
        else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) filter = argv[++i];
        else { usage(); return 10; }
    }

    snapshot = (POVSnapshot *)malloc(sizeof(*snapshot));
    if (snapshot == NULL) {
        fprintf(stderr, "PortView: out of memory\n");
        return 20;
    }
    memset(snapshot, 0, sizeof(*snapshot));
    take_snapshot(snapshot);
    if (kv) print_kv(snapshot, filter); else print_human(snapshot, filter);
    rc = snapshot->truncated ? 5 : 0;
    free(snapshot);
    return rc;
}
