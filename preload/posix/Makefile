
CFLAGS += -std=c99 -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 -fPIC -DFIU_ENABLE=1 \
		-I. -I../../libfiu/ -L../../libfiu/ $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

# prefix for installing the binaries
PREFIX=/usr/local

# install utility, we assume it's GNU/BSD compatible
INSTALL=install


MODS = $(wildcard modules/*.mod)
GEN_C = $(addsuffix .c,$(MODS))
GEN_OBJS = $(addsuffix .o,$(MODS))
GEN_FL = $(addsuffix .fl,$(MODS))
CUSTOM_OBJS = $(patsubst %.c,%.o,$(wildcard modules/*.custom.c))
OBJS = codegen.o $(GEN_OBJS) $(CUSTOM_OBJS)


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
	NICE_GEN = @echo "  GEN $@"; ./generate
	Q = @
else
	NICE_CC = $(CC)
	NICE_GEN = ./generate
	Q =
endif


default: all
	
all: fiu_posix_preload.so function_list

$(GEN_OBJS): $(GEN_C)

%.mod.c: %.mod
	$(NICE_GEN) $< $@ $<.fl

.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@

fiu_posix_preload.so: $(OBJS)
	$(NICE_CC) $(ALL_CFLAGS) -shared -fPIC $(OBJS) -lfiu -ldl \
		-o fiu_posix_preload.so

# this should only be needed when building the function list and not the
# preload library
%.mod.fl: %.mod
	$(NICE_GEN) $< $<.c $@

function_list: $(GEN_FL) function_list.skel
	@echo "  function_list"
	$(Q) cp function_list.skel function_list
	$(Q) for i in $(GEN_FL); do cat $$i >> function_list; done

install: fiu_posix_preload.so
	$(INSTALL) -d $(PREFIX)/lib
	$(INSTALL) -m 0755 fiu_posix_preload.so $(PREFIX)/lib

clean:
	rm -f $(OBJS) $(GEN_OBJS:.o=.c) $(GEN_FL)
	rm -f function_list fiu_posix_preload.so
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default install clean


