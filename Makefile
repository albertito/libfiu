
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


clean: python2_clean
	$(MAKE) -C libfiu clean


.PHONY: default all clean libfiu utils \
	python2 python2_install python2_clean


