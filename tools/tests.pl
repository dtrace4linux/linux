#! /usr/bin/perl

# $Header:$

# Author : Paul Fox
# Date: April 2011

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
	Getopt::Long::Configure('require_order');
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'help',
		);

	usage() if $opts{help};

	my @tests;
	my $fname = "tests.d";
	$fname = "tests/tests.d" if ! -f $fname;
	my $fh = new FileHandle($fname);
	die "Cannot open $fname -- $!" if !$fh;
	my $name = '';
	my $note = '';
	while (<$fh>) {
		chomp;
		if (/^name:\s+(.*)$/) {
			$name = $1;
			next;
		}
		if (/^note:\s+(.*$)/) {
			if ($1) {
				$note = $1;
				next;
			}
			$note = '';
			while (<$fh>) {
				chomp;
				last if !/^\t/;
				$note .= "$_\n";
			}
			next;
		}
		if (/^d:/) {
			my $d = '';
			while (<$fh>) {
				chomp;
				last if !/^\t/;
				$d .= "$_\n";
			}
			my %info;
			$info{name} = $name;
			$info{note} = $note;
			$info{d} = $d;
			push @tests, \%info;
			next;
		}
	}

	$| = 1;
	print "Tests:\n";
	my $exit_code = 0;
	my $arg = shift(@ARGV) || "";
	foreach my $info (@tests) {
		print time_string() . "Test: ", $info->{name}, "\n";
		next if $arg ne 'run' && $arg ne $info->{name};
		my $cmd = "build/dtrace -n '$info->{d}'";
		print $cmd, "\n";
		my $ret = system($cmd);
		$exit_code ||= $ret;
	}
	print time_string() . "All tests completed.\n";
	exit($exit_code);
}
sub time_string
{
	return strftime("%Y%m%d %H:%M:%S ", localtime);
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
tests.pl - run a series of regression tests.
Usage: tests.pl [run | <test-name>]

  Tool to run the tests in a script file (tests/tests.d) which are
  small D scripts, typically used during development, to validate that
  various probe features work properly on a running kernel.

  If these scripts panic your kernel, please report which test is
  failing.

  When run with no arguments, the list of test names is listed.

Switches:

EOF

	exit(1);
}

main();
0;

