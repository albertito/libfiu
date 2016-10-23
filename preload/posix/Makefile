
CFLAGS += -std=c99 -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=600 -fPIC -DFIU_ENABLE=1 \
		-D_LARGEFILE64_SOURCE=1 -I. -I../../libfiu/ \
		$(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

ifdef POSIX_TRACE
    ALL_CFLAGS += -DFIU_POSIX_TRACE=1
endif


# prefix for installing the binaries
PREFIX=/usr/local

# install utility, we assume it's GNU/BSD compatible
INSTALL=install


MODS = $(sort $(wildcard modules/*.mod))
GEN_C = $(addsuffix .c,$(MODS))
GEN_OBJS = $(addsuffix .o,$(MODS))
GEN_FL = $(addsuffix .fl,$(MODS))
CUSTOM_OBJS = $(patsubst %.c,%.o,$(sort $(wildcard modules/*.custom.c)))
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

BF = $(ALL_CFLAGS) ~ $(PREFIX)
build-flags: .force-build-flags
	@if [ x"$(BF)" != x"`cat build-flags 2>/dev/null`" ]; then \
		if [ -f build-flags ]; then \
			echo "build flags changed, rebuilding"; \
		fi; \
		echo "$(BF)" > build-flags; \
	fi

$(GEN_OBJS): $(GEN_C)

$(OBJS): build-flags codegen.h

%.mod.c: %.mod
	$(NICE_GEN) $< $@ $<.fl

.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@

# We define _GNU_SOURCE to get RTLD_NEXT if available; on non-GNU
# platforms it should be harmless.
codegen.o: codegen.c build-flags build-env.h
	$(NICE_CC) $(ALL_CFLAGS) -D_GNU_SOURCE -c $< -o $@

# some platforms do not have libdl, we only use it if available
build-needlibdl:
	@$(LD) -ldl -o dlcheck.so 2>/dev/null \
		&& echo -ldl > $@ || echo > $@
	@rm -f dlcheck.so

# libc's soname depends on the platform (most use libc.so.6, but for example
# ia64 and alpha use libc.so.6.1), so find which one to use at build-time.
# Please note that the argument to ldd *must* start with "./", otherwise some
# "ldd"s won't work (for example, the one in FreeBSD 8.1).
build-libcsoname:
	@$(CC) -x c /dev/null -lc -shared -o build-libccheck.so
	@ldd ./build-libccheck.so | grep libc.so | awk '{ print $$1 }' > $@
	@rm build-libccheck.so
	@test "`cat $@`" != "" || \
		(echo "Error finding soname, please report"; rm $@; exit 1)

build-env.h: build-env.h.in build-libcsoname
	@echo "  GEN $@"
	$(Q) sed "s+@@LIBC_SONAME@@+`cat build-libcsoname`+g" build-env.h.in \
		> build-env.h


fiu_posix_preload.so: build-flags build-env.h build-needlibdl $(OBJS)
	$(NICE_CC) $(ALL_CFLAGS) -shared -fPIC $(OBJS) \
		-L../../libfiu/ \
		-lfiu `cat build-needlibdl` \
		-o fiu_posix_preload.so


# this should only be needed when building the function list and not the
# preload library
%.mod.fl: %.mod
	$(NICE_GEN) $< $<.c $@

function_list: $(GEN_FL) function_list.in
	@echo "  function_list"
	$(Q) cp function_list.in function_list
	$(Q) for i in $(GEN_FL); do cat $$i >> function_list; done

install: fiu_posix_preload.so
	$(INSTALL) -d $(PREFIX)/lib
	$(INSTALL) -m 0755 fiu_posix_preload.so $(PREFIX)/lib

uninstall:
	$(RM) $(PREFIX)/lib/fiu_posix_preload.so

clean:
	rm -f $(OBJS) $(GEN_OBJS:.o=.c) $(GEN_FL)
	rm -f build-flags build-env.h build-libcsoname build-needlibdl
	rm -f function_list fiu_posix_preload.so
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out
	rm -f modules/*.bb modules/*.bbg modules/*.da
	rm -f modules/*.gcov modules/*.gcda modules/*.gcno modules/gmon.out

.PHONY: default install uninstall clean .force-build-flags


