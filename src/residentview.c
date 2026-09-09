#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RV_MAX_RECORDS 192
#define RV_NAME_LEN 80

typedef struct {
    char kind[16];
    char name[RV_NAME_LEN];
    unsigned long address;
    long priority;
    unsigned long version;
} RVRecord;

typedef struct {
    RVRecord records[RV_MAX_RECORDS];
    unsigned int count;
    unsigned int truncated;
} RVSnapshot;

static void snapshot_init(RVSnapshot *s)
{
    memset(s, 0, sizeof(*s));
}

static void add_record(RVSnapshot *s, const char *kind, const char *name,
                       unsigned long address, long priority,
                       unsigned long version)
{
    RVRecord *r;

    if (s->count >= RV_MAX_RECORDS) {
        s->truncated = 1;
        return;
    }

    r = &s->records[s->count++];
    strncpy(r->kind, kind ? kind : "unknown", sizeof(r->kind) - 1);
    r->kind[sizeof(r->kind) - 1] = '\0';
    strncpy(r->name, name ? name : "<unnamed>", sizeof(r->name) - 1);
    r->name[sizeof(r->name) - 1] = '\0';
    r->address = address;
    r->priority = priority;
    r->version = version;
}

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/libraries.h>
#include <exec/tasks.h>
#include <exec/ports.h>
#include <proto/exec.h>

extern struct ExecBase *SysBase;

static void snapshot_node_list(RVSnapshot *s, struct List *list, const char *kind)
{
    struct Node *node;

    for (node = list->lh_Head; node != NULL && node->ln_Succ != NULL;
         node = node->ln_Succ) {
        add_record(s, kind, node->ln_Name,
                   (unsigned long)node, (long)node->ln_Pri, 0);
    }
}

static void snapshot_library_list(RVSnapshot *s, struct List *list,
                                  const char *kind)
{
    struct Library *lib;

    for (lib = (struct Library *)list->lh_Head;
         lib != NULL && lib->lib_Node.ln_Succ != NULL;
         lib = (struct Library *)lib->lib_Node.ln_Succ) {
        add_record(s, kind, lib->lib_Node.ln_Name,
                   (unsigned long)lib, (long)lib->lib_Node.ln_Pri,
                   ((unsigned long)lib->lib_Version << 16) |
                   (unsigned long)lib->lib_Revision);
    }
}

static void take_snapshot(RVSnapshot *s)
{
    struct Task *current;

    Forbid();

    snapshot_library_list(s, &SysBase->LibList, "library");
    snapshot_library_list(s, &SysBase->DeviceList, "device");
    snapshot_node_list(s, &SysBase->PortList, "port");
    snapshot_node_list(s, &SysBase->ResourceList, "resource");
    snapshot_node_list(s, &SysBase->TaskReady, "task-ready");
    snapshot_node_list(s, &SysBase->TaskWait, "task-wait");

    current = FindTask(NULL);
    if (current != NULL) {
        add_record(s, "task-running", current->tc_Node.ln_Name,
                   (unsigned long)current, (long)current->tc_Node.ln_Pri, 0);
    }

    Permit();
}
#else
static void take_snapshot(RVSnapshot *s)
{
    /* Host build exists only to exercise shared formatting and CLI parsing. */
    add_record(s, "host-stub", "Amiga Exec snapshot unavailable",
               0, 0, 0);
}
#endif

static int kind_matches(const char *wanted, const char *actual)
{
    if (wanted == NULL || strcmp(wanted, "all") == 0)
        return 1;
    if (strcmp(wanted, actual) == 0)
        return 1;
    if (strcmp(wanted, "tasks") == 0 && strncmp(actual, "task-", 5) == 0)
        return 1;
    return 0;
}

static void print_human(const RVSnapshot *s, const char *filter)
{
    unsigned int i;

    printf("ResidentView snapshot\n");
    for (i = 0; i < s->count; ++i) {
        const RVRecord *r = &s->records[i];
        if (!kind_matches(filter, r->kind))
            continue;
        printf("%-12s %08lX pri=%ld", r->kind, r->address, r->priority);
        if (r->version != 0)
            printf(" ver=%lu.%lu", r->version >> 16, r->version & 0xFFFFUL);
        printf(" %s\n", r->name);
    }
    if (s->truncated)
        printf("WARNING: snapshot truncated at %u records\n", RV_MAX_RECORDS);
}

static void print_kv(const RVSnapshot *s, const char *filter)
{
    unsigned int i;
    unsigned int out = 0;

    printf("tool=ResidentView\n");
    for (i = 0; i < s->count; ++i) {
        const RVRecord *r = &s->records[i];
        if (!kind_matches(filter, r->kind))
            continue;
        printf("record.%u.kind=%s\n", out, r->kind);
        printf("record.%u.address=%08lX\n", out, r->address);
        printf("record.%u.priority=%ld\n", out, r->priority);
        printf("record.%u.version=%lu\n", out, r->version);
        printf("record.%u.name=%s\n", out, r->name);
        ++out;
    }
    printf("record_count=%u\n", out);
    printf("truncated=%s\n", s->truncated ? "true" : "false");
}

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [--kv] [--kind all|library|device|port|resource|tasks|task-ready|task-wait|task-running]\n",
            prog);
}

int main(int argc, char **argv)
{
    RVSnapshot snapshot;
    const char *filter = "all";
    int kv = 0;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) {
            kv = 1;
        } else if (strcmp(argv[i], "--kind") == 0 && i + 1 < argc) {
            filter = argv[++i];
        } else {
            usage(argv[0]);
            return 10;
        }
    }

    snapshot_init(&snapshot);
    take_snapshot(&snapshot);

    if (kv)
        print_kv(&snapshot, filter);
    else
        print_human(&snapshot, filter);

    return snapshot.truncated ? 5 : 0;
}
