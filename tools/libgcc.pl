#! /usr/bin/perl

# Script to locate libgcc.a on your system, needed for 
# x86-32 64-bit div/mod instructions.

use warnings;
use strict;

use FileHandle;

if ( -f "build/libgcc.a" ) {
	exit(0);
}

sub main
{
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
				unlink("x");
				unlink("x.c");
				print "$wd\n";
				if (!symlink("$wd/libgcc.a", "build/libgcc.a")) {
					print "Error creating symlink build/libgcc.a -- $!\n";
					exit(1);
				}
				exit(0);
			}
		}
	}
	print "Sorry: libgcc.a not found. Please put a symlink to it in the\n";
	print "build/ directory for the 32-bit compilation to work.\n";
}
######################################################################
#   Main entry point.						     #
######################################################################
main();
0;
