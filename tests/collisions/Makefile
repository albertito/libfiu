
CFLAGS += -std=c99 -pedantic -Wall
ALL_CFLAGS = -I../../libfiu/ -L../../libfiu/ -L./ \
	-D_XOPEN_SOURCE=600 -D_GNU_SOURCE -fPIC -DFIU_ENABLE=1 $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
	NICE_RUN = @echo "  RUN $<"; \
		   LD_LIBRARY_PATH="./:../../libfiu/"
	NICE_LN = @echo "  LN $@"; ln -f
else
	NICE_CC = $(CC)
	NICE_RUN = LD_LIBRARY_PATH="./:../../libfiu/"
	NICE_LN = ln -f
endif

default: tests

all: tests


BF = $(ALL_CFLAGS) ~ $(PREFIX)
build-flags: .force-build-flags
	@if [ x"$(BF)" != x"`cat build-flags 2>/dev/null`" ]; then \
		if [ -f build-flags ]; then \
			echo "build flags changed, rebuilding"; \
		fi; \
		echo "$(BF)" > build-flags; \
	fi

#
# Test library and binaries.
#

libcoltest.so: libcoltest.c libcoltest.h
	$(NICE_CC) $(ALL_CFLAGS) -shared -fPIC $< -o $@

binary: binary.c libcoltest.so
	$(NICE_CC) $(ALL_CFLAGS) -L. $< -lfiu -lcoltest -o $@


tests: binary
	$(NICE_RUN) ./binary

#
# Cleanup
#

clean:
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out build-flags
	rm -f libcoltest.so binary

FORCE:

.PHONY: default all clean \
	tests c-tests py-tests \
	.force-build-flags


