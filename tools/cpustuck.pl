#! /usr/bin/perl

# $Header:$

use strict;
use warnings;

use FileHandle;

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts;

sub main
{
	$| = 1;
	if (defined($ARGV[0])) {
		while (1) {
			print "$ARGV[0]";
			sleep(1);
		}
	}

	my $fh = new FileHandle("/proc/cpuinfo");
	my $cpus = 1;
	while (<$fh>) {
		if (/^siblings.*: (\d+)$/) {
			$cpus = $1;
			last;
		}
	}

	my $i = 0;
	for (; $i < $cpus-1; $i++) {
		if (fork() == 0) {
			system("taskset -c $i $0 $i");
			exit(0);
		}
	}
	system("taskset -c $i $0 $i");
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
cpustuck.pl -- tool to check for stuck cpus
Usage: cpustuck.pl

  This tool forks one process per processor, and assigns each process
  to a separate cpu. Once a second, the cpu number is printed. If
  a CPU is stuck, then we can see, because it will be missing from the
  output.

  Test tool to check dtrace and help debug stuck cpus.

Switches:

EOF

	exit(1);
}

main();
0;

