#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TV_MAX_RECORDS 256

typedef struct {
    unsigned long address;
    long priority;
    char state[16];
    char name[96];
} TVRecord;

typedef struct {
    TVRecord records[TV_MAX_RECORDS];
    unsigned int count;
    int truncated;
} TVSnapshot;

static void usage(void)
{
    fprintf(stderr, "Usage: TaskView [--kv] [--state all|ready|wait|current]\n");
}

static void copy_name(char *dst, size_t dst_size, const char *src)
{
    if (dst_size == 0) return;
    if (src == NULL) src = "<unnamed>";
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

static void add_record(TVSnapshot *snapshot, unsigned long address, long priority,
                       const char *state, const char *name)
{
    TVRecord *r;
    if (snapshot->count >= TV_MAX_RECORDS) {
        snapshot->truncated = 1;
        return;
    }
    r = &snapshot->records[snapshot->count++];
    r->address = address;
    r->priority = priority;
    copy_name(r->state, sizeof(r->state), state);
    copy_name(r->name, sizeof(r->name), name);
}

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <proto/exec.h>
extern struct ExecBase *SysBase;

static void snapshot_list(TVSnapshot *snapshot, struct List *list, const char *state)
{
    struct Node *node;
    for (node = list->lh_Head; node != NULL && node->ln_Succ != NULL; node = node->ln_Succ) {
        struct Task *task = (struct Task *)node;
        add_record(snapshot, (unsigned long)task, (long)task->tc_Node.ln_Pri,
                   state, task->tc_Node.ln_Name);
        if (snapshot->truncated) break;
    }
}

static int take_snapshot(TVSnapshot *snapshot)
{
    struct Task *current;
    memset(snapshot, 0, sizeof(*snapshot));

    Forbid();
    current = FindTask(NULL);
    if (current != NULL) {
        add_record(snapshot, (unsigned long)current, (long)current->tc_Node.ln_Pri,
                   "current", current->tc_Node.ln_Name);
    }
    if (!snapshot->truncated) snapshot_list(snapshot, &SysBase->TaskReady, "ready");
    if (!snapshot->truncated) snapshot_list(snapshot, &SysBase->TaskWait, "wait");
    Permit();
    return 0;
}
#else
static int take_snapshot(TVSnapshot *snapshot)
{
    memset(snapshot, 0, sizeof(*snapshot));
    add_record(snapshot, 0x1000UL, 0, "current", "TaskView-host-stub");
    add_record(snapshot, 0x1100UL, 5, "ready", "ready.example");
    add_record(snapshot, 0x1200UL, -5, "wait", "wait.example");
    return 0;
}
#endif

static int state_matches(const char *wanted, const char *actual)
{
    return strcmp(wanted, "all") == 0 || strcmp(wanted, actual) == 0;
}

static unsigned int matching_count(const TVSnapshot *snapshot, const char *state)
{
    unsigned int i, count = 0;
    for (i = 0; i < snapshot->count; ++i)
        if (state_matches(state, snapshot->records[i].state)) ++count;
    return count;
}

static void print_human(const TVSnapshot *snapshot, const char *state)
{
    unsigned int i;
    printf("TaskView task snapshot\n");
    for (i = 0; i < snapshot->count; ++i) {
        const TVRecord *r = &snapshot->records[i];
        if (!state_matches(state, r->state)) continue;
        printf("%-7s pri=%4ld addr=%08lX %s\n", r->state, r->priority,
               r->address, r->name);
    }
    printf("record_count=%u truncated=%s\n", matching_count(snapshot, state),
           snapshot->truncated ? "true" : "false");
}

static void print_kv(const TVSnapshot *snapshot, const char *state)
{
    unsigned int i, out = 0;
    printf("tool=TaskView\n");
    printf("schema=amiforensics.snapshot.kv/1\n");
    printf("filter_state=%s\n", state);
    for (i = 0; i < snapshot->count; ++i) {
        const TVRecord *r = &snapshot->records[i];
        if (!state_matches(state, r->state)) continue;
        printf("record.%u.kind=task\n", out);
        printf("record.%u.state=%s\n", out, r->state);
        printf("record.%u.name=%s\n", out, r->name);
        printf("record.%u.address=%08lX\n", out, r->address);
        printf("record.%u.priority=%ld\n", out, r->priority);
        ++out;
    }
    printf("record_count=%u\n", out);
    printf("truncated=%s\n", snapshot->truncated ? "true" : "false");
}

int main(int argc, char **argv)
{
    const char *state = "all";
    int kv = 0;
    int i;
    TVSnapshot *snapshot;
    int rc;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) {
            kv = 1;
        } else if (strcmp(argv[i], "--state") == 0 && i + 1 < argc) {
            state = argv[++i];
            if (strcmp(state, "all") != 0 && strcmp(state, "ready") != 0 &&
                strcmp(state, "wait") != 0 && strcmp(state, "current") != 0) {
                usage();
                return 10;
            }
        } else {
            usage();
            return 10;
        }
    }

    snapshot = (TVSnapshot *)malloc(sizeof(*snapshot));
    if (snapshot == NULL) {
        fprintf(stderr, "TaskView: out of memory\n");
        return 20;
    }

    rc = take_snapshot(snapshot);
    if (rc == 0) {
        if (kv) print_kv(snapshot, state);
        else print_human(snapshot, state);
        if (snapshot->truncated) rc = 5;
    }
    free(snapshot);
    return rc;
}
