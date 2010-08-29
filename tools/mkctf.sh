#! /bin/sh
#
# This script compiles the file driver/ctf_struct.c in the same way as
# the kernel makefile does, but its not a part of the kernel driver - 
# there is no code. It is compiled in debug (-g) mode, to ensure we have
# a full symbol table of kernel structures (just add appropriate #include's)
# so we can generate a CTF version of the DWARF debug symbols, so dtrace D
# scripts can utilise those features which require access to struct/union
# type data.
#

if [ ! -f build/ctfconvert ]; then
	exit 0
fi

BUILD_KERNEL=${BUILD_KERNEL:-`uname -r`}
build_dir=/lib/modules/$BUILD_KERNEL/build
pwd=`pwd`
cd $build_dir
cmd=`grep '^cmd_' $pwd/build/driver/.cpu_x86.o.cmd | 
sed -e 's/^.* := //' |
sed -e 's/-Wp,-MD[^ ]* /-g /' |
sed -e 's/\\\\//g' |
sed -e 's/cpu_x86/ctf_struct/g' `
#echo $cmd
eval $cmd
cd $pwd
build/ctfconvert -L label -o build/linux-$BUILD_KERNEL.ctf build/driver/.tmp_ctf_struct.o
ls -l build/linux-$BUILD_KERNEL.ctf
