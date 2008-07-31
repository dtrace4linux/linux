#! /usr/bin/perl

# $Header:$

# Simple script to load the driver and get it ready.

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;

my $SUDO = "setuid root";

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts = (
	here => 0
	);

sub main
{
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'fast',
		'help',
		'here',
		);

	usage() if ($opts{help});
	if (! -f "$ENV{HOME}/bin/setuid" ) {
		$SUDO = "sudo";
	}
	print "Syncing...\n";
	system("sync ; sync");
	if ( -e "/dev/dtrace" ) {
		print "rmmod dtracedrv\n";
		system("$SUDO rmmod dtracedrv");
		#sleep 1
		system("sync ; sync");
	}

	#####################################################################
	#   Here  goes...we could crash the kernel... We sleep for a bit at
	#   present  since we need the /dev entries to appear. Chmod 666 so
	#   we can avoid being root all the time whilst we debug.
	#####################################################################
	print "Loading: dtracedrv.ko\n";
	spawn("$SUDO insmod dtracedrv.ko dtrace_here=$opts{here}");
	sleep(1);

	mkdev("/dev/dtrace");
	mkdev("/dev/fbt");
	
	###############################################
	#   Read  symbols  in  /proc/kallsyms into a  #
	#   hash				      #
	###############################################
	my $fh = new FileHandle("/proc/kallsyms");
	my %syms;
	while (<$fh>) {
		chomp;
		my $s = (split(" ", $_))[2];
		$syms{$s} = $_;
	}
	###############################################
	#   Need  to  'export'  GPL  symbols  to the  #
	#   driver  so  we can call these functions.  #
	#   Until  this  is  done,  the  driver is a  #
	#   little unsafe(!) Items below labelled as  #
	#   'optional'  are  allowed to be missing -  #
	#   we   only  ever  wanted  them  for  some  #
	#   initial  debugging,  and  dont need them  #
	#   any more.				      #
	#   					      #
	#   Colon  separated entries let us fallback  #
	#   to  an  alternate mechanism to find what  #
	#   we are after (see fbt_linux.c)	      #
	###############################################
	print "Preparing symbols...\n";
	my $err = 0;
	foreach my $s (qw/
		__symbol_get
		access_process_vm
		get_symbol_offset
		hrtimer_cancel
		hrtimer_init
		hrtimer_start
		kallsyms_addresses:optional
		kallsyms_expand_symbol
		kallsyms_lookup_name
		kallsyms_num_syms:optional
		kallsyms_op:optional
		kernel_text_address
		modules:print_modules
		sys_call_table:syscall_call
		/) {
		my $done = 0;
		foreach my $rawsym (split(":", $s)) {
			if ($rawsym eq 'optional') {
				$done = 1;
				last;
			}
			next if !defined($syms{$rawsym});
			my $fh = new FileHandle(">/dev/fbt");
			print $fh $syms{$rawsym} . "\n";
			$done = 1;
			last;
		}
		if (! $done) {
			print "ERROR: This kernel is missing: '$s'\n";
			$err++;
		}
	}
	if ( "$err" != 0 ) {
		print "Please do not run dtrace - your kernel is not supported.\n";
		exit(1);
	}

	#####################################################################
	#   Keep  a  copy of probes to avoid polluting /var/log/messages as
	#   we debug the driver..
	#####################################################################
	if (!$opts{fast}) {
		print "Probes available: ";
		my $pname =strftime("/tmp/probes-%Y%m%d-%H:%M", localtime);
		if ( -f "/tmp/probes.current" && ! -f $pname ) {
			if (!rename("/tmp/probes.current", "/tmp/probes.prev")) {
				print "rename error /tmp/probes.current -- $!\n";
			}
		}
		system("../../build/dtrace -l | tee $pname | wc -l ");
		unlink("/tmp/probes.current");
		if (!symlink($pname, "/tmp/probes.current")) {
			print "symlink($pname, /tmp/probes.current) error -- $!\n";
		}
	}
}
#####################################################################
#   If  the  /dev entries are not there then maybe we are on an old
#   kernel,   or  distro  (my  old  non-RAMfs  Knoppix  distro  for
#   instance).
#####################################################################
sub mkdev
{	my $dev = shift;

	my $name = basename($dev);
	if ( ! -e "/dev/$name" && -e "/sys/class/misc/$name/dev" ) {
		my $fh = new FileHandle("/sys/class/misc/$name/dev");
		my $dev = <$fh>;
		chomp($dev);
		$dev =~ s/:/ /;
		system("$SUDO mknod /dev/$name c $dev");
	}
	system("$SUDO chmod 666 /dev/$name");
}
sub spawn
{	my $cmd = shift;

	print $cmd, "\n";
	system($cmd);
}

#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print "load.pl: load dtrace driver and prime the links to the kernel.\n";
	print "Usage: load.pl [-here] ]-fast]\n";
	print "\n";
	print "Switches:\n";
	print "\n";
	print "   -fast   Dont do 'dtrace -l' after load to avoid kernel messages\n";
	print "   -here   Enable tracing to /var/log/messages\n";
	print "\n";
	exit(1);
}

main();
0;


