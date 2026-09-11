#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMP_MAX_RECORDS 256U
#define CMP_LINE_LEN 512U
#define CMP_KEY_LEN 192U
#define CMP_VALUE_LEN 256U

typedef struct {
    char key[CMP_KEY_LEN];
    char value[CMP_VALUE_LEN];
} CMPRecord;

typedef struct {
    CMPRecord records[CMP_MAX_RECORDS];
    unsigned int count;
    int truncated;
} CMPSnapshot;

static void trim_line(char *s)
{
    size_t n = strlen(s);
    while (n > 0U && (s[n - 1U] == '\n' || s[n - 1U] == '\r')) s[--n] = '\0';
}

static int find_key(const CMPSnapshot *s, const char *key)
{
    unsigned int i;
    for (i = 0U; i < s->count; ++i)
        if (strcmp(s->records[i].key, key) == 0) return (int)i;
    return -1;
}

static int copy_field(char *dst, size_t dst_size, const char *src)
{
    size_t n = strlen(src);
    if (n >= dst_size) return 0;
    memcpy(dst, src, n + 1U);
    return 1;
}

static void discard_line_tail(FILE *fp)
{
    int ch;
    do {
        ch = fgetc(fp);
    } while (ch != '\n' && ch != EOF);
}

static int load_snapshot(const char *path, CMPSnapshot *s)
{
    FILE *fp;
    char line[CMP_LINE_LEN];
    memset(s, 0, sizeof(*s));
    fp = fopen(path, "r");
    if (fp == NULL) return 20;
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *eq;
        size_t line_len = strlen(line);
        int complete_line = (line_len > 0U && line[line_len - 1U] == '\n');

        if (!complete_line && !feof(fp)) {
            s->truncated = 1;
            discard_line_tail(fp);
            continue;
        }

        trim_line(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        eq = strchr(line, '=');
        if (eq == NULL || eq == line) continue;
        *eq++ = '\0';
        if (find_key(s, line) >= 0) continue;
        if (s->count >= CMP_MAX_RECORDS) { s->truncated = 1; continue; }
        if (!copy_field(s->records[s->count].key, CMP_KEY_LEN, line) ||
            !copy_field(s->records[s->count].value, CMP_VALUE_LEN, eq)) {
            s->truncated = 1;
            continue;
        }
        s->count++;
    }
    if (ferror(fp)) { fclose(fp); return 20; }
    fclose(fp);
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "Usage: Compare [--kv] <before.kv> <after.kv>\n");
}

int main(int argc, char **argv)
{
    const char *before_path;
    const char *after_path;
    CMPSnapshot *before;
    CMPSnapshot *after;
    unsigned int added = 0U, removed = 0U, modified = 0U, unchanged = 0U;
    unsigned int i;
    int kv = 0;
    int rc;

    if (argc == 4 && strcmp(argv[1], "--kv") == 0) { kv = 1; before_path = argv[2]; after_path = argv[3]; }
    else if (argc == 3) { before_path = argv[1]; after_path = argv[2]; }
    else { usage(); return 10; }

    before = (CMPSnapshot *)malloc(sizeof(*before));
    after = (CMPSnapshot *)malloc(sizeof(*after));
    if (before == NULL || after == NULL) { free(before); free(after); return 20; }
    rc = load_snapshot(before_path, before);
    if (rc == 0) rc = load_snapshot(after_path, after);
    if (rc != 0) { fprintf(stderr, "Compare: cannot read snapshot\n"); free(before); free(after); return rc; }

    for (i = 0U; i < before->count; ++i) {
        int j = find_key(after, before->records[i].key);
        if (j < 0) removed++;
        else if (strcmp(before->records[i].value, after->records[j].value) != 0) modified++;
        else unchanged++;
    }
    for (i = 0U; i < after->count; ++i)
        if (find_key(before, after->records[i].key) < 0) added++;

    if (kv) {
        printf("tool=Compare\n");
        printf("schema=amiforensics.compare.kv/1\n");
        printf("before=%s\n", before_path);
        printf("after=%s\n", after_path);
        for (i = 0U; i < before->count; ++i) {
            int j = find_key(after, before->records[i].key);
            if (j < 0)
                printf("change.removed.%u=%s\n", i, before->records[i].key);
            else if (strcmp(before->records[i].value, after->records[j].value) != 0) {
                printf("change.modified.%u.key=%s\n", i, before->records[i].key);
                printf("change.modified.%u.before=%s\n", i, before->records[i].value);
                printf("change.modified.%u.after=%s\n", i, after->records[j].value);
            }
        }
        for (i = 0U; i < after->count; ++i)
            if (find_key(before, after->records[i].key) < 0)
                printf("change.added.%u=%s\n", i, after->records[i].key);
        printf("added=%u\nremoved=%u\nmodified=%u\nunchanged=%u\n", added, removed, modified, unchanged);
        printf("truncated=%s\n", (before->truncated || after->truncated) ? "true" : "false");
    } else {
        printf("AmiForensics Compare\nBefore: %s\nAfter: %s\n", before_path, after_path);
        for (i = 0U; i < before->count; ++i) {
            int j = find_key(after, before->records[i].key);
            if (j < 0) printf("- %s=%s\n", before->records[i].key, before->records[i].value);
            else if (strcmp(before->records[i].value, after->records[j].value) != 0)
                printf("~ %s: %s -> %s\n", before->records[i].key, before->records[i].value, after->records[j].value);
        }
        for (i = 0U; i < after->count; ++i)
            if (find_key(before, after->records[i].key) < 0)
                printf("+ %s=%s\n", after->records[i].key, after->records[i].value);
        printf("Summary: +%u -%u ~%u =%u\n", added, removed, modified, unchanged);
    }

    rc = (before->truncated || after->truncated) ? 5 : 0;
    free(before);
    free(after);
    return rc;
}
