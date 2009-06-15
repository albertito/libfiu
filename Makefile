

default: libfiu

install: libfiu_install

all: libfiu preload

all_install: libfiu_install preload_install


libfiu:
	$(MAKE) -C libfiu

libfiu_clean:
	$(MAKE) -C libfiu clean

libfiu_install:
	$(MAKE) -C libfiu install



preload: libfiu
	$(MAKE) -C preload

preload_clean:
	$(MAKE) -C preload clean

preload_install: preload
	$(MAKE) -C preload install

python2:
	cd bindings/python && python setup.py build

python2_install:
	cd bindings/python && python setup.py install

python3:
	cd bindings/python && python3 setup.py build

python3_install:
	cd bindings/python && python3 setup.py install

python_clean:
	cd bindings/python && rm -rf build/


clean: python_clean preload_clean libfiu_clean


.PHONY: default all clean install all_install \
	libfiu libfiu_clean libfiu_install \
	python2 python2_install python3 python3_install python_clean \
	preload preload_clean preload_install


