Linux port of DTrace

Dec 2015
Paul D. Fox
paul.d.fox@gmail.com
http://www.twitter.com/crispeditor
http://www.crispeditor.co.uk

Blog - latest news and stuff about the dtrace project:

   http://crtags.blogspot.com/
   http://www.crispeditor.co.uk/blog/

Download dtrace tarballs for linux here:

   https://github.com/dtrace4linux/linux
   ftp://crispeditor.co.uk/pub/release/website/dtrace

Introduction
============

This is a port of the Sun DTrace user and kernel code to Linux.
No  linux  kernel  code  is  touched in this build, but what is
produced  is  a dynamically loadable kernel module. This avoids
licensing issues and allows people to load and update dtrace as
they desire.

The  goal  of  this project is to make available DTrace for the
Linux  platforms. By making it available for everyone, they can
use  it  to  optimise their systems and tools, and in return, I
get to benefit from their work.

PayPal
======
If  you want to make a donation for this software, feel free to
do so. Nothing is asked of you - it is genuinely free software,
but it can help guage interest and appreciation if you do.

You  can  pay  by  visiting  the  link  below  and  clicking on
"Donate", or use this reference for donations:

   * paul.d.fox@gmail.com
   * [Paypal](https://www.paypal.com/cgi-bin/webscr?business=paul.d.fox@gmail.com&cmd=_xclick&currency_code=GBP&amount=15&item_name=DTrace)

Licensing
=========

The  original  DTrace is licensed under Sun's (now Oracle) CDDL
license.  Original  copyrights  are left intact. No GPL code is
incorporated into the release, to avoid legal conflicts.

Any  mistakes  or omissions in copyright attribution will be my
mistake, so please let me know if there are such cases.

The  linux kernel was referred to in order to engineer the glue
for  dtrace  behavior, and there is no intention of making this
code fall under anything other than CDDL. (If Oracle migrate to
a GPL friendly license, then this port of dtrace can follow). I
do  not  own  the license or assert any rights on the licensing
other than that expected of me as a consumer/supplier.

I  have  no political affiliation or preference for a licensing
scheme,  simply  that  Sun/Oracle has gracefully donated to the
community a large body of work.

I  reserve  the  right to change the licensing model for my own
code  at  a later date, when and if someone puts forward a case
as to the correct license agreement.

If  the code is useful to you - great. Spread it around and get
people to use, debug and enhance it.

GIT Repository
==============

  https://github.com/dtrace4linux/linux

(Theres an older and orphaned github repository under 
Peter McCormicks name, please ignore this as it has not been
updated in a long while and is no longer active).

Installation
============

You may need to grab some extra packages for building DTrace.
Use either of the following to download extra packages. This
list may be incomplete depending on the version of your kernel/distro.

	$ tools/get-deps-arch.sh	# if using ArchLinux
	$ tools/get-deps.pl 		# if using Ubuntu
	$ tools/get-deps-fedora.sh	# RedHat/Fedora

	$ make all
	$ make install
	$ make load           (need to be root or have sudo access)

If the libdwarf package installed on the system is too old
it still compiles without any problem, but you will get
runtime errors from the io.d and/or sched.d files due to
undefined kernel structure definitions.

If you get a undefined struct definition such as dtrace_cpu_t
when running, please upgrade it.

Tested successfully with version 20100214 (whereas 20080409
is to old).

More details
============

Building is done in a build/ directory. The makefiles allow
you to compile for alternate kernel releases in the same tree,
which is useful for cross-version checking.

The result is:

	build/dtrace               User land executable
	build/drti.o               Object file for USDT apps
	build/driver/dtracedrv.ko  Kernel loadable module

Installing will copy them to Solaris compliant locations:

	/usr/sbin/dtrace
	/usr/lib/dtrace/64/drti.o

You dont need to 'install' to run dtrace, but you will need
to load the driver.

Kernel versioning
=================
dtrace relies on a kernel module and so a binary is needed
per system you deploy to, or kernel version.

dtrace is sensitive to the kernel - and attempts to cater for that,
but very old, or very new kernels may not have been validated.
Please feed back if that is the case.

A kernel strack trace is expected when loading the module, due to
currently unknown reasons (the current theory is that the kernel
ftrace mechanism which probes dtrace as its loaded gets confused
by what it sees).

If you get a stack trace something like this in the logs when
loading the module, this can safely be ignore (it appears to
be no harm (unless you use system-tap and dtrace at the same
time, then there could be a conflict):

    [  182.556392] dtracedrv: module license 'CDDL' taints kernel.
    [  182.556396] Disabling lock debugging due to kernel taint
    [  184.760136] CPU: 5 PID: 11008 Comm: dtrace Tainted: P           O 3.12.0+scst+tf.1 #5
    [  184.760140] Hardware name: To be filled by O.E.M. To be filled by O.E.M./SABERTOOTH 990FX, BIOS 0901 11/24/2011
    [  184.760142]  ffffffffa093c8a0 ffffffff813c18b9 ffff8800dacdaa80 ffffffffa08f28ed
    [  184.760146]  755f6f745f646162 ffff8800dacdaa80 ffff88040acbbb9c 0000000000000020
    [  184.760149]  0000000000000001 ffffffffa09370c0 ffff8800dd020a80 ffffffffa08e7f3e
    [  184.760151] Call Trace:
    [  184.760156]  [<ffffffff813c18b9>] ? dump_stack+0x41/0x58
    [  184.760165]  [<ffffffffa08f28ed>] ? mutex_enter_common+0x2d/0xeb [dtracedrv]
    [  184.760172]  [<ffffffffa08e7f3e>] ? par_alloc+0x20/0xd0 [dtracedrv]
    [  184.760178]  [<ffffffffa08f0894>] ? instr_provide_module+0x31/0x1f5 [dtracedrv]
    [  184.760184]  [<ffffffffa08f489c>] ? sdt_open+0x3/0x3 [dtracedrv]
    [  184.760189]  [<ffffffffa08f2980>] ? mutex_enter_common+0xc0/0xeb [dtracedrv]
    [  184.760195]  [<ffffffffa08d83d0>] ? dtrace_probe_provide+0xcd/0xf7 [dtracedrv]
    [  184.760201]  [<ffffffffa08d844f>] ? dtrace_open+0x55/0x10f [dtracedrv]
    [  184.760203]  [<ffffffff812b3e60>] ? kobj_lookup+0xfc/0x133
    [  184.760209]  [<ffffffffa08e7400>] ? dtracedrv_open+0x4c/0x51 [dtracedrv]
    [  184.760212]  [<ffffffff812a3973>] ? misc_open+0x107/0x168
    [  184.760216]  [<ffffffff8112c091>] ? chrdev_open+0x129/0x148
    [  184.760218]  [<ffffffff8112bf68>] ? cdev_put+0x1a/0x1a
    [  184.760220]  [<ffffffff81127082>] ? do_dentry_open+0x16c/0x22b
    [  184.760221]  [<ffffffff8112720f>] ? finish_open+0x2c/0x35
    [  184.760224]  [<ffffffff81134d9d>] ? do_last+0x9fd/0xc4a
    [  184.760226]  [<ffffffff81135249>] ? path_openat+0x25f/0x5bd
    [  184.760228]  [<ffffffff81141561>] ? mntput_no_expire+0x1b/0x16c
    [  184.760230]  [<ffffffff811356a2>] ? do_filp_open+0x2d/0x75
    [  184.760233]  [<ffffffff8111e32c>] ? kmem_cache_alloc+0x114/0x194
    [  184.760235]  [<ffffffff813c4d93>] ? _raw_spin_unlock+0x9/0xb
    [  184.760237]  [<ffffffff8113fa09>] ? __alloc_fd+0xfa/0x10c
    [  184.760239]  [<ffffffff81126cfa>] ? do_sys_open+0x146/0x1d6
    [  184.760241]  [<ffffffff813c7909>] ? ia32_do_call+0x13/0x13

No Linux Kernel source modifications required
=============================================

This is important for a number of reasons -- unless dtrace
is accepted into the kernel, it has to live with changes to header
files and data structures. Also, from a licensing perspective it
is not valid for dtrace to touch your sources. It is also much
easier to not even require kernel sources - so long as 
a kernel build environment is available.

INSTALLATION
============

Run `make` with no arguments to see the current options. You
may need to run one of the `tools/get-deps` scripts for your OS
flavor to ensure you have the tools and kernel build
environment for your kernel.

    make all
        to compile the drivers and user space commands. Check the file
        Packages, for hints on what you need (not much, but libelf, kernel
        source, flex/yacc -- bison will do).

    make install
        Copy dtrace binary and driver to correct install location.

    make load
        To load the drivers, and then you can play with cmd/dtrace/dtrace.

    make unl
        to unload the drivers.

    make test
        To run the userland cmd/dtrace regression test

To build the userland (command and object file etc) and the
kernel module for different architectures, set the environment
variable `BUILD_ARCH` appropriately and then use the make targets
separately.

This example is for building on a system with a 64-bit kernel,
but with 32-bit userland:

       BUILD_ARCH=i386 make cmds
       BUILD_ARCH=x86_64 make kernel

Dependencies
============
To build dtrace for linux requires a number of tools - mostly
the basic Unix development tools, plus you will need the kernel
source/build tree. dtrace does not affect or touch your kernel
sources, but it needs the normal header files for creating a 
loadable module.

Examine the following scripts to help identify missing packages:

	tools/get-deps-arch.sh
	tools/get-deps-fedora.sh
	tools/get-deps.pl

Internet scripts
================
Many scripts on the 'Net won't work since they tend to assume a
Solaris kernel, but if you look at them and read them to learn,
then they can mostly be adapted for Linux.
