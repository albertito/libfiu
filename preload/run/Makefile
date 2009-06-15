
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


OBJS = run.o


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
else
	NICE_CC = $(CC)
endif


default: all
	
all: fiu_run_preload.so fiu-run

.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@

fiu_run_preload.so: $(OBJS)
	$(NICE_CC) $(ALL_CFLAGS) -shared -fPIC $(OBJS) -lfiu -ldl \
		-o fiu_run_preload.so

fiu-run: fiu-run.in
	cat fiu-run.in | sed "s+@@PREFIX@@+$(PREFIX)+g" > fiu-run
	chmod +x fiu-run

install: fiu_run_preload.so fiu-run
	install -d $(PREFIX)/lib
	install -m 0755 fiu_run_preload.so $(PREFIX)/lib
	install -d $(PREFIX)/bin
	install -m 0755 fiu-run $(PREFIX)/bin
	install -d $(PREFIX)/man/man1
	install -m 0644 fiu-run.1 $(PREFIX)/man/man1/

clean:
	rm -f $(OBJS) fiu_run_preload.so fiu-run
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default install clean


