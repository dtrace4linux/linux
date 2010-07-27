#! /usr/bin/perl

# Script to populate /usr/lib/dtrace with Linux/kernel specific files.
# $Header:$

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;

my $license = <<EOF;
/*
GPLv3 license. This is a generated file from mkinstall.pl.

This file is part of the Dtrace/Linux distribution.
EOF

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts = (
	o => "/usr/lib/dtrace",
	);

sub main
{
	Getopt::Long::Configure('require_order');
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'help',
		'o=s',
		);

	usage() if $opts{help};

	$license .= strftime("Generated: %Y%m%d %H:%M:%S\n", localtime());
	$license .= "*/\n";

	gen_signal_d();
	gen_regs_d();
}
sub gen_regs_d
{	my $fname = $opts{o} . "/regs.d";

	system("cp etc/regs.d $opts{o}");
}
sub gen_signal_d
{	my $fname = $opts{o} . "/signal.d";

	my $fh = new FileHandle(">$fname");
	die "Cannot create $fname - $!" if !$fh;

	my $fh1 = new FileHandle("/usr/include/bits/signum.h");

	print $fh $license;
	print $fh "\n";

	my %sigs;

	while (<$fh1>) {
		next if !/^#define\s+(SIG[A-Z0-9]+)\t+(\d+)/;
		print $fh "inline int $1 = $2;\n";
		print $fh "#pragma D binding \"1.0\" $1\n";
	}
	print $fh "inline int SIGPOLL = SIGIO;\n";
	print $fh "#pragma D binding \"1.0\" SIGPOLL\n";

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

