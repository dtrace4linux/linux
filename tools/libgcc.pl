#! /usr/bin/perl

# Script to locate libgcc.a on your system, needed for 
# x86-32 64-bit div/mod instructions.

use warnings;
use strict;

use FileHandle;

sub main
{
	die "\$BUILD_DIR must be defined before running this script" if !$ENV{BUILD_DIR};

	my $target = "$ENV{BUILD_DIR}/libgcc.a";
	exit(0) if -f $target;

	my $ccode = <<EOF;
	int main(int argc, char **argv)
	{
		return 0;
	}
EOF

	my $fh = new FileHandle(">x.c");
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
	print "Sorry: libgcc.a not found. Please put a symlink to it in the\n";
	print "build/ directory for the 32-bit compilation to work.\n";
}
######################################################################
#   Main entry point.						     #
######################################################################
main();
0;
