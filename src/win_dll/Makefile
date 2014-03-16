#
# Makefile for installing the Mingw32 version of the SDL library

all:
	if test -d $(prefix); then \
	    mkdir $(prefix)/bin; \
	    cp -v bin/* $(prefix)/bin; \
	    cp -v SDL2.dll $(prefix)/bin; \
	    mkdir $(prefix)/lib; \
	    cp -v lib/* $(prefix)/lib; \
	    mkdir $(prefix)/include; \
	    mkdir $(prefix)/include/SDL2; \
	    cp -v include/SDL2/* $(prefix)/include/SDL2; \
	    sed "s|^prefix=.*|prefix=$(prefix)|" <bin/sdl2-config >$(prefix)/bin/sdl2-config; \
	    chmod 755 $(prefix)/bin/sdl2-config; \
	    sed "s|^libdir=.*|libdir=\'$(prefix)/lib\'|" <lib/libSDL2.la >$(prefix)/lib/libSDL2.la; \
	else \
	    echo "*** ERROR: $(prefix) does not exist!"; \
	    exit 1; \
	fi
