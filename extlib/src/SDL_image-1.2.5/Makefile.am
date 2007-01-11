# Makefile.am for the SDL sample image loading library and viewer

lib_LTLIBRARIES = libSDL_image.la

libSDL_imageincludedir = $(includedir)/SDL
libSDL_imageinclude_HEADERS =	\
	SDL_image.h

libSDL_image_la_SOURCES =		\
	IMG.c			\
	IMG_bmp.c		\
	IMG_gif.c		\
	IMG_jpg.c		\
	IMG_lbm.c		\
	IMG_pcx.c		\
	IMG_png.c		\
	IMG_pnm.c		\
	IMG_tga.c		\
	IMG_tif.c		\
	IMG_xcf.c		\
	IMG_xpm.c		\
	IMG_xv.c

EXTRA_DIST =			\
	CHANGES			\
	gcc-fat.sh		\
	autogen.sh

libSDL_image_la_LDFLAGS = 	\
	-no-undefined		\
	-release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libSDL_image_la_LIBADD = $(IMG_LIBS)
