#!/usr/bin/env bash
#
# Painless DTrace install for Ubuntu 8.04
#
# Author: philippe.hanrigou@gmail.com
# Date November 2008
#
# Dec 2011 Dont download out of date dtrace's. Dont try to build, either.

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
	case `uname -m` in
	  amd64)
	  	i386_pkgs="libc6-dev-i386"
		;;
	  *)
	  	###############################################
	  	#   Ubuntu  11.10/i386  is  very  boken.  Do  #
	  	#   this.				      #
	  	###############################################
		if [ ! -d /usr/include/sys -a -d /usr/include/i386-linux-gnu/sys ]; then
			ln -s /usr/include/i386-linux-gnu/sys /usr/include/sys
		fi
	  	;;
	esac

	sudo apt-get install openssh-server \
   		binutils-dev zlib1g-dev flex bison \
   		elfutils libdwarf-dev libelf-dev libc6-dev libc-dev \
		linux-libc-dev \
		$i386_pkgs
	###############################################
	#   Thanks  to Sunny Fugate for pointing out  #
	#   how   to  ensure  <gnu/stubs-32.h>  gets  #
	#   resolved  when  compiling the syscalls.c  #
	#   test.				      #
	###############################################
	sudo apt-get install g++-multilib
}

build()
{
   make -C ${DISTRIBUTION} clean all
}

#download
install_dependencies
#build

