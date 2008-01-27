#! /bin/sh

# A noddy script to automate production of universal binaries on OS X.

make distclean &>/dev/null

export LDFLAGS="-Wl,-syslibroot,/Developer/SDKs/MacOSX10.3.9.sdk"
export CC="gcc -arch ppc -isysroot /Developer/SDKs/MacOSX10.3.9.sdk"
export CXX="g++ -arch ppc -isysroot /Developer/SDKs/MacOSX10.3.9.sdk"
./configure --with-internal-libs
make || exit
mv ponscr ponscr.ppc
make distclean &>/dev/null

export LDFLAGS="-Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk"
export CC="gcc -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk"
export CXX="g++ -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk"
./configure --with-internal-libs
make || exit
mv ponscr ponscr.intel

lipo -create \
     -arch ppc ponscr.ppc \
     -arch i386 ponscr.intel \
     -output ponscr
