# Build rules for internal libraries.
# Do not invoke this directly; these rules are used automatically when relevant.

REDIR=>/dev/null 2>&1 # comment out to show configure outputs

EL = extlib/lib
EI = extlib/include
EB = extlib/bin
ES = extlib/src

SDLSRC  = $(ES)/SDL-1.2.11
SDLISRC = $(ES)/SDL_image-1.2.5
ZSRC    = $(ES)/zlib-1.2.3
PNGSRC  = $(ES)/libpng-1.2.14
JPEGSRC = $(ES)/jpeg-6b
SDLMSRC = $(ES)/SDL_mixer-1.2.7
SMPGSRC = $(ES)/smpeg
BZSRC   = $(ES)/bzip2-1.0.3

LPREFIX = $(shell pwd)/extlib

CLEAN_TARGETS=clean_sdl clean_sdl_image clean_sdl_mixer clean_bzip2 clean_smpeg clean_freetype
DISTCLEAN_TARGETS=dclean_sdl dclean_sdl_image dclean_sdl_mixer

#-- SDL -----------------------------------------------------------------------

.PHONY: internal_sdl clean_sdl dclean_sdl
internal_sdl: $(EB)/sdl-config
clean_sdl:
	@if [ -f $(SDLSRC)/Makefile ]; then $(MAKE) -C $(SDLSRC) uninstall; fi
	@if [ -f $(SDLSRC)/Makefile ]; then $(MAKE) -C $(SDLSRC) clean; fi
dclean_sdl: clean_sdl
	@if [ -f $(SDLSRC)/Makefile ]; then $(MAKE) -C $(SDLSRC) distclean; fi

$(SDLSRC)/Makefile:
	@echo Configuring internal SDL...
	@cd $(SDLSRC) && \
	./configure --prefix="$(LPREFIX)" \
	    $(addprefix --disable-,shared joystick $(addprefix video-,fbcon directfb svga vgl wscons opengl)) \
	    $(REDIR)

$(EB)/sdl-config: $(SDLSRC)/Makefile
	@$(MAKE) -C $(SDLSRC)
	@$(MAKE) -C $(SDLSRC) install

#-- SDL_image -----------------------------------------------------------------

.PHONY: internal_sdl_image clean_sdl_image dclean_sdl_image
internal_sdl_image: $(EL)/libSDL_image$(LIBSUFFIX)
clean_sdl_image:
	@-if [ -f $(SDLISRC)/Makefile ]; then $(MAKE) -C $(SDLISRC) uninstall; fi
	@-if [ -f $(SDLISRC)/Makefile ]; then $(MAKE) -C $(SDLISRC) clean; fi
	@-if [ -f $(EL)/libz$(LIBSUFFIX) ]; then $(MAKE) -C $(ZSRC) uninstall prefix="$(LPREFIX)"; fi
	@-if [ -f $(ZSRC)/libz$(LIBSUFFIX) ]; then $(MAKE) -C $(ZSRC) clean; fi
	@-if [ -f $(PNGSRC)/Makefile ]; then $(MAKE) -C $(PNGSRC) uninstall; fi
	@-if [ -f $(PNGSRC)/Makefile ]; then $(MAKE) -C $(PNGSRC) clean; fi
	@-if [ -f $(JPEGSRC)/Makefile ]; then \
	    rm -f $(EL)/libjpeg$(LIBSUFFIX) $(addprefix $(EI)/,jconfig.h jpeglib.h jmorecfg.h jerror.h); \
	    $(MAKE) -C $(JPEGSRC) clean; \
	fi
dclean_sdl_image:
	@-if [ -f $(SDLISRC)/Makefile ]; then $(MAKE) -C $(SDLISRC) distclean; fi
	@-if [ -f $(PNGSRC)/Makefile ]; then $(MAKE) -C $(PNGSRC) distclean; fi
	@-if [ -f $(JPEGSRC)/Makefile ]; then $(MAKE) -C $(JPEGSRC) distclean; fi

$(SDLISRC)/Makefile: $(EB)/sdl-config $(if $(INTERNAL_LIBPNG),internal_png) $(if $(INTERNAL_LIBJPEG),internal_jpeg)
	@echo Configuring internal SDL_image...
	@cd $(SDLISRC) && \
	./configure --prefix="$(LPREFIX)" \
	    $(addprefix --disable-,shared lbm pcx pnm tga xcf xv) \
	    $(REDIR)

$(EL)/libSDL_image$(LIBSUFFIX): $(SDLISRC)/Makefile
	@$(MAKE) -C $(SDLISRC)
	@$(MAKE) -C $(SDLISRC) install

# First build libz, libpng, and libjpeg, if required.

.PHONY: internal_png internal_jpeg
internal_png: $(EL)/libpng$(LIBSUFFIX)
internal_jpeg: $(EL)/libjpeg$(LIBSUFFIX)

$(PNGSRC)/Makefile:
	@echo Configuring internal libpng...
	@cd $(PNGSRC) && \
	./configure --prefix="$(LPREFIX)" --disable-shared --disable-dependency-tracking $(REDIR)

$(EL)/libpng$(LIBSUFFIX): $(PNGSRC)/Makefile $(EL)/libz$(LIBSUFFIX)
	@$(MAKE) -C $(PNGSRC)
	@$(MAKE) -C $(PNGSRC) install

$(EL)/libz$(LIBSUFFIX):
	@$(MAKE) -C $(ZSRC) install prefix="$(LPREFIX)"

$(JPEGSRC)/Makefile:
	@echo Configuring internal libjpeg...
	cd $(JPEGSRC) && \
	./configure --prefix="$(LPREFIX)" --disable-shared --enable-static $(REDIR)

$(EL)/libjpeg$(LIBSUFFIX): $(JPEGSRC)/Makefile 
	@$(MAKE) -C $(JPEGSRC) install-lib

#-- SDL_mixer -----------------------------------------------------------------

.PHONY: internal_sdl_mixer clean_sdl_mixer dclean_sdl_mixer
internal_sdl_mixer: $(EL)/libSDL_mixer$(LIBSUFFIX)
clean_sdl_mixer:
	@if [ -f $(SDLMSRC)/Makefile ]; then $(MAKE) -C $(SDLMSRC) uninstall; fi
	@if [ -f $(SDLMSRC)/Makefile ]; then $(MAKE) -C $(SDLMSRC) clean; fi
dclean_sdl_mixer:
	@if [ -f $(SDLMSRC)/Makefile ]; then $(MAKE) -C $(SDLMSRC) distclean; fi
	@rm -f extlib/configured_mixer_libs

$(SDLMSRC)/Makefile: $(EB)/sdl-config $(if $(INTERNAL_SMPEG),$(EB)/smpeg-config) extlib/configured_mixer_libs
	@echo Configuring internal SDL_mixer...
	@cd $(SDLMSRC) && \
	./configure --prefix="$(LPREFIX)" --enable-music-native-midi-gpl \
	    $(addprefix --disable-,shared $(addprefix music-,cmd mod timidity-midi)) \
	    $(REDIR)

$(EL)/libSDL_mixer$(LIBSUFFIX): $(SDLMSRC)/Makefile
	@$(MAKE) -C $(SDLMSRC)
	@$(MAKE) -C $(SDLMSRC) install

# First build Ogg libs, if required.

extlib/configured_mixer_libs:
	$(SHELL) extlib/configure_mixer_libs $@

#-- SMPEG ---------------------------------------------------------------------

.PHONY: internal_smpeg clean_smpeg dclean_smpeg
internal_smpeg: $(EB)/smpeg-config
clean_smpeg:
	@-if [ -f $(SMPGSRC)/Makefile ]; then $(MAKE) -C $(SMPGSRC) uninstall; fi
	@-if [ -f $(SMPGSRC)/Makefile ]; then $(MAKE) -C $(SMPGSRC) clean; fi
dclean_smpeg: clean_smpeg
	@-if [ -f $(SMPGSRC)/Makefile ]; then $(MAKE) -C $(SMPGSRC) distclean; fi

$(SMPGSRC)/Makefile: $(EB)/sdl-config
	@echo Configuring internal smpeg...
	@cd $(SMPGSRC) && \
	./configure --prefix="$(LPREFIX)" \
	    $(addprefix --disable-,shared gtk-player opengl-player debug) \
	    $(REDIR)

$(EB)/smpeg-config: $(SMPGSRC)/Makefile
	@$(MAKE) -C $(SMPGSRC)
	@$(MAKE) -C $(SMPGSRC) install

#-- Freetype ------------------------------------------------------------------

.PHONY: internal_freetype clean_freetype
internal_freetype:
clean_freetype:

#-- libbzip2 ------------------------------------------------------------------

BZOBJS = blocksort huffman crctable randtable compress decompress bzlib

.PHONY: internal_bzip2 clean_bzip2
internal_bzip2: $(EL)/libbz2$(LIBSUFFIX) $(EI)/bzlib.h
clean_bzip2:
	rm -f $(EL)/libbz2$(LIBSUFFIX) $(EI)/bzlib.h $(BZSRC)/*$(OBJSUFFIX)

$(EL)/libbz2$(LIBSUFFIX): $(foreach fn,$(BZOBJS),$(BZSRC)/$(fn)$(OBJSUFFIX))
	rm -f $@
	$(AR) cq $@ $^
	@if [ -f "$(RANLIB)" -o -f /usr/bin/ranlib -o \
		-f /bin/ranlib -o -f /usr/ccs/bin/ranlib ]; then \
		$(if $(RANLIB),$(RANLIB),ranlib) $@ ; \
	fi

$(EI)/bzlib.h: $(BZSRC)/bzlib.h
	cp $< $@
