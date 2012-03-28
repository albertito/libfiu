
CFLAGS += -std=c99 -pedantic -Wall -rdynamic
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
	NICE_PY_RUN = @echo "  PY  $<"; \
		      LD_LIBRARY_PATH=../libfiu/ \
		      PYTHONPATH=../bindings/python/
else
	NICE_CC = $(CC)
	NICE_RUN = LD_LIBRARY_PATH=../libfiu/
	NICE_PY_RUN = LD_LIBRARY_PATH=../libfiu/ \
		      PYTHONPATH=../bindings/python/
endif

default: tests

all: tests

tests: c-tests py-tests

#
# C tests
#

C_SRCS := $(wildcard test-*.c)
C_OBJS := $(patsubst %.c,%.o,$(C_SRCS))
C_BINS := $(patsubst %.c,%,$(C_SRCS))

c-tests: $(patsubst %.c,c-run-%,$(C_SRCS))

test-%: test-%.o build-flags
	$(NICE_CC) $(ALL_CFLAGS) $< -lfiu -o $@

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

.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@


#
# Python tests
#

PY_TESTS := $(wildcard test-*.py)

py-tests: $(patsubst %.py,py-run-%,$(PY_TESTS))

py-run-%: %.py
	$(NICE_PY_RUN) python ./$<


clean:
	# Normally, $C_OBJS and $C_BINS are removed by make after building,
	# since here they're considered "intermediate files"; however we
	# also remove them when cleaning just in case.
	rm -f $(C_OBJS) $(C_BINS)
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out build-flags

FORCE:

.PHONY: default all clean \
	tests c-tests py-tests \
	.force-build-flags


