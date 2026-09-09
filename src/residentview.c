#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define RV_MAX_RECORDS 192
#define RV_NAME_LEN 80
#define RV_RESULT_LEN 4096
#define RV_DEFAULT_PORT "RESIDENTVIEW"

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

static void snapshot_init(RVSnapshot *s) { memset(s, 0, sizeof(*s)); }

static void add_record(RVSnapshot *s, const char *kind, const char *name,
                       unsigned long address, long priority,
                       unsigned long version)
{
    RVRecord *r;
    if (s->count >= RV_MAX_RECORDS) { s->truncated = 1; return; }
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
#include <exec/resident.h>
#include <proto/exec.h>
#ifndef RV_NO_AREXX
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <proto/rexxsyslib.h>
#endif

extern struct ExecBase *SysBase;

static void snapshot_node_list(RVSnapshot *s, struct List *list, const char *kind)
{
    struct Node *node;
    for (node = list->lh_Head; node != NULL && node->ln_Succ != NULL; node = node->ln_Succ)
        add_record(s, kind, (const char *)node->ln_Name, (unsigned long)node, (long)node->ln_Pri, 0);
}

static void snapshot_library_list(RVSnapshot *s, struct List *list, const char *kind)
{
    struct Library *lib;
    for (lib = (struct Library *)list->lh_Head;
         lib != NULL && lib->lib_Node.ln_Succ != NULL;
         lib = (struct Library *)lib->lib_Node.ln_Succ)
        add_record(s, kind, (const char *)lib->lib_Node.ln_Name, (unsigned long)lib,
                   (long)lib->lib_Node.ln_Pri,
                   ((unsigned long)lib->lib_Version << 16) | (unsigned long)lib->lib_Revision);
}

static void snapshot_residents(RVSnapshot *s)
{
    struct Resident **mods = (struct Resident **)SysBase->ResModules;
    unsigned int i;
    if (mods == NULL) return;
    for (i = 0; mods[i] != NULL && i < 256; ++i) {
        struct Resident *r = mods[i];
        if (r->rt_MatchWord != RTC_MATCHWORD || r->rt_MatchTag != r) continue;
        add_record(s, "resident", (const char *)r->rt_Name, (unsigned long)r,
                   (long)r->rt_Pri, ((unsigned long)r->rt_Version << 16));
    }
}

static void take_snapshot(RVSnapshot *s)
{
    struct Task *current;
    Forbid();
    snapshot_residents(s);
    snapshot_library_list(s, &SysBase->LibList, "library");
    snapshot_library_list(s, &SysBase->DeviceList, "device");
    snapshot_node_list(s, &SysBase->PortList, "port");
    snapshot_node_list(s, &SysBase->ResourceList, "resource");
    snapshot_node_list(s, &SysBase->TaskReady, "task-ready");
    snapshot_node_list(s, &SysBase->TaskWait, "task-wait");
    current = FindTask(NULL);
    if (current != NULL)
        add_record(s, "task-running", (const char *)current->tc_Node.ln_Name,
                   (unsigned long)current, (long)current->tc_Node.ln_Pri, 0);
    Permit();
}
#else
static void take_snapshot(RVSnapshot *s)
{
    add_record(s, "host-stub", "Amiga Exec snapshot unavailable", 0, 0, 0);
    add_record(s, "resident", "host-resident-stub", 0, 0, 0);
}
#endif

static int kind_matches(const char *wanted, const char *actual)
{
    if (wanted == NULL || strcmp(wanted, "all") == 0) return 1;
    if (strcmp(wanted, actual) == 0) return 1;
    if (strcmp(wanted, "tasks") == 0 && strncmp(actual, "task-", 5) == 0) return 1;
    if (strcmp(wanted, "libraries") == 0 && strcmp(actual, "library") == 0) return 1;
    if (strcmp(wanted, "devices") == 0 && strcmp(actual, "device") == 0) return 1;
    if (strcmp(wanted, "ports") == 0 && strcmp(actual, "port") == 0) return 1;
    if (strcmp(wanted, "resources") == 0 && strcmp(actual, "resource") == 0) return 1;
    if (strcmp(wanted, "residents") == 0 && strcmp(actual, "resident") == 0) return 1;
    return 0;
}

static void print_human(const RVSnapshot *s, const char *filter)
{
    unsigned int i;
    printf("ResidentView snapshot\n");
    for (i = 0; i < s->count; ++i) {
        const RVRecord *r = &s->records[i];
        if (!kind_matches(filter, r->kind)) continue;
        printf("%-12s %08lX pri=%ld", r->kind, r->address, r->priority);
        if (r->version != 0) printf(" ver=%lu.%lu", r->version >> 16, r->version & 0xFFFFUL);
        printf(" %s\n", r->name);
    }
    if (s->truncated) printf("WARNING: snapshot truncated at %u records\n", RV_MAX_RECORDS);
}

static void print_kv(const RVSnapshot *s, const char *filter)
{
    unsigned int i, out = 0;
    printf("tool=ResidentView\n");
    for (i = 0; i < s->count; ++i) {
        const RVRecord *r = &s->records[i];
        if (!kind_matches(filter, r->kind)) continue;
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

static int append_text(char *dst, size_t cap, size_t *used, const char *text)
{
    size_t n = strlen(text);
    if (*used + n + 1 > cap) return 0;
    memcpy(dst + *used, text, n); *used += n; dst[*used] = '\0'; return 1;
}

static int snapshot_to_text(const RVSnapshot *s, const char *filter, char *dst, size_t cap)
{
    unsigned int i, out = 0; size_t used = 0; char line[192];
    dst[0] = '\0';
    for (i = 0; i < s->count; ++i) {
        const RVRecord *r = &s->records[i];
        if (!kind_matches(filter, r->kind)) continue;
        sprintf(line, "%s|%08lX|%ld|%lu|%s\n", r->kind, r->address, r->priority, r->version, r->name);
        if (!append_text(dst, cap, &used, line)) return 0;
        ++out;
    }
    sprintf(line, "record_count=%u truncated=%s", out, s->truncated ? "true" : "false");
    return append_text(dst, cap, &used, line);
}

static void normalize_command(const char *src, char *dst, size_t cap)
{
    size_t i = 0;
    while (*src != '\0' && i + 1 < cap) {
        unsigned char c = (unsigned char)*src++;
        if (c == '\t') c = ' ';
        dst[i++] = (char)toupper(c);
    }
    while (i > 0 && dst[i - 1] == ' ') --i;
    dst[i] = '\0';
}

static const char *command_filter(const char *cmd)
{
    if (strcmp(cmd, "SNAPSHOT") == 0 || strcmp(cmd, "LIST ALL") == 0) return "all";
    if (strcmp(cmd, "LIST LIBRARIES") == 0) return "libraries";
    if (strcmp(cmd, "LIST DEVICES") == 0) return "devices";
    if (strcmp(cmd, "LIST PORTS") == 0) return "ports";
    if (strcmp(cmd, "LIST TASKS") == 0) return "tasks";
    if (strcmp(cmd, "LIST RESOURCES") == 0) return "resources";
    if (strcmp(cmd, "LIST RESIDENTS") == 0) return "residents";
    return NULL;
}

#if (defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)) && !defined(RV_NO_AREXX)
static void reply_rexx(struct RexxMsg *msg, long rc, const char *result, long error_code)
{
    msg->rm_Result1 = rc; msg->rm_Result2 = 0;
    if (rc == 0 && result != NULL && (msg->rm_Action & RXFF_RESULT) != 0) {
        STRPTR arg = CreateArgstring((STRPTR)result, (LONG)strlen(result));
        if (arg != NULL) msg->rm_Result2 = (LONG)arg; else msg->rm_Result1 = 20;
    } else if (rc != 0) msg->rm_Result2 = error_code;
    ReplyMsg((struct Message *)msg);
}

static int serve_arexx(const char *port_name)
{
    struct MsgPort *port; int quitting = 0;
    RexxSysBase = (void *)OpenLibrary((CONST_STRPTR)"rexxsyslib.library", 0);
    if (RexxSysBase == NULL) { fprintf(stderr, "ResidentView: cannot open rexxsyslib.library\n"); return 20; }
    Forbid();
    if (FindPort((CONST_STRPTR)port_name) != NULL) {
        Permit(); fprintf(stderr, "ResidentView: ARexx port '%s' already exists\n", port_name);
        CloseLibrary((struct Library *)RexxSysBase); RexxSysBase = NULL; return 20;
    }
    Permit();
    port = CreateMsgPort();
    if (port == NULL) { CloseLibrary((struct Library *)RexxSysBase); RexxSysBase = NULL; return 20; }
    port->mp_Node.ln_Name = (char *)port_name; AddPort(port);
    printf("ResidentView ARexx port: %s\n", port_name);
    while (!quitting) {
        struct RexxMsg *msg; ULONG sigmask = 1UL << port->mp_SigBit;
        ULONG signals = Wait(sigmask | SIGBREAKF_CTRL_C);
        if (signals & SIGBREAKF_CTRL_C) break;
        while ((msg = (struct RexxMsg *)GetMsg(port)) != NULL) {
            char cmd[96]; const char *filter;
            if (!IsRexxMsg(msg) || msg->rm_Args[0] == NULL) { reply_rexx(msg, 10, NULL, 1); continue; }
            normalize_command((const char *)msg->rm_Args[0], cmd, sizeof(cmd));
            if (strcmp(cmd, "PING") == 0) { reply_rexx(msg, 0, "PONG", 0); continue; }
            if (strcmp(cmd, "QUIT") == 0) { reply_rexx(msg, 0, "BYE", 0); quitting = 1; continue; }
            filter = command_filter(cmd);
            if (filter != NULL) {
                RVSnapshot snapshot; char result[RV_RESULT_LEN];
                snapshot_init(&snapshot); take_snapshot(&snapshot);
                if (!snapshot_to_text(&snapshot, filter, result, sizeof(result))) reply_rexx(msg, 5, NULL, 2);
                else if (snapshot.truncated) reply_rexx(msg, 5, NULL, 3);
                else reply_rexx(msg, 0, result, 0);
                continue;
            }
            reply_rexx(msg, 10, NULL, 1);
        }
    }
    RemPort(port); DeleteMsgPort(port); CloseLibrary((struct Library *)RexxSysBase); RexxSysBase = NULL; return 0;
}
#elif defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
static int serve_arexx(const char *port_name)
{
    (void)port_name;
    fprintf(stderr, "ResidentView: ARexx support not included in this build\n");
    return 20;
}
#else
static int serve_arexx(const char *port_name)
{
    printf("ResidentView ARexx host stub: %s\n", port_name);
    printf("PING=PONG\n");
    return 0;
}
#endif

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [--kv] [--kind all|resident|residents|library|libraries|device|devices|port|ports|resource|resources|tasks|task-ready|task-wait|task-running]\n"
            "       %s --serve [PORT]\n", prog, prog);
}

int main(int argc, char **argv)
{
    RVSnapshot snapshot; const char *filter = "all"; int kv = 0, i;
    if (argc >= 2 && strcmp(argv[1], "--serve") == 0) {
        const char *port = (argc >= 3) ? argv[2] : RV_DEFAULT_PORT;
        if (argc > 3) { usage(argv[0]); return 10; }
        return serve_arexx(port);
    }
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kv") == 0) kv = 1;
        else if (strcmp(argv[i], "--kind") == 0 && i + 1 < argc) filter = argv[++i];
        else { usage(argv[0]); return 10; }
    }
    snapshot_init(&snapshot); take_snapshot(&snapshot);
    if (kv) print_kv(&snapshot, filter); else print_human(&snapshot, filter);
    return snapshot.truncated ? 5 : 0;
}
