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
my %opts = (
	loop => 1000
	);

sub do_child
{	my $ppid = shift;

	while (-d "/proc/$ppid") {
		new FileHandle("/etc/hosts");
		new FileHandle("/etc/hosts-nonexistant");
	}
}
sub main
{
	Getopt::Long::Configure('require_order');
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'help',
		'loop=s',
		);

	usage() if $opts{help};

	print <<EOF;
You are about to run a serious of tests which attempt to do reasonable
coverage of dtrace in core areas. This deliberately involves forcing
page faults and GPFs in the kernel, in a recoverable and safe way.

These tests may crash your kernel - and better to know this up front before
you assume dtrace/linux is production worthy.

These tests will become less "noisy" and will be extended with additional
use cases. Dont worry about errors like:

dtrace: error on enabled probe ID 2 (ID 274009: syscall:x64:open:entry): invalid address (0xffff880013769ed8) in action #7

in the output for now. These will be tidied up.

Press <Enter> if you understand the above and would like to continue:
EOF
	my $ans = <STDIN>;

	if (! -f "/proc/dtrace/stats") {
		print "dtrace driver does not appear to be loaded.\n";
		exit(1);
	}

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

	###############################################
	#   Fork a child to keep us busy.	      #
	###############################################
	$SIG{INT} = sub { exit(0); };
	my $ppid = $$;
	my $pid = fork();
	if ($pid == 0) {
		do_child($ppid);
		exit(0);
	}

	$| = 1;
	print "Tests:\n";
	my $exit_code = 0;
	my $arg = shift(@ARGV) || "";
	foreach my $info (@tests) {
		print time_string() . "Test: ", $info->{name}, "\n";
		next if $arg ne 'run' && $arg ne $info->{name};
		my $d = $info->{d};
		my $loop = $opts{loop};
		$d =~ s/\${loop}/$loop/g;
		my $cmd = "build/dtrace -n '$d'";
		print $cmd, "\n";
		my $ret = system($cmd);
		$exit_code ||= $ret;
	}
	kill SIGKILL, $pid;

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

  -loop NN    Loop NN times instrad of max $opts{loop} times.
EOF

	exit(1);
}

main();
0;

