
CFLAGS += -std=c99 -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 -fPIC -DFIU_ENABLE=1 \
		-I. -I../../libfiu/ -L../../libfiu/ $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif


GEN_C = $(addsuffix .c,$(wildcard modules/*.mod))
GEN_OBJS = $(addsuffix .o,$(wildcard modules/*.mod))
CUSTOM_OBJS = $(patsubst %.c,%.o,$(wildcard modules/*.custom.c))
OBJS = codegen.o $(GEN_OBJS) $(CUSTOM_OBJS)


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
	NICE_GEN = @echo "  GEN $@"; ./generate
else
	NICE_CC = $(CC)
	NICE_GEN = ./generate
endif


default: all
	
all: fiu_posix_preload.so

$(GEN_OBJS): $(GEN_C)

%.mod.c: %.mod
	$(NICE_GEN) $< $@

.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@

fiu_posix_preload.so: $(OBJS)
	$(NICE_CC) $(ALL_CFLAGS) -shared -fPIC $(OBJS) -lfiu -ldl \
		-o fiu_posix_preload.so

clean:
	rm -f $(OBJS) $(GEN_OBJS:.o=.c) fiu_posix_preload.so
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default clean


