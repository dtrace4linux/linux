#! /usr/bin/perl

# $Header:$
# Script to compile dtrace against kernels on the system.

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;
use Cwd;

my $modpost;
my $asmlnk;
my $warn_app = "";

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts;

my $ctrl_c;

sub int_handler
{
	print "Ctrl-C typed..aborting\n";
	$ctrl_c = 1;
}
sub main
{
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'help',
		'no32',
		'no64',
		'v',
		);

	usage() if ($opts{help});

	$ENV{TOPDIR} = Cwd::cwd();

	###############################################
	#   Dont do a bug.sh when things fail.	      #
	###############################################
	$ENV{MAKE_KERNELS} = 1;

	my $kernel = $ARGV[0];
	$kernel = $ENV{KERNEL} if !$kernel;
	$warn_app = "warn" if -f "$ENV{HOME}/bin/warn";

	my $uname = `uname -m`;
	chomp($uname);

	$SIG{INT} = \&int_handler;

	for my $f (glob("/lib/modules/2*")) {
		exit(1) if $ctrl_c;
		my $dir = basename($f);

		next if $kernel && $dir ne $kernel;

		print "======= Building: $f ===============================\n";
		$ENV{BUILD_KERNEL} = $dir;
		if (!$opts{no64} && system("$warn_app make all")) {
			exit(1);
		}
		exit(1) if $ctrl_c;

		if (!$opts{no32} &&  $uname eq "x86_64") {
			build_i386($f);
		}
	}
}
sub build_i386
{	my $f = shift;

	my $dir = basename($f);
	print "======= Building: $f (i386) =================\n";
	###############################################
	#   Redirect  the include/asm symlink so its  #
	#   fine  with us, but we need to be careful  #
	#   to put it back, even on a ^C.	      #
	###############################################
	if (-d "$f/build/include/asm-i386") {
		$asmlnk = readlink("$f/build/include/asm");
		print "$f/build/include/asm -> $asmlnk\n";
		if (!rename("$f/build/include/asm", "$f/build/include/asm.sav")) {
			print "Error on rename: $f/build/include/asm - $!\n";
			exit(1);
		}
		if (!symlink("asm-i386", "$f/build/include/asm")) {
			print "symlink i386 -> $f/build/include/asm - error $!\n";
			exit(1);
		}
	}

	###############################################
	#   Replace  scripts/mod/modpost  because it  #
	#   wont know what to do.		      #
	###############################################
	$modpost = "$f/build/scripts/mod/modpost";
	if (!rename($modpost, "$modpost.sav")) {
		print "Error rename: $modpost -- $!\n";
	}
	my $fh = new FileHandle(">$modpost");
	print $fh "touch $ENV{TOPDIR}/build/driver/dtracedrv.mod.c\n";
	print $fh "exit 0\n";
	$fh->close();
	chmod(0755, $modpost);

	$ENV{BUILD_KERNEL} = "$dir-i386";
	$ENV{BUILD_ARCH} = "i386";
	my $ret = system("$warn_app make " . ($opts{v} ? "V=1" : "") . " all");

	restore_asmlnk($f);

	exit(1) if $ret;

	$modpost = "";
	delete($ENV{BUILD_ARCH});
}
sub restore_asmlnk
{	my $f = shift;

	if ($asmlnk) {
		unlink("$f/build/include/asm");
		if (!rename("$f/build/include/asm.sav", "$f/build/include/asm")) {
			print "Cannot restore $f/build/include/asm -- $!\n";
			exit(1);
		}
	}

	if ($modpost) {
		unlink($modpost);
		if (!rename("$modpost.sav", $modpost)) {
			print "Error - cannot restore $modpost - $!\n";
		}
	}
}

#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
Some help...
Usage:

Switches:

EOF

	exit(1);
}

main();
0;

