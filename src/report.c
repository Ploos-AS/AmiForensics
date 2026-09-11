#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPORT_MAX_SOURCES 16U
#define REPORT_MAX_RECORDS 512U
#define REPORT_LINE_LEN 512U
#define REPORT_KEY_LEN 192U
#define REPORT_VALUE_LEN 256U
#define REPORT_PATH_LEN 256U

typedef struct {
    char key[REPORT_KEY_LEN];
    char value[REPORT_VALUE_LEN];
    unsigned int source;
} ReportRecord;

typedef struct {
    char path[REPORT_PATH_LEN];
    char schema[REPORT_VALUE_LEN];
    char tool[REPORT_VALUE_LEN];
    unsigned int records;
    unsigned int malformed;
    int truncated;
} ReportSource;

typedef struct {
    ReportSource sources[REPORT_MAX_SOURCES];
    ReportRecord records[REPORT_MAX_RECORDS];
    unsigned int source_count;
    unsigned int record_count;
    unsigned int warnings;
    int truncated;
} Report;

static int copy_field(char *dst, size_t size, const char *src)
{
    size_t n = strlen(src);
    if (n >= size) return 0;
    memcpy(dst, src, n + 1U);
    return 1;
}

static void trim_line(char *s)
{
    size_t n = strlen(s);
    while (n > 0U && (s[n - 1U] == '\n' || s[n - 1U] == '\r')) s[--n] = '\0';
}

static void discard_line_tail(FILE *fp)
{
    int ch;
    do { ch = fgetc(fp); } while (ch != '\n' && ch != EOF);
}

static int add_record(Report *r, unsigned int source, const char *key, const char *value)
{
    ReportRecord *rec;
    if (r->record_count >= REPORT_MAX_RECORDS) {
        r->truncated = 1;
        r->warnings++;
        return 0;
    }
    rec = &r->records[r->record_count];
    if (!copy_field(rec->key, sizeof(rec->key), key) ||
        !copy_field(rec->value, sizeof(rec->value), value)) {
        r->truncated = 1;
        r->warnings++;
        return 0;
    }
    rec->source = source;
    r->record_count++;
    r->sources[source].records++;
    return 1;
}

static int load_source(Report *r, const char *path)
{
    FILE *fp;
    ReportSource *src;
    unsigned int index;
    char line[REPORT_LINE_LEN];

    if (r->source_count >= REPORT_MAX_SOURCES) return 10;
    index = r->source_count;
    src = &r->sources[index];
    memset(src, 0, sizeof(*src));
    if (!copy_field(src->path, sizeof(src->path), path)) return 10;

    fp = fopen(path, "r");
    if (fp == NULL) return 20;
    r->source_count++;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *eq;
        size_t n = strlen(line);
        int complete = (n > 0U && line[n - 1U] == '\n');
        if (!complete && !feof(fp)) {
            src->truncated = 1;
            r->truncated = 1;
            r->warnings++;
            discard_line_tail(fp);
            continue;
        }
        trim_line(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        eq = strchr(line, '=');
        if (eq == NULL || eq == line) {
            src->malformed++;
            r->warnings++;
            continue;
        }
        *eq++ = '\0';
        if (strcmp(line, "schema") == 0 && src->schema[0] == '\0') {
            if (!copy_field(src->schema, sizeof(src->schema), eq)) {
                src->truncated = 1; r->truncated = 1; r->warnings++;
            }
        }
        if (strcmp(line, "tool") == 0 && src->tool[0] == '\0') {
            if (!copy_field(src->tool, sizeof(src->tool), eq)) {
                src->truncated = 1; r->truncated = 1; r->warnings++;
            }
        }
        add_record(r, index, line, eq);
    }
    if (ferror(fp)) { fclose(fp); return 20; }
    fclose(fp);
    if (src->schema[0] == '\0') r->warnings++;
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "Usage: Report [--kv] <input.kv> [input.kv ...]\n");
}

int main(int argc, char **argv)
{
    Report *r;
    unsigned int i;
    int kv = 0;
    int first = 1;
    int rc = 0;

    if (argc >= 3 && strcmp(argv[1], "--kv") == 0) { kv = 1; first = 2; }
    if (argc <= first) { usage(); return 10; }
    if ((unsigned int)(argc - first) > REPORT_MAX_SOURCES) { usage(); return 10; }

    r = (Report *)malloc(sizeof(*r));
    if (r == NULL) return 20;
    memset(r, 0, sizeof(*r));

    for (i = (unsigned int)first; i < (unsigned int)argc; ++i) {
        rc = load_source(r, argv[i]);
        if (rc != 0) {
            fprintf(stderr, "Report: cannot load %s\n", argv[i]);
            free(r);
            return rc;
        }
    }

    if (kv) {
        printf("tool=Report\n");
        printf("schema=amiforensics.report.kv/1\n");
        printf("source.count=%u\n", r->source_count);
        printf("record.count=%u\n", r->record_count);
        printf("warning.count=%u\n", r->warnings);
        for (i = 0U; i < r->source_count; ++i) {
            const ReportSource *s = &r->sources[i];
            printf("source.%u.path=%s\n", i, s->path);
            printf("source.%u.tool=%s\n", i, s->tool[0] ? s->tool : "unknown");
            printf("source.%u.schema=%s\n", i, s->schema[0] ? s->schema : "unknown");
            printf("source.%u.records=%u\n", i, s->records);
            printf("source.%u.malformed=%u\n", i, s->malformed);
            printf("source.%u.truncated=%s\n", i, s->truncated ? "true" : "false");
        }
        for (i = 0U; i < r->record_count; ++i) {
            const ReportRecord *rec = &r->records[i];
            printf("record.%u.source=%u\n", i, rec->source);
            printf("record.%u.key=%s\n", i, rec->key);
            printf("record.%u.value=%s\n", i, rec->value);
        }
        printf("truncated=%s\n", r->truncated ? "true" : "false");
    } else {
        printf("AmiForensics Report\nSources: %u  Records: %u  Warnings: %u\n", r->source_count, r->record_count, r->warnings);
        for (i = 0U; i < r->source_count; ++i) {
            const ReportSource *s = &r->sources[i];
            printf("\n[%u] %s\n", i, s->path);
            printf("    Tool: %s\n", s->tool[0] ? s->tool : "unknown");
            printf("    Schema: %s\n", s->schema[0] ? s->schema : "unknown");
            printf("    Records: %u  Malformed: %u%s\n", s->records, s->malformed,
                   s->truncated ? "  TRUNCATED" : "");
        }
        if (r->truncated) printf("\nWARNING: report input exceeded bounded limits.\n");
    }

    rc = r->truncated ? 5 : 0;
    free(r);
    return rc;
}
