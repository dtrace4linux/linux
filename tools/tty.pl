#! /usr/bin/perl

# Simple tool to put load on a system, whilst having something to
# watch.

use FileHandle;

my $num_cpus = 1;

sub commify {
	local $_ = shift;
	1 while s/^([-+]?\d+)(\d{3})/$1,$2/;
	return $_;
}
sub main
{
	if ($ARGV[0] && $ARGV[0] =~ /-(\d+)/) {
		$ENV{CPUS} = $1;
		shift;
	}
	$ENV{CPUS} = 1 if !defined($ENV{CPUS});
	$ENV{CPU} = 0 if !defined($ENV{CPU});
	$ENV{COUNT} = 0 if !defined($ENV{COUNT});

	$| = 1;
	if ($ENV{COUNT} == 0) {
		print "\033[2J";
	}

	if ($ENV{CPUS} > 1 && !$ENV{FORKED}) {
		$ENV{FORKED} = 1;
		for (my $i = 0; $i < $ENV{CPUS}; $i++) {
			$ENV{CPU} = $i;
			if (fork() == 0) {
				exec $0, 1, 1, 65 + $i;
			}
		}
		while (1) {
			wait();
		}
	}

	my $rows = $ENV{ROWS};
	my $cols = $ENV{COLS};

	if (!defined($rows)) {
		my $fh = new FileHandle("stty -a | ");
		my $line = <$fh>;
		$line =~ m/rows (\d+); columns (\d+);/;
		($rows, $cols) = ($1, $2);
		$ENV{ROWS} = $rows;
		$ENV{COLS} = $cols;
	}
	my $fh = new FileHandle("/proc/dtrace/stats");
	my $str = "\033[1;1H\033[37m";
	while (<$fh>) {
		chomp;
		next if $ENV{CPU} != 0;
		my ($name, $val) = split(/=/);
		$val = commify($val);
		$str .= "$name=$val\n";
		}
	my $lines = $.;
	if ($ENV{CPU} == 0) {
		my $cmd = "tail -" . ($rows - $lines - 1) . " /var/log/kern.log";
#		$str .= `$cmd`;
	}

	my $r = $ARGV[0] || 1;
	my $c = $ARGV[1] || 1;
	my $ch = $ARGV[2] || 65;
	$r = $lines + 1  if $r < $lines + 1;

	my $fg = sprintf("\033[%dm", 31 + $ENV{CPU});
	$str .= sprintf("$fg\033[%d;40HCount: $ENV{COUNT}", $ENV{CPU} + 1);
#	$str .= sprintf "$\033[$r;${c}H%c", $ch;
	$ENV{COUNT}++;

	$c += $ENV{CPUS};
	if ($c >= $cols) {
		$c -= $cols;
		$r++;
	}
	if ($r >= $rows) {
		$r = 1;
		$ch += $ENV{CPUS};
		if ($ch > ord('z')) {
			$ch = 65 + $ch - ord('z');
		}
	}

	print $str;

	exec $0, $r, $c, $ch;
}
main();
0;

