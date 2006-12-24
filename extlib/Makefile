# Build rules for internal libraries.
# Do not invoke this directly; these rules are used automatically when relevant.

.PHONY: clean_sdl
clean_sdl:

.PHONY: clean_sdl_image
clean_sdl_image:

.PHONY: clean_sdl_mixer
clean_sdl_mixer:

.PHONY: clean_smpeg
clean_smpeg:

.PHONY: clean_freetype
clean_freetype:

BZOBJS = blocksort huffman crctable randtable compress decompress bzlib
extlib/lib/libbz2$(LIBSUFFIX): $(foreach fn,$(BZOBJS),extlib/src/bzip2-1.0.3/$(fn)$(OBJSUFFIX))
	rm -f $@
	$(AR) cq $@ $^
	@if [ -f "$(RANLIB)" -o -f /usr/bin/ranlib -o \
		-f /bin/ranlib -o -f /usr/ccs/bin/ranlib ]; then \
		$(if $(RANLIB),$(RANLIB),ranlib) $@ ; \
	fi

.PHONY: clean_bzip2
clean_bzip2:
	rm -f extlib/lib/libbz2$(LIBSUFFIX) extlib/src/bzip2-1.0.3/*$(OBJSUFFIX)

