#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TE_MAX_EVENTS 256U
#define TE_NAME_LEN 96U

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

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
extern struct ExecBase *SysBase;

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
static void take_snapshot(TESnapshot *s)
{
    memset(s, 0, sizeof(*s));
    add_event(s, "snapshot-task", "TraceExec-host-stub", 0x00123400UL);
    add_event(s, "observation", "host-deterministic-event", 0x00123456UL);
}
#endif

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

static void print_human(const TESnapshot *s)
{
    unsigned int i;
    printf("TraceExec event observation\n");
    printf("Hooking: disabled (M4.1 foundation)\n");
    for (i = 0; i < s->count; ++i)
        printf("%3lu %-16s %08lX %s\n", s->events[i].sequence,
               s->events[i].event, s->events[i].address & 0xFFFFFFFFUL,
               s->events[i].name);
    printf("Records: %u%s\n", s->count, s->truncated ? " (truncated)" : "");
}

static void usage(void)
{
    fprintf(stderr, "Usage: TraceExec [--kv]\n");
}

int main(int argc, char **argv)
{
    TESnapshot *snapshot;
    int kv = 0;
    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) kv = 1;
        else { usage(); return 10; }
    }
    snapshot = (TESnapshot *)malloc(sizeof(*snapshot));
    if (snapshot == NULL) return 20;
    take_snapshot(snapshot);
    if (kv) print_kv(snapshot); else print_human(snapshot);
    i = snapshot->truncated ? 5 : 0;
    free(snapshot);
    return i;
}
