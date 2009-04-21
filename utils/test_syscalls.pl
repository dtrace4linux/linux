#! /usr/bin/perl

# $Header:$

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

	my $fname = glob("build/driver/syscalls-*.tbl");
	my $fh = new FileHandle($fname);
	my $str = <<EOF;
int main(int argc, char **argv)
{
EOF
	while (<$fh>) {
		next if !/^.*defined\(__NR_(.*)\)/;
		my $func = $1;
		next if $func eq 'exit'; 
		$str .= "\t$1();\n";
	}
	$str .= "	exit(0);\n";
	$str .= "}\n";
	print $str;
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
