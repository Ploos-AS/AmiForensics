#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TE_MAX_EVENTS 256U
#define TE_MAX_TASKS 192U
#define TE_NAME_LEN 96U
#define TE_MIN_WATCH_SAMPLES 2U
#define TE_MAX_WATCH_SAMPLES 16U
#define TE_DEFAULT_INTERVAL_TICKS 5U
#define TE_MAX_INTERVAL_TICKS 50U

typedef struct {
    unsigned long sequence;
    char event[24];
    char name[TE_NAME_LEN];
    unsigned long address;
} TEEvent;

typedef struct {
    TEEvent events[TE_MAX_EVENTS];
    unsigned int count;
    int truncated;
} TESnapshot;

typedef struct {
    unsigned long address;
    char name[TE_NAME_LEN];
} TETaskRecord;

typedef struct {
    TETaskRecord records[TE_MAX_TASKS];
    unsigned int count;
    int truncated;
} TETaskSet;

static void add_event(TESnapshot *s, const char *event, const char *name, unsigned long address)
{
    TEEvent *r;
    if (s->count >= TE_MAX_EVENTS) { s->truncated = 1; return; }
    r = &s->events[s->count];
    r->sequence = (unsigned long)s->count;
    strncpy(r->event, event ? event : "unknown", sizeof(r->event) - 1U);
    r->event[sizeof(r->event) - 1U] = '\0';
    strncpy(r->name, name ? name : "", sizeof(r->name) - 1U);
    r->name[sizeof(r->name) - 1U] = '\0';
    r->address = address;
    s->count++;
}

static int taskset_contains(const TETaskSet *set, unsigned long address)
{
    unsigned int i;
    for (i = 0; i < set->count; ++i)
        if (set->records[i].address == address) return 1;
    return 0;
}

static const TETaskRecord *taskset_find(const TETaskSet *set, unsigned long address)
{
    unsigned int i;
    for (i = 0; i < set->count; ++i)
        if (set->records[i].address == address) return &set->records[i];
    return NULL;
}

static void add_task_record(TETaskSet *set, unsigned long address, const char *name)
{
    TETaskRecord *r;
    if (address == 0UL || taskset_contains(set, address)) return;
    if (set->count >= TE_MAX_TASKS) { set->truncated = 1; return; }
    r = &set->records[set->count++];
    r->address = address;
    strncpy(r->name, name ? name : "", sizeof(r->name) - 1U);
    r->name[sizeof(r->name) - 1U] = '\0';
}

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/exec.h>
extern struct ExecBase *SysBase;

static void snapshot_task_list(TETaskSet *set, struct List *list)
{
    struct Node *node;
    for (node = list->lh_Head; node != NULL && node->ln_Succ != NULL; node = node->ln_Succ)
        add_task_record(set, (unsigned long)node, node->ln_Name);
}

static void take_task_set(TETaskSet *set)
{
    struct Task *task;
    memset(set, 0, sizeof(*set));
    Forbid();
    task = FindTask(NULL);
    if (task != NULL)
        add_task_record(set, (unsigned long)task, task->tc_Node.ln_Name);
    snapshot_task_list(set, &SysBase->TaskReady);
    snapshot_task_list(set, &SysBase->TaskWait);
    Permit();
}

static void wait_interval(unsigned int ticks)
{
    Delay((LONG)ticks);
}

static void take_snapshot(TESnapshot *s)
{
    struct Task *task;
    memset(s, 0, sizeof(*s));
    Forbid();
    task = FindTask(NULL);
    if (task != NULL)
        add_event(s, "snapshot-task", task->tc_Node.ln_Name, (unsigned long)task);
    Permit();
}
#else
static unsigned int host_task_sample;

static void take_task_set(TETaskSet *set)
{
    memset(set, 0, sizeof(*set));
    if (host_task_sample == 0U) {
        add_task_record(set, 0x00100000UL, "host-task-a");
        add_task_record(set, 0x00101000UL, "host-task-b");
    } else if (host_task_sample == 1U) {
        add_task_record(set, 0x00100000UL, "host-task-a");
        add_task_record(set, 0x00102000UL, "host-task-c");
    } else {
        add_task_record(set, 0x00100000UL, "host-task-a");
        add_task_record(set, 0x00102000UL, "host-task-c");
    }
    host_task_sample++;
}

static void wait_interval(unsigned int ticks)
{
    (void)ticks;
}

static void take_snapshot(TESnapshot *s)
{
    memset(s, 0, sizeof(*s));
    add_event(s, "snapshot-task", "TraceExec-host-stub", 0x00123400UL);
    add_event(s, "observation", "host-deterministic-event", 0x00123456UL);
}
#endif

static void compare_task_sets(TESnapshot *events, const TETaskSet *before, const TETaskSet *after)
{
    unsigned int i;
    for (i = 0; i < after->count; ++i) {
        if (!taskset_contains(before, after->records[i].address))
            add_event(events, "task-added", after->records[i].name, after->records[i].address);
    }
    for (i = 0; i < before->count; ++i) {
        if (!taskset_contains(after, before->records[i].address))
            add_event(events, "task-removed", before->records[i].name, before->records[i].address);
    }
}

static int watch_tasks(TESnapshot *events, unsigned int samples, unsigned int interval_ticks,
                       unsigned int *taskset_truncated)
{
    TETaskSet *before;
    TETaskSet *after;
    TETaskSet *tmp;
    unsigned int sample;

    before = (TETaskSet *)malloc(sizeof(*before));
    after = (TETaskSet *)malloc(sizeof(*after));
    if (before == NULL || after == NULL) {
        free(before);
        free(after);
        return 20;
    }

    memset(events, 0, sizeof(*events));
    *taskset_truncated = 0U;
    take_task_set(before);
    if (before->truncated) *taskset_truncated = 1U;
    for (sample = 1U; sample < samples; ++sample) {
        wait_interval(interval_ticks);
        take_task_set(after);
        if (after->truncated) *taskset_truncated = 1U;
        compare_task_sets(events, before, after);
        tmp = before;
        before = after;
        after = tmp;
    }

    free(before);
    free(after);
    return 0;
}

static void print_kv(const TESnapshot *s)
{
    unsigned int i;
    printf("tool=TraceExec\n");
    printf("schema=amiforensics.trace.kv/1\n");
    printf("mode=event-observation\n");
    printf("hooking=false\n");
    for (i = 0; i < s->count; ++i) {
        const TEEvent *r = &s->events[i];
        printf("record.%u.kind=event\n", i);
        printf("record.%u.sequence=%lu\n", i, r->sequence);
        printf("record.%u.event=%s\n", i, r->event);
        printf("record.%u.name=%s\n", i, r->name);
        printf("record.%u.address=%08lX\n", i, r->address & 0xFFFFFFFFUL);
    }
    printf("record_count=%u\n", s->count);
    printf("truncated=%s\n", s->truncated ? "true" : "false");
}

static void print_watch_kv(const TESnapshot *s, unsigned int samples, unsigned int interval_ticks,
                           unsigned int taskset_truncated)
{
    unsigned int i;
    printf("tool=TraceExec\n");
    printf("schema=amiforensics.trace.kv/1\n");
    printf("mode=task-lifecycle-watch\n");
    printf("hooking=false\n");
    printf("samples_requested=%u\n", samples);
    printf("interval_ticks=%u\n", interval_ticks);
    for (i = 0; i < s->count; ++i) {
        const TEEvent *r = &s->events[i];
        printf("record.%u.kind=event\n", i);
        printf("record.%u.sequence=%lu\n", i, r->sequence);
        printf("record.%u.event=%s\n", i, r->event);
        printf("record.%u.name=%s\n", i, r->name);
        printf("record.%u.address=%08lX\n", i, r->address & 0xFFFFFFFFUL);
    }
    printf("record_count=%u\n", s->count);
    printf("taskset_truncated=%s\n", taskset_truncated ? "true" : "false");
    printf("truncated=%s\n", s->truncated ? "true" : "false");
}

static void print_human(const TESnapshot *s)
{
    unsigned int i;
    printf("TraceExec event observation\n");
    printf("Hooking: disabled\n");
    for (i = 0; i < s->count; ++i)
        printf("%3lu %-16s %08lX %s\n", s->events[i].sequence,
               s->events[i].event, s->events[i].address & 0xFFFFFFFFUL,
               s->events[i].name);
    printf("Records: %u%s\n", s->count, s->truncated ? " (truncated)" : "");
}

static void print_watch_human(const TESnapshot *s, unsigned int samples, unsigned int interval_ticks,
                              unsigned int taskset_truncated)
{
    unsigned int i;
    printf("TraceExec bounded task lifecycle observation\n");
    printf("Hooking: disabled\n");
    printf("Samples: %u, interval ticks: %u\n", samples, interval_ticks);
    for (i = 0; i < s->count; ++i)
        printf("%3lu %-16s %08lX %s\n", s->events[i].sequence,
               s->events[i].event, s->events[i].address & 0xFFFFFFFFUL,
               s->events[i].name);
    printf("Records: %u%s%s\n", s->count,
           s->truncated ? " (event list truncated)" : "",
           taskset_truncated ? " (task snapshot truncated)" : "");
}

static int parse_uint(const char *text, unsigned int min_value, unsigned int max_value,
                      unsigned int *value)
{
    char *end = NULL;
    unsigned long parsed;
    if (text == NULL || *text == '\0') return 0;
    parsed = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || parsed < min_value || parsed > max_value) return 0;
    *value = (unsigned int)parsed;
    return 1;
}

static void usage(void)
{
    fprintf(stderr, "Usage: TraceExec [--kv] [--watch-tasks SAMPLES [--interval TICKS]]\n");
}

int main(int argc, char **argv)
{
    TESnapshot *snapshot;
    int kv = 0;
    int watch = 0;
    int rc;
    int i;
    unsigned int samples = 0U;
    unsigned int interval_ticks = TE_DEFAULT_INTERVAL_TICKS;
    unsigned int taskset_truncated = 0U;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) {
            kv = 1;
        } else if (strcmp(argv[i], "--watch-tasks") == 0) {
            if (watch || i + 1 >= argc ||
                !parse_uint(argv[++i], TE_MIN_WATCH_SAMPLES, TE_MAX_WATCH_SAMPLES, &samples)) {
                usage();
                return 10;
            }
            watch = 1;
        } else if (strcmp(argv[i], "--interval") == 0) {
            if (i + 1 >= argc ||
                !parse_uint(argv[++i], 1U, TE_MAX_INTERVAL_TICKS, &interval_ticks)) {
                usage();
                return 10;
            }
        } else {
            usage();
            return 10;
        }
    }

    if (!watch && interval_ticks != TE_DEFAULT_INTERVAL_TICKS) {
        usage();
        return 10;
    }

    snapshot = (TESnapshot *)malloc(sizeof(*snapshot));
    if (snapshot == NULL) return 20;

    if (watch) {
        rc = watch_tasks(snapshot, samples, interval_ticks, &taskset_truncated);
        if (rc != 0) { free(snapshot); return rc; }
        if (kv) print_watch_kv(snapshot, samples, interval_ticks, taskset_truncated);
        else print_watch_human(snapshot, samples, interval_ticks, taskset_truncated);
        rc = (snapshot->truncated || taskset_truncated) ? 5 : 0;
    } else {
        take_snapshot(snapshot);
        if (kv) print_kv(snapshot); else print_human(snapshot);
        rc = snapshot->truncated ? 5 : 0;
    }

    free(snapshot);
    return rc;
}
