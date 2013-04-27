#! /bin/sh

# My RaspberryPi/wheeze seemed to be missing key /usr/include dirs
# so patch them if we need to.

# 19-Feb-2013 PDF New file

if [ ! -d /usr/include/asm ]; then
	echo Creating /usr/include/asm
	ln -s arm-linux-gnueabihf/asm /usr/include/asm
fi
if [ ! -d /usr/include/bits ]; then
	echo Creating /usr/include/bits
	ln -s arm-linux-gnueabihf/bits /usr/include/bits
fi
if [ ! -d /usr/include/gnu ]; then
	echo Creating /usr/include/gnu
	ln -s arm-linux-gnueabihf/gnu /usr/include/gnu
fi

