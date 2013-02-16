#! /usr/bin/perl

# Script to locate libgcc.a on your system, needed for 
# x86-32 64-bit div/mod instructions in the kernel.

# 10-Feb-2013 PDF Dont complain if this is an ARM machine.
# 16-Feb-2013 PDF Add support for ARM.

use warnings;
use strict;

use FileHandle;

sub main
{
	if (!$ENV{BUILD_DIR}) {
		$ENV{BUILD_DIR} = "build-" . `uname -r`;
		chomp($ENV{BUILD_DIR});
	}
	die "\$BUILD_DIR must be defined before running this script" if !$ENV{BUILD_DIR};

	my $fh;

	$fh = new FileHandle("uname -m |");
	my $ln = <$fh>;

	my $target = "$ENV{BUILD_DIR}/libgcc.a";
	exit(0) if -f $target;

	my $ccode = <<EOF;
	int main(int argc, char **argv)
	{
		return 0;
	}
EOF

	$fh = new FileHandle(">x.c");
	print $fh $ccode;
	$fh->close();

	$fh = new FileHandle("gcc -m32 -v -o x x.c 2>&1 |");
	while (<$fh>) {
		chomp;
		for my $wd (split(" ")) {
			next if $wd !~ /-L/;
			$wd =~ s/^-L//;
			if ( -f "$wd/libgcc.a") {
				###############################################
				#   Empty  pipe  so  we  can remove the temp  #
				#   files.				      #
				###############################################
				while (<$fh>) {
				}

				print "$wd\n";
				###############################################
				#   Remove   destination   -   we  may  have  #
				#   upgraded our OS.			      #
				###############################################
				unlink($target);
				if (!symlink("$wd/libgcc.a", $target)) {
					print "Error creating symlink $target -- $!\n";
					exit(1);
				}
				unlink("x");
				unlink("x.c");
				exit(0);
			}
		}
	}
	unlink("x");

	###############################################
	#   Try an alternate tactic - this for AS2.   #
	###############################################
	my $gcc_dir = "/usr/lib/gcc-lib";
	$gcc_dir = "/usr/lib/gcc" if ! -d $gcc_dir && -d "/usr/lib/gcc";
	$fh = new FileHandle("find $gcc_dir -name libgcc.a |");
	while (<$fh>) {
		chomp;
		if (!symlink($_, $target)) {
			print "Error creating symlink $target -- $!\n";
			exit(1);
		}
		exit(0);
	}
	###############################################
	#   Oh dear.				      #
	###############################################
	print "Sorry: libgcc.a not found. Please put a symlink to it in the\n";
	print "build/ directory for the 32-bit compilation to work.\n";
}
######################################################################
#   Main entry point.						     #
######################################################################
main();
0;
