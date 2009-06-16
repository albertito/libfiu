
# prefix for installing the binaries
PREFIX=/usr/local


default:
	
install:
	install -d $(PREFIX)/bin
	install -m 0755 fiu-ctrl $(PREFIX)/bin
	install -d $(PREFIX)/man/man1
	install -m 0644 fiu-ctrl.1 $(PREFIX)/man/man1/

clean:

.PHONY: default install clean


