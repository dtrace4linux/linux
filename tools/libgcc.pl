#! /usr/bin/perl

# Script to locate libgcc.a on your system, needed for 
# x86-32 64-bit div/mod instructions.

use warnings;
use strict;

use FileHandle;

if ( ! -d "build" ) {
	mkdir("build", 0755);
}

unlink("build/libgcc.a");

if ( -f "build/x86-32/libgcc.a" ) {
	exit(0);
}
if ( ! -d "build/x86-32" ) {
	mkdir("build/x86-32", 0755);
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
				unlink("build/x86-32/libgcc.a");
				if (!symlink("$wd/libgcc.a", "build/x86-32/libgcc.a")) {
					print "Error creating symlink build/libgcc.a -- $!\n";
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
