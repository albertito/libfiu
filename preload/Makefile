
default: all

all: posix run

install: posix_install run_install

clean: posix_clean run_clean


posix:
	$(MAKE) -C posix/

posix_clean:
	$(MAKE) -C posix/ clean

posix_install:
	$(MAKE) -C posix/ install

run:
	$(MAKE) -C run/

run_clean:
	$(MAKE) -C run/ clean

run_install:
	$(MAKE) -C run/ install



.PHONY: default clean install \
	posix posix_clean posix_install \
	run run_clean run_install 

