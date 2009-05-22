
all: default

default: libfiu utils

libfiu:
	$(MAKE) -C libfiu

install:
	$(MAKE) -C libfiu install


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


preload:
	$(MAKE) -C preload

preload_clean:
	$(MAKE) -C preload clean

clean: python_clean
	$(MAKE) -C libfiu clean


.PHONY: default all clean libfiu utils \
	python2 python2_install python3 python3_install python_clean \
	preload preload_clean


