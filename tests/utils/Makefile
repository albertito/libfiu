
ifneq ($(V), 1)
	NICE_RUN = @echo "  RUN $<"; LD_LIBRARY_PATH=../../libfiu/
	NICE_LN = @echo "  LN $@"; ln -f
else
	NICE_RUN = LD_LIBRARY_PATH=../../libfiu/
	NICE_LN = ln -f
endif


default: all

all: tests


# Link the libraries to a single place, as the scripts expect.
libs:
	mkdir -p libs/

libs/fiu_posix_preload.so: ../../preload/posix/fiu_posix_preload.so libs
	$(NICE_LN) $< libs/

libs/fiu_run_preload.so: ../../preload/run/fiu_run_preload.so libs
	$(NICE_LN) $< libs/

lnlibs: libs/fiu_posix_preload.so libs/fiu_run_preload.so


#
# Tests
#

SRCS := $(wildcard test-*)

tests: $(patsubst %,run-%,$(SRCS))

run-%: % lnlibs
	$(NICE_RUN) ./$< > output-$<.txt 2>&1


#
# Cleanup
#

clean:
	rm -f libs/*.so output-*.txt
	rmdir libs/

.PHONY: default all clean \
	tests lnlibs

