This directory contains a Perl script (with Makefile to run) that creates a 
file called smpeg.exp which is a list of all functions that need to be exported.

The script was shamelessly taken from the SDL Project Builder archives. The patten matching functions had to be changed to meet smpeg's API coding style. If you find missing functions in the build, you may need to modify the script or hand edit the smpeg.exp file.



