#!/bin/sh
# finish up the installation
# this script should be executed using the sudo command
# this file is copied to smpeg-devel.post_install and smpeg-devel.post_upgrade
# inside the .pkg bundle
echo "Running post-install script"
umask 022

USER=basename ~
echo "User is \"$USER\""

ROOT=/Developer/Documentation/SDL

echo "Moving smpeg.framework to ~/Library/Frameworks"
# move smpeg to its proper home, so the target stationary works
sudo -u $USER mkdir -p ~/Library/Frameworks
sudo -u $USER /Developer/Tools/CpMac -r $ROOT/smpeg.framework ~/Library/Frameworks
rm -rf $ROOT/smpeg.framework

echo "Precompiling Header"
# precompile header for speedier compiles
sudo -u $USER /usr/bin/cc -precomp ~/Library/Frameworks/smpeg.framework/Headers/smpeg.h -o ~/Library/Frameworks/smpeg.framework/Headers/smpeg.p

# echo "Installing Stationary"
# echo "Installing Stationary"
# echo "Installing Stationary"
# # move stationary to its proper home
# mkdir -p "/Developer/ProjectBuilder Extras/Project Templates/Application"
# mkdir -p "/Developer/ProjectBuilder Extras/Target Templates/smpeg"

# cp -r "$ROOT/Project Stationary/smpeg Application" "/Developer/ProjectBuilder Extras/Project Templates/Application/"
# cp "$ROOT/Project Stationary/Application.trgttmpl" "/Developer/ProjectBuilder Extras/Target Templates/smpeg/"

# rm -rf "$ROOT/Project Stationary"

echo "Installing Man Pages"
# remove old man pages
#rm -rf "/Developer/Documentation/ManPages/man1/smpeg"*

# install man pages
#mkdir -p "/Developer/Documentation/ManPages/man1"
#cp "$ROOT/doc/"*".1" "/Developer/Documentation/ManPages/man1/"
#mkdir -p "/Developer/Documentation/smpeg"
#rm -rf "$ROOT/doc/"*".1"

#echo "Rebuilding Apropos Database"
# rebuild apropos database
#/usr/libexec/makewhatis

# # copy README file to your home directory
# sudo -u $USER cp "$ROOT/Readme smpeg Developer.txt" ~/

# # open up the README file
# sudo -u $USER open ~/"Readme smpeg Developer.txt"
