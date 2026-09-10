CC = m68k-amigaos-gcc
CFLAGS ?= -O2 -Wall -Wextra -m68000
LDFLAGS ?=

TARGETS := FileInfo Strings HunkInfo BootInfo ResidentView PatchView TaskView ProcessView

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

ResidentView: src/residentview.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

PatchView: src/patchview.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

TaskView: src/taskview.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

ProcessView: src/processview.c
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
	@grep -q 'Forbid' src/residentview.c
	@grep -q 'ResModules' src/residentview.c
	@grep -q 'RTC_MATCHWORD' src/residentview.c
	@grep -q 'CreateArgstring' src/residentview.c
	@grep -q 'RXFF_RESULT' src/residentview.c
	@grep -q 'LIST RESIDENTS' src/residentview.c
	@grep -q '0x4EF9' src/patchview.c
	@grep -q 'OpenLibrary' src/patchview.c
	@grep -q 'OpenDevice' src/patchview.c
	@grep -q 'CreateIORequest' src/patchview.c
	@grep -q 'dd_Library' src/patchview.c
	@grep -q 'direct_jmp' src/patchview.c
	@grep -q 'TaskReady' src/taskview.c
	@grep -q 'TaskWait' src/taskview.c
	@grep -q 'FindTask' src/taskview.c
	@grep -q 'Forbid' src/taskview.c
	@grep -q 'NT_PROCESS' src/processview.c
	@grep -q 'TaskReady' src/processview.c
	@grep -q 'TaskWait' src/processview.c
	@grep -q 'Forbid' src/processview.c
	@echo "static checks: PASS"

clean:
	rm -f $(TARGETS)
