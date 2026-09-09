CC ?= m68k-amigaos-gcc
CFLAGS ?= -O2 -Wall -Wextra -m68000
LDFLAGS ?=

TARGETS := FileInfo Strings HunkInfo BootInfo

.PHONY: all clean check

all: $(TARGETS)

FileInfo: src/fileinfo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

Strings: src/strings.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

HunkInfo: src/hunkinfo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

BootInfo: src/bootinfo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

check:
	@grep -q 'HUNK_HEADER' src/fileinfo.c
	@grep -q 'crc32_update' src/fileinfo.c
	@grep -q 'classify_indicator' src/strings.c
	@grep -q 'trackdisk.device' src/strings.c
	@grep -q 'parse_reloc' src/hunkinfo.c
	@grep -q 'parse_symbols' src/hunkinfo.c
	@grep -q 'boot_checksum_sum' src/bootinfo.c
	@grep -q 'checksum_valid' src/bootinfo.c
	@echo "static checks: PASS"

clean:
	rm -f $(TARGETS)
