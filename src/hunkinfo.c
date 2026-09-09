#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define HUNK_UNIT    999U
#define HUNK_NAME   1000U
#define HUNK_CODE   1001U
#define HUNK_DATA   1002U
#define HUNK_BSS    1003U
#define HUNK_RELOC32 1004U
#define HUNK_RELOC16 1005U
#define HUNK_RELOC8  1006U
#define HUNK_EXT    1007U
#define HUNK_SYMBOL 1008U
#define HUNK_DEBUG  1009U
#define HUNK_END    1010U
#define HUNK_HEADER 1011U

#define HUNK_TYPE_MASK 0x3FFFFFFFU
#define HUNK_SIZE_MASK 0x3FFFFFFFU

static int read_u32(FILE *fp, uint32_t *out)
{
    unsigned char b[4];
    if (fread(b, 1, 4, fp) != 4)
        return 0;
    *out = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8) | (uint32_t)b[3];
    return 1;
}

static int skip_longs(FILE *fp, uint32_t count)
{
    unsigned char buf[256];
    uint32_t bytes;
    size_t chunk;

    if (count > 0x3FFFFFFFU)
        return 0;
    bytes = count * 4U;
    while (bytes != 0U) {
        chunk = bytes > sizeof(buf) ? sizeof(buf) : (size_t)bytes;
        if (fread(buf, 1, chunk, fp) != chunk)
            return 0;
        bytes -= (uint32_t)chunk;
    }
    return 1;
}

static const char *hunk_name(uint32_t type)
{
    switch (type) {
    case HUNK_UNIT: return "UNIT";
    case HUNK_NAME: return "NAME";
    case HUNK_CODE: return "CODE";
    case HUNK_DATA: return "DATA";
    case HUNK_BSS: return "BSS";
    case HUNK_RELOC32: return "RELOC32";
    case HUNK_RELOC16: return "RELOC16";
    case HUNK_RELOC8: return "RELOC8";
    case HUNK_EXT: return "EXT";
    case HUNK_SYMBOL: return "SYMBOL";
    case HUNK_DEBUG: return "DEBUG";
    case HUNK_END: return "END";
    case HUNK_HEADER: return "HEADER";
    default: return "UNKNOWN";
    }
}

static int skip_string_longs(FILE *fp, uint32_t longs)
{
    return skip_longs(fp, longs);
}

static int parse_reloc(FILE *fp, unsigned long *entries)
{
    uint32_t count, target;
    for (;;) {
        if (!read_u32(fp, &count))
            return 0;
        if (count == 0U)
            return 1;
        if (!read_u32(fp, &target))
            return 0;
        (void)target;
        if (!skip_longs(fp, count))
            return 0;
        *entries += (unsigned long)count;
    }
}

static int parse_symbols(FILE *fp, unsigned long *symbols)
{
    uint32_t name_longs, value;
    for (;;) {
        if (!read_u32(fp, &name_longs))
            return 0;
        if (name_longs == 0U)
            return 1;
        if (!skip_string_longs(fp, name_longs) || !read_u32(fp, &value))
            return 0;
        (void)value;
        ++*symbols;
    }
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <file>\n", prog);
}

int main(int argc, char **argv)
{
    FILE *fp;
    uint32_t word, resident_longs, table_size, first_hunk, last_hunk;
    uint32_t i, raw_type, type, size_longs;
    unsigned long code_bytes = 0, data_bytes = 0, bss_bytes = 0;
    unsigned long reloc_entries = 0, symbols = 0, debug_hunks = 0;
    unsigned long stream_hunks = 0, segments = 0;

    if (argc != 2) {
        usage(argv[0]);
        return 10;
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        fprintf(stderr, "HunkInfo: cannot open '%s'\n", argv[1]);
        return 20;
    }

    if (!read_u32(fp, &word) || (word & HUNK_TYPE_MASK) != HUNK_HEADER) {
        fprintf(stderr, "HunkInfo: not a HUNK_HEADER executable\n");
        fclose(fp);
        return 5;
    }

    for (;;) {
        if (!read_u32(fp, &resident_longs))
            goto malformed;
        if (resident_longs == 0U)
            break;
        if (!skip_string_longs(fp, resident_longs))
            goto malformed;
    }

    if (!read_u32(fp, &table_size) || !read_u32(fp, &first_hunk) ||
        !read_u32(fp, &last_hunk))
        goto malformed;

    if (last_hunk < first_hunk || table_size == 0U ||
        (last_hunk - first_hunk + 1U) > table_size) {
        fprintf(stderr, "HunkInfo: invalid header hunk range/table\n");
        fclose(fp);
        return 20;
    }

    printf("File: %s\n", argv[1]);
    printf("Header: HUNK_HEADER\n");
    printf("Table size: %lu\n", (unsigned long)table_size);
    printf("First hunk: %lu\n", (unsigned long)first_hunk);
    printf("Last hunk: %lu\n", (unsigned long)last_hunk);
    printf("Declared segments: %lu\n", (unsigned long)(last_hunk - first_hunk + 1U));

    for (i = 0; i < table_size; ++i) {
        if (!read_u32(fp, &word))
            goto malformed;
        printf("HeaderSize[%lu]: %lu longs\n", (unsigned long)i,
               (unsigned long)(word & HUNK_SIZE_MASK));
    }

    while (read_u32(fp, &raw_type)) {
        type = raw_type & HUNK_TYPE_MASK;
        ++stream_hunks;
        printf("Hunk[%lu]: %s (%lu)\n", stream_hunks - 1UL,
               hunk_name(type), (unsigned long)type);

        switch (type) {
        case HUNK_CODE:
        case HUNK_DATA:
            if (!read_u32(fp, &size_longs))
                goto malformed;
            size_longs &= HUNK_SIZE_MASK;
            if (type == HUNK_CODE)
                code_bytes += (unsigned long)size_longs * 4UL;
            else
                data_bytes += (unsigned long)size_longs * 4UL;
            ++segments;
            if (!skip_longs(fp, size_longs))
                goto malformed;
            break;

        case HUNK_BSS:
            if (!read_u32(fp, &size_longs))
                goto malformed;
            size_longs &= HUNK_SIZE_MASK;
            bss_bytes += (unsigned long)size_longs * 4UL;
            ++segments;
            break;

        case HUNK_RELOC32:
            if (!parse_reloc(fp, &reloc_entries))
                goto malformed;
            break;

        case HUNK_SYMBOL:
            if (!parse_symbols(fp, &symbols))
                goto malformed;
            break;

        case HUNK_DEBUG:
        case HUNK_NAME:
        case HUNK_UNIT:
            if (!read_u32(fp, &size_longs) || !skip_longs(fp, size_longs))
                goto malformed;
            if (type == HUNK_DEBUG)
                ++debug_hunks;
            break;

        case HUNK_END:
            break;

        case HUNK_RELOC16:
        case HUNK_RELOC8:
        case HUNK_EXT:
        default:
            fprintf(stderr, "HunkInfo: unsupported hunk type %lu (%s); refusing to guess length\n",
                    (unsigned long)type, hunk_name(type));
            fclose(fp);
            return 5;
        }
    }

    if (ferror(fp))
        goto malformed;

    fclose(fp);
    printf("Segments seen: %lu\n", segments);
    printf("CODE bytes: %lu\n", code_bytes);
    printf("DATA bytes: %lu\n", data_bytes);
    printf("BSS bytes: %lu\n", bss_bytes);
    printf("RELOC32 entries: %lu\n", reloc_entries);
    printf("Symbols: %lu\n", symbols);
    printf("DEBUG hunks: %lu\n", debug_hunks);
    return 0;

malformed:
    fprintf(stderr, "HunkInfo: truncated or malformed HUNK stream\n");
    fclose(fp);
    return 20;
}
