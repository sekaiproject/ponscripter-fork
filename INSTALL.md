# Building and Installing

## Prerequisites

Building Ponscripter requires the following as a bare minimum:

  - A Unix-type environment with Bourne shell and standard utilities
  - GNU make
  - Suitable C and C++ compilers (see Compilers below)

For Microsoft Windows, this means you need MinGW and MSYS. The following steps should work:

1. Install [MinGW](http://sourceforge.net/projects/mingw/files/Installer/mingw-get-setup.exe/download).
2. Select the following during setup: `mingw32-libz, mingw32-autoconf, mingw32-automake, mingw32-gcc-g++, msys-core`
3. Run `pi.bat` (default location: `C:\MinGW\msys\1.0\postinstall\pi.bat`)
4. Start a shell (default: `C:\MinGW\msys\1.0\msys.bat`) and use it to configure and build.

Building the documentation (which is optional) requires two further
tools:

  - perl
  - xmlto

The following dependencies are included, but can optionally be dynamically
linked against if you have them installed on your system. If you do not provide them, the
included versions will be statically linked against.

  - [SDL2](http://www.libsdl.org/download-2.0.php)
  - [SDL2_image](https://www.libsdl.org/projects/SDL_image/), with at least PNG, JPEG, and BMP support
  - [SDL2_mixer](http://www.libsdl.org/projects/SDL_mixer/), with at least Ogg Vorbis support
  - [smpeg2](https://icculus.org/smpeg/) (ours was aquired from [here](http://dev.gentoo.org/~hasufell/distfiles/smpeg-2.0.0.tar.bz2))
  - bzip2
  - Freetype

If you would like steam support, than you should download the [Steamworks SDK](https://partner.steamgames.com) (last tested with v1.29) and move it into `src/extlib/steam-api` such that the folder `src/extlib/steam-api/public` exists. 

## Compiler

Compilation is only extensively tested with the GNU Compiler
Collection.  Some success has been achieved with compatible compilers
such as Intel's.

The Sun Workshop compilers are known NOT to work.

Microsoft's C++ compiler will not work with the Ponscripter build
system, but should theoretically be capable of building the program.

On OSX, clang has worked.


# Building

Building Ponscripter is done in the usual way:
```
./configure
make
```
On Unix-like platforms you will then want to run

`make install`

with suitable privileges; this will install the binary and manpages.

If you are building a specific game, such as Narcissu Side 2nd, there
might be a game environment variable to set. For narcissu, it is:

`export GAME=narci2`
  
## Building with Steam on Linux

The Steam runtime is only supported on Debian based distros, primarily Ubuntu. 

The following instructions should correctly build a binary for distribution with steam which links against Steam's provided SDL2 libraries. It assumes you have downloaded the code to your home directory and have followed the Steam prerequisites above.
```
wget http://media.steampowered.com/client/runtime/steam-runtime-sdk_latest.tar.xz
tar xvf steam-runtime-sdk_latest.tar.xz
cd steam-runtime-sdk*

bash setup.sh # all defaults
bash setup.sh # always failed once and then worked for me.
./shell-i386.sh
cd ~/ponscripter-fork

export CC=gcc
export CXX=g++

run.sh ./configure --with-external-sdl-mixer --steam
run.sh make
```
The resulting binary can be tested with `run.sh ./src/ponscr /path/to/0.utf`. You should ensure the `steam_api.so` file is alongside the binary and `steam_appid.txt` is present.

## Building with Steam on Windows

On windows, just running `./configure --steam` in addition to the otherwise normal build is sufficient.

## OS X Fat Binaries

To build a fat binary on Mac OS X, ignore the above instructions, and
instead run

  `util/osx_build.sh`

This will create three files (ponscr.ppc, ponscr.intel, and ponscr)
under src.  The third is your fat binary.


## Build Options

Run

`./configure --help`

for a summary of available options.

The two most likely to be useful are `--prefix` (if e.g. you want to
install to your home directory rather than `/usr/local`) and
`--with-internal-libs` (if you want to use only bundled libraries, not
anything your OS provides).

Reasons for using the latter include building a binary that will work
on multiple Linux distros, or working round library incompatibilities.

# Common Problems

## Linking Errors

The linking stage of the build sometimes fails with unresolved
symbols.

This is normally SDL's fault &mdash; `sdl-config` often fails to include
libraries that SDL is actually depending on.  For example, if ESD is
installed on Ubuntu, SDL will happily link against it without
mentioning the fact.

The workaround is to run the linking step manually (copy and paste the
failing command from the error output), and add the necessary options
yourself (in the above case, simply tacking on `-lesd` is all that's
required).

