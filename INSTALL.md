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

If you would like Steam support, than you should download the [Steamworks SDK](https://partner.steamgames.com) (last tested with v1.29) and move it into `src/extlib/src/steam-sdk` such that the folder `src/extlib/src/steam-sdk/public` exists. 

## Compiler

Compilation is only extensively tested with the GNU Compiler
Collection.  Some success has been achieved with compatible compilers
such as Intel's.

The Sun Workshop compilers are known NOT to work.

Microsoft's C++ compiler will not work with the Ponscripter build
system, but should theoretically be capable of building the program.

On OS X, clang has worked.


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

run.sh ./configure --with-external-sdl-mixer --steam
run.sh make
```
The resulting binary can be tested with `run.sh ./src/ponscr /path/to/0.utf`. You should ensure the `steam_api.so` file is alongside the binary and `steam_appid.txt` is present.

## Building with Steam on Windows

On windows, just running `./configure --steam` in addition to the otherwise normal build is sufficient.

## Building on OS X

Compiling with OS X is a bit more difficult than most else, but it is confirmed to build with [Clang](http://clang.llvm.org/), the new OS X default compiler.

This fork only builds with OS X 10.5+ support because of SDL2 and a few other things. Ponscripter does not yet see Clang as a legitimate compiler unfortunately, but with `--unsupported-compiler` it works fine.

OS X is best built with internal libs. Otherwise, you may run into issues with varying versions of operating systems and libraries.

Here are my standard `./configure` and `make` lines, successfully building on OS X 10.9:

```
./configure --unsupported-compiler --with-internal-libs
```
```
make
```

## Building an OS X App

Building a proper application on OS X is fairly easy. Simply make sure you're in the base directory and run this:

```
make osxapp
```

This will put a Ponscripter application in the root directory. If you drag this application into a directory containing Ponscripter game files, it will run that game!

But the question most people want answered is "How do I bundle my game for distribution".

1. Copy the file `src/Makefile.game` to your own file named `src/Makefile.mygame`.
2. Look at `Makefile.narci` as an example, and replace whatever you can in your new game-specific makefile with your own information.
3. Put your game's data (`0.utf`, `*.arc`, etc) in a folder called `gamedata` in the root directory. This will be automatically copied into the created app.
4. Provided you've filled out the game-specific Makefile thoroughly enough, you should be able to do something like this:

```
make osxapp GAME=mygame
```

Because it will be grabbing the required information from `Makefile.mygame`, it will automatically create your App in the base Ponscripter directory.

NOTE: To set a custom icon for your application, simply create a file called `src/resources/mygame.icns`, and it will be automatically applied at make time. Or put the custom icon in `gamedata/icon.icns`.

If you're building a Steam application, make sure you set `STEAM_APPID` in your makefile

Once built as above, you should be able to launch your game without issue!

## Building with Steam on OS X

As per above instructions, make sure the [Steamworks SDK](https://partner.steamgames.com) is in `src/extlib/src/steam-sdk`. Once this is done, simply adding the `--steam` flag to your `./configure` line should be enough, as shown:
```
./configure --unsupported-compiler --with-internal-libs --steam
```

As with Linux builds, you should ensure the `steam_api.dylib` file is alongside the binary and `steam_appid.txt` is present. Otherwise, it will crash when launching. (This is done automatically when building a .app bundle above)


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

## aclocal-1.13: command not found

You don't actually need aclocal, but because of the timestamps it thinks it
needs to run it. To fix this error, run
`src/extlib/src/SDL2_image-2.0.0/fix-timestamps.sh`

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

