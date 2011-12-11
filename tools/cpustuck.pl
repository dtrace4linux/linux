#! /usr/bin/perl

# $Header:$

use strict;
use warnings;

use FileHandle;
use POSIX;

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts;

sub main
{
	$| = 1;
	if (defined($ARGV[0])) {
		my $n = 0;
		my $old_t = 0;

		my $tmo = 1.0;

		while (1) {
			if ($ARGV[0]) {
				print "$ARGV[0]";
				select(undef, undef, undef, $tmo);
				next;
			}

			my $t = strftime("%H:%M:%S", localtime());
			if (substr($t, 0, 5) eq substr($old_t, 0, 5)) {
				print $ARGV[0];
				select(undef, undef, undef, $tmo);
				next;
			}
			$old_t = $t;

			print "\n" . $old_t . " " . $ARGV[0];
			select(undef, undef, undef, $tmo);
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

