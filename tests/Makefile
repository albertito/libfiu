
CFLAGS += -std=c99 -pedantic -Wall
ALL_CFLAGS = -I../libfiu/ -L../libfiu/ \
	-D_XOPEN_SOURCE=600 -D_GNU_SOURCE -fPIC -DFIU_ENABLE=1 $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
	NICE_RUN = @echo "  RUN $<"; LD_LIBRARY_PATH=../libfiu/
	NICE_PY = @echo "  PY  $<"; ./wrap-python 2
	NICE_LN = @echo "  LN $@"; ln -f
else
	NICE_CC = $(CC)
	NICE_RUN = LD_LIBRARY_PATH=../libfiu/
	NICE_PY = ./wrap-python 2
	NICE_LN = ln -f
endif

default: tests

all: tests

tests: c-tests py-tests gen-tests utils-tests collisions-tests


# Link the libraries to a single place, some of the tests need this.
libs:
	mkdir -p libs/

libs/fiu_posix_preload.so: ../preload/posix/fiu_posix_preload.so libs
	$(NICE_LN) $< libs/

libs/fiu_run_preload.so: ../preload/run/fiu_run_preload.so libs
	$(NICE_LN) $< libs/

lnlibs: libs/fiu_posix_preload.so libs/fiu_run_preload.so


#
# C tests
#

C_SRCS := $(wildcard test-*.c)
C_OBJS := $(patsubst %.c,%.o,$(C_SRCS))
C_BINS := $(patsubst %.c,%,$(C_SRCS))

c-tests: $(patsubst %.c,c-run-%,$(C_SRCS))

test-%: test-%.c build-flags
	$(NICE_CC) $(ALL_CFLAGS) $< -lfiu -lpthread -o $@

# Tests that use the stack need special build flags, as some common
# optimizations can cause them to fail.
# -rdynamic: Adds all symbols to the dynamic symbol table. This option is
#    needed for backtrace() to work properly.
# -fno-optimize-sibling-calls: This optimization can turn some calls into
#    direct jumps, which leaves caller information out of the stack frame and
#    makes functions not appear in the backtrace. We disable it.
test-enable_stac%: test-enable_stac%.c build-flags
	$(NICE_CC) $(ALL_CFLAGS) \
		-rdynamic -fno-optimize-sibling-calls $< -lfiu -lpthread -o $@

c-run-%: %
	$(NICE_RUN) ./$<


BF = $(ALL_CFLAGS) ~ $(PREFIX)
build-flags: .force-build-flags
	@if [ x"$(BF)" != x"`cat build-flags 2>/dev/null`" ]; then \
		if [ -f build-flags ]; then \
			echo "build flags changed, rebuilding"; \
		fi; \
		echo "$(BF)" > build-flags; \
	fi


#
# Python tests
#

PY_TESTS := $(wildcard test-*.py)

py-tests: $(patsubst %.py,py-run-%,$(PY_TESTS))

py-run-%: %.py lnlibs small-cat
	$(NICE_PY) ./$<

small-cat: small-cat.c
	$(NICE_CC) $(ALL_CFLAGS) $< -o $@

#
# Sub-directory tests
#

gen-tests:
	$(MAKE) -C generated

utils-tests:
	$(MAKE) -C utils

collisions-tests:
	$(MAKE) -C collisions

#
# Cleanup
#

# Normally, $C_OBJS and $C_BINS are removed by make after building,
# since here they're considered "intermediate files"; however we
# also remove them when cleaning just in case.
clean:
	rm -f $(C_OBJS) $(C_BINS)
	rm -rf libs/ small-cat
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out build-flags
	$(MAKE) -C generated clean

FORCE:

.PHONY: default all clean \
	tests c-tests py-tests gen-tests utils-tests \
	.force-build-flags


