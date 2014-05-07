#!/usr/bin/env bash
#
# Script to get packages for building.
#
# Author: philippe.hanrigou@gmail.com
# Date November 2008
#
# Dec 2011 Dont download out of date dtrace's. Dont try to build, either.
# 20120623 Dont load g++-multilib when on ARM.

DISTRIBUTION=dtrace-20081028

download()
{
   sudo apt-get install curl
   if [ ! -e ${DISTRIBUTION} ]; then
       echo "\n==== Downloading DTrace for Linux ====\n"
       curl -O
ftp://crisp.dyndns-server.com/pub/release/website/dtrace/${DISTRIBUTION}.tar.bz2
       tar jxvf ${DISTRIBUTION}.tar.bz2
   fi
}

install_dependencies()
{
	# fc14: libdwarf libdwarf-devel
	# ubuntu: linux-crashdump
	i386_pkgs=
	arch=`uname -m`
	case $arch in
	  amd64)
	  	i386_pkgs="libc6-dev-i386"
		;;
	  *)
	  	###############################################
	  	#   Ubuntu  13.10  is  very  broken. Do  #
	  	#   this.				      #
	  	###############################################
	  if [ ! -d /usr/include/sys -a -d /usr/include/x86_64-linux-gnu/sys ]; then
			ln -s /usr/include/x86_64-linux-gnu/sys /usr/include/sys
		fi
	  	;;
	esac
	  
	  	###############################################
	  	#   Ubuntu  11.10/i386  is  very  broken. Do  #
	  	#   this.				      #
	  	###############################################

		if [ ! -d /usr/include/sys -a -d /usr/include/i386-linux-gnu/sys ]; then
			ln -s /usr/include/i386-linux-gnu/sys /usr/include/sys
		fi
	  	;;
	esac

	sudo apt-get install "$@" flex bison make
	sudo apt-get install "$@" libelf-dev
	sudo apt-get install "$@" libc6-dev 
	sudo apt-get install "$@" openssh-server \
   		binutils-dev zlib1g-dev \
   		elfutils libdwarf-dev libiberty-dev libelf-dev libc-dev \
		zlib1g-dev \
		linux-libc-dev \
		linux-headers-$(uname -r) \
		$i386_pkgs
	###############################################
	#   Thanks  to Sunny Fugate for pointing out  #
	#   how   to  ensure  <gnu/stubs-32.h>  gets  #
	#   resolved  when  compiling the syscalls.c  #
	#   test.				      #
	###############################################
	case $arch in 
	  arm*)
	  	;;
	  *)
		sudo apt-get install "$@" g++-multilib
		;;
	esac
}

build()
{
   make -C ${DISTRIBUTION} clean all
}

#download
install_dependencies "$@"
#build

