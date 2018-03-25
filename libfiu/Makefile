
CFLAGS += -std=c99 -pedantic -Wall
ALL_CFLAGS = -D_XOPEN_SOURCE=600 -fPIC -DFIU_ENABLE=1 $(CFLAGS)

# Optimization CFLAGS, only used when doing a non-debug build.
OPT_CFLAGS = -O3

ifdef DEBUG
    ALL_CFLAGS += -g
else
   ifdef PROFILE
        ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
   else
       ALL_CFLAGS += $(OPT_CFLAGS)
   endif
endif

ifdef TRACE
    ALL_CFLAGS += -DFIU_TRACE=1
endif

# prefix for installing the binaries
PREFIX=/usr/local

# prefix for eventual location of binaries
DESTDIR=$(PREFIX)

# install utility, we assume it's GNU/BSD compatible
INSTALL=install


OBJS = fiu.o fiu-rc.o backtrace.o wtable.o hash.o


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
else
	NICE_CC = $(CC)
endif

LIB_VER=0.96
LIB_SO_VER=0


# We attempt to detect if we have the extensions we use in backtrace.c by just
# attempting to compile it normally, and fall back to the dummy version if
# that fails. Not very sophisticated but should be safe.
USE_DUMMY_BACKTRACE := $(shell $(CC) $(ALL_CFLAGS) -c backtrace.c \
	-o backtrace-check.o || echo -DDUMMY_BACKTRACE)
ifndef USE_DUMMY_BACKTRACE
# The real backtrace needs linking against libdl.
USE_LIBDL = -ldl
endif


default: all

all: libs libfiu.pc

libfiu.pc: build-flags libfiu.pc.in
	@echo "generating libfiu.pc"
	@cat libfiu.pc.in | \
		sed 's@++DESTDIR++@$(DESTDIR)@g' | \
		sed 's@++LIB_VER++@$(LIB_VER)@g' \
		> libfiu.pc

libs: libfiu.so libfiu.a

libfiu.so: build-flags symbols.map fiu.h $(OBJS)
	$(NICE_CC) $(ALL_CFLAGS) -shared -fPIC \
		-Wl,-soname,libfiu.so.$(LIB_SO_VER) \
		-Wl,--version-script=symbols.map \
		$(OBJS) -lpthread $(USE_LIBDL) -o libfiu.so.$(LIB_VER)
	ln -fs libfiu.so.$(LIB_VER) libfiu.so
	ln -fs libfiu.so.$(LIB_VER) libfiu.so.$(LIB_SO_VER)

libfiu.a: build-flags fiu.h $(OBJS)
	$(AR) cr libfiu.a $(OBJS)


install-lib: libs libfiu.pc
	$(INSTALL) -d $(PREFIX)/lib
	$(INSTALL) -m 0755 libfiu.so.$(LIB_VER) $(PREFIX)/lib
	ln -fs libfiu.so.$(LIB_VER) $(PREFIX)/lib/libfiu.so
	ln -fs libfiu.so.$(LIB_VER) $(PREFIX)/lib/libfiu.so.$(LIB_SO_VER)
	$(INSTALL) -m 0755 libfiu.a $(PREFIX)/lib
	$(INSTALL) -d $(PREFIX)/include
	$(INSTALL) -m 0644 fiu.h $(PREFIX)/include
	$(INSTALL) -m 0644 fiu-control.h $(PREFIX)/include
	$(INSTALL) -m 0644 fiu-local.h $(PREFIX)/include
	$(INSTALL) -d $(PREFIX)/lib/pkgconfig
	$(INSTALL) -m 644 libfiu.pc $(PREFIX)/lib/pkgconfig
	@echo
	@echo "Please run ldconfig to update your library cache"
	@echo

install-man:
	$(INSTALL) -d $(PREFIX)/share/man/man3
	$(INSTALL) -m 0644 libfiu.3 $(PREFIX)/share/man/man3/

install: install-lib install-man

uninstall-lib:
	$(RM) $(PREFIX)/lib/libfiu.so
	$(RM) $(PREFIX)/lib/libfiu.so.$(LIB_VER)
	$(RM) $(PREFIX)/lib/libfiu.so.$(LIB_SO_VER)
	$(RM) $(PREFIX)/lib/libfiu.a
	$(RM) $(PREFIX)/include/fiu.h
	$(RM) $(PREFIX)/include/fiu-control.h
	$(RM) $(PREFIX)/include/fiu-local.h
	$(RM) $(PREFIX)/lib/pkgconfig/libfiu.pc

uninstall-man:
	$(RM) $(PREFIX)/share/man/man3/libfiu.3

uninstall: uninstall-lib uninstall-man


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

backtrace.o: ALL_CFLAGS += $(USE_DUMMY_BACKTRACE)


doxygen:
	        $(MAKE) LIB_VER=$(LIB_VER) -C doxygen


clean:
	rm -f libfiu.pc $(OBJS) backtrace-check.o
	rm -f libfiu.so libfiu.so.$(LIB_VER) libfiu.so.$(LIB_SO_VER) libfiu.a
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out build-flags
	$(MAKE) -C doxygen $@

.PHONY: default all libs \
	install-lib install-man install \
	uninstall-lib uninstall-man uninstall \
	doxygen clean \
	.force-build-flags


