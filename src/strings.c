#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_MIN_LEN 4
#define MAX_STRING_LEN 1024

static int contains_ci(const char *haystack, const char *needle)
{
    size_t i, j;
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);

    if (nlen == 0 || hlen < nlen)
        return 0;

    for (i = 0; i + nlen <= hlen; ++i) {
        for (j = 0; j < nlen; ++j) {
            unsigned char a = (unsigned char)haystack[i + j];
            unsigned char b = (unsigned char)needle[j];
            if (tolower(a) != tolower(b))
                break;
        }
        if (j == nlen)
            return 1;
    }
    return 0;
}

static const char *classify_indicator(const char *s)
{
    if (contains_ci(s, ".library"))
        return "library";
    if (contains_ci(s, ".device"))
        return "device";
    if (contains_ci(s, "rexxmast") || contains_ci(s, "arexx"))
        return "arexx";
    if (contains_ci(s, "http://") || contains_ci(s, "https://") ||
        contains_ci(s, "ftp://") || contains_ci(s, "telnet://"))
        return "network";
    if (contains_ci(s, "exec.library") || contains_ci(s, "dos.library") ||
        contains_ci(s, "trackdisk.device"))
        return "system";
    if (strchr(s, ':') != NULL)
        return "amiga-path-or-port";
    return "text";
}

static void emit_string(unsigned long offset, const char *s, int indicators_only)
{
    const char *kind = classify_indicator(s);

    if (indicators_only && strcmp(kind, "text") == 0)
        return;

    printf("%08lX  %-18s  %s\n", offset, kind, s);
}

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [-n minimum] [--indicators] <file>\n",
            prog);
}

int main(int argc, char **argv)
{
    FILE *fp;
    const char *path = NULL;
    char current[MAX_STRING_LEN + 1];
    size_t len = 0;
    unsigned long offset = 0;
    unsigned long start_offset = 0;
    int minimum = DEFAULT_MIN_LEN;
    int indicators_only = 0;
    int ch;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-n") == 0) {
            if (++i >= argc) {
                usage(argv[0]);
                return 10;
            }
            minimum = atoi(argv[i]);
            if (minimum < 1 || minimum > MAX_STRING_LEN) {
                fprintf(stderr, "Strings: invalid minimum length\n");
                return 10;
            }
        } else if (strcmp(argv[i], "--indicators") == 0) {
            indicators_only = 1;
        } else if (path == NULL) {
            path = argv[i];
        } else {
            usage(argv[0]);
            return 10;
        }
    }

    if (path == NULL) {
        usage(argv[0]);
        return 10;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Strings: cannot open '%s'\n", path);
        return 20;
    }

    while ((ch = fgetc(fp)) != EOF) {
        unsigned char c = (unsigned char)ch;
        int printable = (c >= 32 && c <= 126);

        if (printable) {
            if (len == 0)
                start_offset = offset;
            if (len < MAX_STRING_LEN)
                current[len++] = (char)c;
        } else {
            if ((int)len >= minimum) {
                current[len] = '\0';
                emit_string(start_offset, current, indicators_only);
            }
            len = 0;
        }
        ++offset;
    }

    if (ferror(fp)) {
        fprintf(stderr, "Strings: read error on '%s'\n", path);
        fclose(fp);
        return 20;
    }

    if ((int)len >= minimum) {
        current[len] = '\0';
        emit_string(start_offset, current, indicators_only);
    }

    fclose(fp);
    return 0;
}
