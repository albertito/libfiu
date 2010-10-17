
CFLAGS += -std=c99 -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=600 -fPIC -DFIU_ENABLE=1 \
		-I. -I../../libfiu/ $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

# prefix for installing the binaries
PREFIX=/usr/local

# location of preload libraries
PLIBPATH=$(PREFIX)/lib

# install utility, we assume it's GNU/BSD compatible
INSTALL=install


OBJS = run.o


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
else
	NICE_CC = $(CC)
endif


default: all
	
all: fiu_run_preload.so fiu-run

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


# some platforms do not have libdl, we only use it if available
build-needlibdl:
	@$(LD) -ldl -o dlcheck.so 2>/dev/null \
		&& echo -ldl > $@ || echo > $@
	@rm -f dlcheck.so


fiu_run_preload.so: build-flags build-needlibdl $(OBJS)
	$(NICE_CC) $(ALL_CFLAGS) -shared -fPIC $(OBJS) \
		-L../../libfiu/ \
		-lfiu `cat build-needlibdl` \
		-o fiu_run_preload.so


fiu-run: build-flags fiu-run.in
	cat fiu-run.in | sed "s+@@PLIBPATH@@+$(PLIBPATH)+g" > fiu-run
	chmod +x fiu-run

install: fiu_run_preload.so fiu-run
	$(INSTALL) -d $(PREFIX)/lib
	$(INSTALL) -m 0755 fiu_run_preload.so $(PREFIX)/lib
	$(INSTALL) -d $(PREFIX)/bin
	$(INSTALL) -m 0755 fiu-run $(PREFIX)/bin
	$(INSTALL) -d $(PREFIX)/share/man/man1
	$(INSTALL) -m 0644 fiu-run.1 $(PREFIX)/share/man/man1/

uninstall:
	$(RM) $(PREFIX)/lib/fiu_run_preload.so
	$(RM) $(PREFIX)/bin/fiu-run
	$(RM) $(PREFIX)/share/man/man1/fiu-run.1

clean:
	rm -f $(OBJS) fiu_run_preload.so fiu-run build-flags
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default install uninstall clean .force-build-flags


