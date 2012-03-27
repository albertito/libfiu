
CFLAGS += -std=c99 -pedantic -Wall -rdynamic
ALL_CFLAGS = -I../libfiu/ -L../libfiu/ \
	-D_XOPEN_SOURCE=600 -D_GNU_SOURCE -fPIC -DFIU_ENABLE=1 $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif


OBJS = test-1.o test-2.o


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
	NICE_RUN = @echo "  RUN $<"; LD_LIBRARY_PATH=../libfiu/
else
	NICE_CC = $(CC)
	NICE_RUN = LD_LIBRARY_PATH=../libfiu/
endif

default: tests

all: tests

tests: run-test-1 run-test-2

test-%: test-%.o
	$(NICE_CC) $(ALL_CFLAGS) $< -lfiu -o $@

run-%: % FORCE
	$(NICE_RUN) ./$<


BF = $(ALL_CFLAGS) ~ $(PREFIX)
build-flags: .force-build-flags
	@if [ x"$(BF)" != x"`cat build-flags 2>/dev/null`" ]; then \
		if [ -f build-flags ]; then \
			echo "build flags changed, rebuilding"; \
		fi; \
		echo "$(BF)" > build-flags; \
	fi

$(OBJS): build-flags

.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) test1
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out build-flags

FORCE:

.PHONY: default all clean \
	.force-build-flags


