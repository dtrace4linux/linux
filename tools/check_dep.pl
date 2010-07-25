#! /usr/bin/perl

# $Header:$

# Perl script to poke around and let user know if things
# are going to be good.

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts;

sub main
{
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'help',
		);

	usage() if ($opts{help});

	my $dirty = 0;

	my $str = `grep zlib /proc/kallsyms`;
	chomp($str);
	if ($str !~ /zlib/i) {
		$dirty = 1;
		print <<EOF;
 =================================================================
 === NOTE:
 === Your kernel does not have the zlib compression routines 
 === available, and you will likely have problems loading
 === the dtrace driver into your kernel. You may want to fix that
 === or try another kernel/distribution (Debian 5 appears to be
 === one such known distro).
 =================================================================
EOF
	}

	###############################################
	#   I     dont    like    this    kind    of  #
	#   wait-and-prompt, so disable for now.      #
	###############################################
	if (0 && $dirty) {
		print "Press <Enter> to continue...";
		my $ans = <>;
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

