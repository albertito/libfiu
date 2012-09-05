

default: all

install: all_install

uninstall: all_uninstall

all: libfiu preload utils

all_install: libfiu_install preload_install utils_install

all_uninstall: libfiu_uninstall preload_uninstall utils_uninstall


libfiu:
	$(MAKE) -C libfiu

libfiu_clean:
	$(MAKE) -C libfiu clean

libfiu_install:
	$(MAKE) -C libfiu install

libfiu_uninstall:
	$(MAKE) -C libfiu uninstall


preload: libfiu
	$(MAKE) -C preload

preload_clean:
	$(MAKE) -C preload clean

preload_install: preload
	$(MAKE) -C preload install

preload_uninstall:
	$(MAKE) -C preload uninstall


utils:
	$(MAKE) -C utils

utils_clean:
	$(MAKE) -C utils clean

utils_install: utils
	$(MAKE) -C utils install

utils_uninstall:
	$(MAKE) -C utils uninstall


tests: test

test: libfiu bindings preload
	$(MAKE) -C tests

test_clean:
	$(MAKE) -C tests clean


bindings: python2 python3

bindings_install: python2_install python3_install

bindings_clean: python_clean

python2: libfiu
	cd bindings/python && python setup.py build

python2_install: python2
	cd bindings/python && python setup.py install

python3: libfiu
	cd bindings/python && python3 setup.py build

python3_install: python3
	cd bindings/python && python3 setup.py install

python_clean:
	cd bindings/python && rm -rf build/


clean: python_clean preload_clean libfiu_clean utils_clean test_clean


.PHONY: default all clean install all_install uninstall all_uninstall \
	libfiu libfiu_clean libfiu_install libfiu_uninstall \
	python2 python2_install python3 python3_install python_clean \
	bindings bindings_install bindings_clean \
	preload preload_clean preload_install preload_uninstall \
	utils utils_clean utils_install utils_uninstall \
	test tests test_clean


