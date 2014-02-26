#!/bin/csh

###
## This script removes the Developer smpeg package
###

setenv HOME_DIR ~

sudo -v -p "Enter administrator password to remove smpeg: "

sudo rm -rf "$HOME_DIR/Library/Frameworks/smpeg.framework"

# will only remove the Frameworks dir if empty (since we put it there)
sudo rmdir "$HOME_DIR/Library/Frameworks"

#sudo rm -r "$HOME_DIR/Readme smpeg Developer.txt"
sudo rm -r "/Developer/Documentation/smpeg"
sudo rm -r "/Developer/Documentation/ManPages/man1/smpeg"*
#sudo rm -r "/Developer/ProjectBuilder Extras/Project Templates/Application/smpeg Application"
#sudo rm -r "/Developer/ProjectBuilder Extras/Target Templates/smpeg"
sudo rm -r "/Library/Receipts/smpeg-devel.pkg"

# rebuild apropos database
sudo /usr/libexec/makewhatis

unsetenv HOME_DIR



