#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DW_MAX_EVENTS 128U
#define DW_MAX_SAMPLES 32U
#define DW_DEFAULT_SAMPLES 4U
#define DW_DEFAULT_INTERVAL_TICKS 10U
#define DW_MAX_INTERVAL_TICKS 100U
#define DW_NAME_LEN 64U

typedef struct {
    unsigned long sequence;
    unsigned long previous_change;
    unsigned long current_change;
} DWEvent;

typedef struct {
    DWEvent events[DW_MAX_EVENTS];
    unsigned int count;
    int truncated;
    unsigned long initial_change;
    unsigned long final_change;
} DWSnapshot;

static void add_event(DWSnapshot *s, unsigned long previous_change, unsigned long current_change)
{
    DWEvent *r;
    if (s->count >= DW_MAX_EVENTS) {
        s->truncated = 1;
        return;
    }
    r = &s->events[s->count];
    r->sequence = (unsigned long)s->count;
    r->previous_change = previous_change;
    r->current_change = current_change;
    s->count++;
}

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#include <devices/trackdisk.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <proto/dos.h>
#include <proto/exec.h>

typedef struct {
    struct MsgPort *port;
    struct IOStdReq *request;
} DWDevice;

static int open_device(DWDevice *dev, const char *name, unsigned int unit)
{
    memset(dev, 0, sizeof(*dev));
    dev->port = CreateMsgPort();
    if (dev->port == NULL) return 20;
    dev->request = (struct IOStdReq *)CreateIORequest(dev->port, sizeof(struct IOStdReq));
    if (dev->request == NULL) {
        DeleteMsgPort(dev->port);
        dev->port = NULL;
        return 20;
    }
    if (OpenDevice((STRPTR)name, (ULONG)unit, (struct IORequest *)dev->request, 0) != 0) {
        DeleteIORequest((struct IORequest *)dev->request);
        DeleteMsgPort(dev->port);
        dev->request = NULL;
        dev->port = NULL;
        return 20;
    }
    return 0;
}

static void close_device(DWDevice *dev)
{
    if (dev->request != NULL) {
        CloseDevice((struct IORequest *)dev->request);
        DeleteIORequest((struct IORequest *)dev->request);
    }
    if (dev->port != NULL) DeleteMsgPort(dev->port);
    memset(dev, 0, sizeof(*dev));
}

static int query_change_number(DWDevice *dev, unsigned long *value)
{
    dev->request->io_Command = TD_CHANGENUM;
    dev->request->io_Flags = 0;
    dev->request->io_Error = 0;
    dev->request->io_Actual = 0;
    dev->request->io_Length = 0;
    dev->request->io_Data = NULL;
    dev->request->io_Offset = 0;
    if (DoIO((struct IORequest *)dev->request) != 0 || dev->request->io_Error != 0) return 20;
    *value = (unsigned long)dev->request->io_Actual;
    return 0;
}

static void wait_interval(unsigned int ticks)
{
    Delay((LONG)ticks);
}
#else
static unsigned int host_sample;

static int query_change_number(void *unused, unsigned long *value)
{
    (void)unused;
    if (host_sample < 2U) *value = 7UL;
    else *value = 8UL;
    host_sample++;
    return 0;
}

static void wait_interval(unsigned int ticks)
{
    (void)ticks;
}
#endif

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

static void print_kv(const DWSnapshot *s, const char *device, unsigned int unit,
                     unsigned int samples, unsigned int interval_ticks)
{
    unsigned int i;
    printf("tool=DiskWatch\n");
    printf("schema=amiforensics.trace.kv/1\n");
    printf("mode=disk-change-watch\n");
    printf("hooking=false\n");
    printf("observation=TD_CHANGENUM\n");
    printf("device=%s\n", device);
    printf("unit=%u\n", unit);
    printf("samples_requested=%u\n", samples);
    printf("interval_ticks=%u\n", interval_ticks);
    printf("initial_change=%lu\n", s->initial_change);
    printf("final_change=%lu\n", s->final_change);
    for (i = 0; i < s->count; ++i) {
        printf("record.%u.kind=event\n", i);
        printf("record.%u.sequence=%lu\n", i, s->events[i].sequence);
        printf("record.%u.event=disk-change-number-changed\n", i);
        printf("record.%u.previous_change=%lu\n", i, s->events[i].previous_change);
        printf("record.%u.current_change=%lu\n", i, s->events[i].current_change);
    }
    printf("record_count=%u\n", s->count);
    printf("truncated=%s\n", s->truncated ? "true" : "false");
}

static void print_human(const DWSnapshot *s, const char *device, unsigned int unit,
                        unsigned int samples, unsigned int interval_ticks)
{
    unsigned int i;
    printf("DiskWatch bounded disk-change observation\n");
    printf("Device: %s unit %u\n", device, unit);
    printf("Primitive: TD_CHANGENUM\n");
    printf("Hooking: disabled\n");
    printf("Samples: %u, interval ticks: %u\n", samples, interval_ticks);
    printf("Initial change number: %lu\n", s->initial_change);
    for (i = 0; i < s->count; ++i)
        printf("%3lu change-number %lu -> %lu\n", s->events[i].sequence,
               s->events[i].previous_change, s->events[i].current_change);
    printf("Final change number: %lu\n", s->final_change);
    printf("Records: %u%s\n", s->count, s->truncated ? " (truncated)" : "");
}

static void usage(void)
{
    fprintf(stderr, "Usage: DiskWatch [--kv] [--device NAME] [--unit N] [--samples N] [--interval TICKS]\n");
}

int main(int argc, char **argv)
{
    DWSnapshot *snapshot;
    const char *device = "trackdisk.device";
    unsigned int unit = 0U;
    unsigned int samples = DW_DEFAULT_SAMPLES;
    unsigned int interval_ticks = DW_DEFAULT_INTERVAL_TICKS;
    unsigned long previous = 0UL;
    unsigned long current = 0UL;
    unsigned int sample;
    int kv = 0;
    int i;
    int rc;
#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
    DWDevice dev;
#endif

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) kv = 1;
        else if (strcmp(argv[i], "--device") == 0) {
            if (i + 1 >= argc || argv[i + 1][0] == '\0') { usage(); return 10; }
            device = argv[++i];
        } else if (strcmp(argv[i], "--unit") == 0) {
            if (i + 1 >= argc || !parse_uint(argv[++i], 0U, 255U, &unit)) { usage(); return 10; }
        } else if (strcmp(argv[i], "--samples") == 0) {
            if (i + 1 >= argc || !parse_uint(argv[++i], 2U, DW_MAX_SAMPLES, &samples)) { usage(); return 10; }
        } else if (strcmp(argv[i], "--interval") == 0) {
            if (i + 1 >= argc || !parse_uint(argv[++i], 1U, DW_MAX_INTERVAL_TICKS, &interval_ticks)) { usage(); return 10; }
        } else {
            usage();
            return 10;
        }
    }

    snapshot = (DWSnapshot *)malloc(sizeof(*snapshot));
    if (snapshot == NULL) return 20;
    memset(snapshot, 0, sizeof(*snapshot));

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
    rc = open_device(&dev, device, unit);
    if (rc != 0) { free(snapshot); return rc; }
    rc = query_change_number(&dev, &previous);
#else
    rc = query_change_number(NULL, &previous);
#endif
    if (rc != 0) {
#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
        close_device(&dev);
#endif
        free(snapshot);
        return rc;
    }
    snapshot->initial_change = previous;

    for (sample = 1U; sample < samples; ++sample) {
        wait_interval(interval_ticks);
#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
        rc = query_change_number(&dev, &current);
#else
        rc = query_change_number(NULL, &current);
#endif
        if (rc != 0) break;
        if (current != previous) add_event(snapshot, previous, current);
        previous = current;
    }
    snapshot->final_change = previous;

#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
    close_device(&dev);
#endif

    if (rc == 0) {
        if (kv) print_kv(snapshot, device, unit, samples, interval_ticks);
        else print_human(snapshot, device, unit, samples, interval_ticks);
        rc = snapshot->truncated ? 5 : 0;
    }
    free(snapshot);
    return rc;
}
