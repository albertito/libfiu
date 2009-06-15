
CFLAGS += -std=c99 -pedantic -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 -fPIC -DFIU_ENABLE=1 $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif


# prefix for installing the binaries
PREFIX=/usr/local


OBJS = fiu.o fiu-rc.o


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
else
	NICE_CC = $(CC)
endif


default: all

all: libs libfiu.pc

libfiu.pc: libfiu.skel.pc
	@echo "generating libfiu.pc"
	@cat libfiu.skel.pc | \
		sed 's@++PREFIX++@$(PREFIX)@g' \
		> libfiu.pc

libs: libfiu.so libfiu.a

libfiu.so: fiu.h $(OBJS)
	$(NICE_CC) $(ALL_CFLAGS) -shared -fPIC $(OBJS) -lpthread -o libfiu.so

libfiu.a: fiu.h $(OBJS)
	$(AR) cr libfiu.a $(OBJS)


install-lib: libs libfiu.pc
	install -d $(PREFIX)/lib
	install -m 0755 libfiu.so $(PREFIX)/lib
	install -m 0755 libfiu.a $(PREFIX)/lib
	install -d $(PREFIX)/include
	install -m 0644 fiu.h $(PREFIX)/include
	install -m 0644 fiu-control.h $(PREFIX)/include
	install -m 0644 fiu-local.h $(PREFIX)/include
	install -d $(PREFIX)/lib/pkgconfig
	install -m 644 libfiu.pc $(PREFIX)/lib/pkgconfig
	@echo
	@echo "Please run ldconfig to update your library cache"
	@echo

install-man:
	install -d $(PREFIX)/man/man3
	install -m 0644 libfiu.3 $(PREFIX)/man/man3/

install: install-lib install-man


.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@

clean:
	rm -f libfiu.pc $(OBJS) libfiu.so libfiu.a
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default all libs install-lib install-man install clean


