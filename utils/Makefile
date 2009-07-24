
# prefix for installing the binaries
PREFIX=/usr/local

# install utility, we assume it's GNU/BSD compatible
INSTALL=install


default:
	
install:
	$(INSTALL) -d $(PREFIX)/bin
	$(INSTALL) -m 0755 fiu-ctrl $(PREFIX)/bin
	$(INSTALL) -m 0755 fiu-ls $(PREFIX)/bin
	$(INSTALL) -d $(PREFIX)/share/man/man1
	$(INSTALL) -m 0644 fiu-ctrl.1 $(PREFIX)/share/man/man1/
	$(INSTALL) -m 0644 fiu-ls.1 $(PREFIX)/share/man/man1/

clean:

.PHONY: default install clean


