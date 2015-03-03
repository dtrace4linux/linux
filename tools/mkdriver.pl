#! /usr/bin/perl

# Perl script to help in building kernel objects away
# from the master sources in the drivers/dtrace area.
# This allows us to use the same source tree to build
# or validate compilation against multiple kernels.

# Author: Paul Fox
# Date: June 2008

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;
use Cwd;

sub main
{

	my $cmd = shift @ARGV;
	die "Usage: mkdriver.pl [all | clean]\n" if !$cmd;

	my $driver = "driver";
	if ($cmd eq 'driver-2' || $cmd eq 'driver-kmem') {
		$driver = $cmd;
		$cmd = shift @ARGV;
	}

	###############################################
	#   Allow  clean  out  of  context.  We dont  #
	#   really  need  this  since  all  volatile  #
	#   stuff is in the build.* dirs anyhow.      #
	###############################################
	if ($cmd eq 'clean') {
		if (!defined($ENV{BUILD_DIR})) {
			return system("rm -rf build*");
		}
		return system("rm -rf $ENV{BUILD_DIR}/$driver");
	}

	$ENV{BUILD_DIR} = "build" if ! $ENV{BUILD_DIR};
	die "$0: \$BUILD_DIR not defined of directory does not exist." if ! -d $ENV{BUILD_DIR};
	if (! -d "$ENV{BUILD_DIR}/$driver" && !mkdir("$ENV{BUILD_DIR}/$driver", 0755)) {
		die "Cannot create $ENV{BUILD_DIR} - $!\n";
	}

	###############################################
	#   Symlink the files to build/dtrace/ dir.   #
	###############################################
	foreach my $f (qw/Makefile *.c *.h *.S/) {
		foreach my $f1 (glob("$driver/$f")) {
			my $target = "$ENV{BUILD_DIR}/$driver/" . basename($f1);
			my $lnk = readlink("$target");
			if (! $lnk || $lnk ne "../../$f1") {
				unlink($target);
				print "symlink ../../$f1 $target\n";
				if (!symlink("../../$f1", "$target")) {
					die "Error: $!\n";
				}
			}
		}
	}

	###############################################
	#   Now do the build.			      #
	###############################################
	$cmd = Cwd::realpath(dirname($0)) . "/make-me";
	if (!chdir("$ENV{BUILD_DIR}/$driver")) {
		die "Cannot cd to $ENV{BUILD_DIR}/$driver dir -- $!\n";
	}

	print "Executing: $cmd\n";
	exec $cmd ||
		print "Cannot execute $cmd -- $!\n";
	exit(1);
}

main();
0;

