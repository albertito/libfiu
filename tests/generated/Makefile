
CFLAGS += -std=c99 -pedantic -Wall
ALL_CFLAGS = -I../../libfiu/ -L../../libfiu/ \
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
		   LD_LIBRARY_PATH=../../libfiu/ \
		   LD_PRELOAD="../../preload/run/fiu_run_preload.so \
		   	../../preload/posix/fiu_posix_preload.so"
	NICE_GEN = @echo "  GEN $@"; ./generate-test
else
	NICE_CC = $(CC)
	NICE_RUN = LD_LIBRARY_PATH=../../libfiu/ \
		   LD_PRELOAD="../../preload/run/fiu_run_preload.so \
		   	../../preload/posix/fiu_posix_preload.so"
	NICE_GEN = ./generate-test
endif

default: tests

all: tests


CONF := $(wildcard tests/*.conf)
GEN_BIN := $(patsubst %.conf,%.bin,$(CONF))

tests: $(patsubst %,%-run,$(GEN_BIN))

%-run: %
	$(NICE_RUN) ./$<

# .bin from .c
%.bin: %.c build-flags
	$(NICE_CC) $(ALL_CFLAGS) $< -lfiu -o $@

# .c from .conf
%.c: %.conf generate-test
	$(NICE_GEN) -c $< -o $@

# Useful for manually generating the C-files, that otherwise get deleted as
# intermediates.
c-files: $(patsubst %.conf,%.c,$(CONF))


BF = $(ALL_CFLAGS) ~ $(PREFIX)
build-flags: .force-build-flags
	@if [ x"$(BF)" != x"`cat build-flags 2>/dev/null`" ]; then \
		if [ -f build-flags ]; then \
			echo "build flags changed, rebuilding"; \
		fi; \
		echo "$(BF)" > build-flags; \
	fi


#
# Cleanup
#

clean:
	rm -f $(GEN_BIN) $(patsubst %.bin,%.c,$(GEN_BIN))
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out build-flags

FORCE:

.PHONY: default all clean \
	tests c-tests py-tests \
	.force-build-flags


