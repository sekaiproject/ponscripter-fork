
EXPORTS = Lua.x
HEADERS = \
	../lua.h \
	../lualib.h \
	../luadebug.h \
	../lauxlib.h

all: $(EXPORTS)

$(EXPORTS): $(HEADERS)
	perl gendef.pl $(HEADERS) >$@ || rm $@

clean:
	rm -f $(EXPORTS)
