
ifneq ($(V), 1)
        NICE_DOXYGEN = @echo "  DOXYGEN  $@"; doxygen
else
        NICE_DOXYGEN = doxygen
endif


default: all

all: public internal

# $(LIB_VER) must be defined externally if we want the generated docs to
# specify a version number. Usually, this Makefile will be invoked by
# libfiu's, which has that variable properly defined.
Doxyfile.base: Doxyfile.base.in
	@echo "generating Doxyfile.base"
	@cat Doxyfile.base.in | \
		sed 's@++VERSION++@$(LIB_VER)@g' \
		> Doxyfile.base

public: Doxyfile.base
	$(NICE_DOXYGEN) Doxyfile.public

internal: Doxyfile.base
	$(NICE_DOXYGEN) Doxyfile.internal

clean:
	rm -rf doc.internal doc.public Doxyfile.base


.PHONY: all clean default doxygen internal public

