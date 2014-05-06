# -*- makefile-gmake -*-
#
# THIS IS A GENERATED FILE - changes will not be kept if configure is
# run again.  If you wish to customise it, please be sure to give your
# version a different filename.
#
# Makefile to place the steam-sdk for ponscripter

LIBFILE=linux32/libsteam_api.so

steam_headers = $(wildcard public/steam/*.h)
target_headers = $(addprefix ../../include/steam/,$(notdir $(steam_headers)))

steam_lib = ./redistributable_bin/$(LIBFILE)
target_lib = ../../lib/$(notdir $(LIBFILE))

.PHONY: all
all: $(target_headers) $(target_lib)

$(target_headers): ../../include/steam/%.h : public/steam/%.h
	@mkdir -p ../../include/steam
	cp $< $@

$(target_lib): % : $(steam_lib)
	cp $< $@
clean:
	rm -f $(target_headers) $(target_lib)
