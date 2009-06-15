
default: all

all: posix

install: posix_install

clean: posix_clean


posix:
	$(MAKE) -C posix/

posix_clean:
	$(MAKE) -C posix/ clean

posix_install:
	$(MAKE) -C posix/ install


.PHONY: default clean install posix posix_clean posix_install

