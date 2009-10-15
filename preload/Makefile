
default: all

all: posix run

install: posix_install run_install

uninstall: posix_uninstall run_uninstall

clean: posix_clean run_clean


posix:
	$(MAKE) -C posix/

posix_clean:
	$(MAKE) -C posix/ clean

posix_install:
	$(MAKE) -C posix/ install

posix_uninstall:
	$(MAKE) -C posix/ uninstall

run:
	$(MAKE) -C run/

run_clean:
	$(MAKE) -C run/ clean

run_install:
	$(MAKE) -C run/ install

run_uninstall:
	$(MAKE) -C run/ uninstall


.PHONY: default clean install uninstall\
	posix posix_clean posix_install posix_uninstall \
	run run_clean run_install run_uninstall

