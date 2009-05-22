
all: default

default: libfiu utils

libfiu:
	$(MAKE) -C libfiu

install:
	$(MAKE) -C libfiu install


python2:
	cd bindings/python2 && python setup.py build

python2_install:
	cd bindings/python2 && python setup.py install

python2_clean:
	cd bindings/python2 && rm -rf build/

python3:
	cd bindings/python3 && python3 setup.py build

python3_install:
	cd bindings/python3 && python3 setup.py install

python3_clean:
	cd bindings/python3 && rm -rf build/


preload:
	$(MAKE) -C preload

preload_clean:
	$(MAKE) -C preload clean

clean: python2_clean python3_clean
	$(MAKE) -C libfiu clean


.PHONY: default all clean libfiu utils \
	python2 python2_install python2_clean \
	python3 python3_install python3_clean \
	preload preload_clean


