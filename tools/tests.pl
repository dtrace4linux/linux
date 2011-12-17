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

my $exit_code = 0;
my $ctrl_c = 0;
my $master_dmesg;

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts = (
	count => 1,
	loop => 1000,
	);

sub commify {
	local $_ = shift;
	1 while s/^([-+]?\d+)(\d{3})/$1,$2/;
	return $_;
}

sub do_child
{	my $ppid = shift;

	$ppid = $$ if !$ppid;

	while (!$ctrl_c && -d "/proc/$ppid") {
		my $fh = new FileHandle("/etc/hosts");
		my $str = <$fh>;
		new FileHandle("/etc/hosts-nonexistant");
		###############################################
		#   We may/may not have each binary.	      #
		###############################################
		if (-f "build/sys64") {
			system("build/sys64");
			system("strace build/sys64 >/dev/null 2>&1");
		}
		if (-f "build/sys32") {
			system("build/sys32");
			system("strace build/sys32 >/dev/null 2>&1");
		}

		system("du /proc >/dev/null 2>&1");
		if (fork() == 0) {
			exit(0);
		}
		wait;
	}
}
######################################################################
#   Read the tests.d file and run each test.			     #
######################################################################
sub do_tests
{
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
	#   Fork  children  to keep us busy. We want  #
	#   all cpus busy.			      #
	###############################################
	my $ncpu = `grep 'processor\t:' /proc/cpuinfo | wc -l`;
	chomp($ncpu);
	my $ppid = $$;
	my %pids;
	for (my $i = 0; $i < $ncpu; $i++) {
		my $pid = fork();
		if ($pid == 0) {
			do_child($ppid);
			exit(0);
		}
		$pids{$pid} = 1;
	}

	if ($opts{child}) {
		print "No dtrace testing - but waiting for children\n";
		pause();
		exit(0);
	}

	print "Tests:\n";
	foreach my $info (@tests) {
		my $dm = get_dmesg();
		if ($dm ne $master_dmesg) {
			my $fh = new FileHandle(">/tmp/dmesg.orig");
			print $fh $master_dmesg;
			$fh = new FileHandle(">/tmp/dmesg.now");
			print $fh $dm;
			$fh->close();
			system("diff /tmp/dmesg.orig /tmp/dmesg.now");

			print "Warning: dmesg output changed...aborting tests so you can verify\n";
			exit(1);
		}

		if (@ARGV) {
			my $found = 0;
			foreach my $arg (@ARGV) {
				$found = 1 if $arg eq $info->{name};
			}
			next if !$found;
		}


		print time_string() . "Test: ", $info->{name}, "\n";
		my $d = $info->{d};
		my $loop = $opts{loop};
		$d =~ s/\${loop}/$loop/g;
		if ($d eq '') {
			print "Something wrong with this test. Came out as blank!\n";
			exit(1);
		}
		my $cmd = $opts{dtrace} ? "dtrace -n '$d'" : "build/dtrace -n '$d'";
		my $ret = spawn($cmd, $info->{name});
		$exit_code ||= $ret;
		dump_stats();
	}
	kill SIGKILL, $_ foreach keys(%pids);
	while (wait > 0) {
	}

}
sub dump_stats
{
	my $fh = new FileHandle("/proc/dtrace/stats");
	while (<$fh>) {
		chomp;
		my ($name, $val) = split(/=/);
		$val = commify($val);
		print "$name=$val\n";
	}
}

######################################################################
#   Read kernel messages so we can detect something happening.	     #
######################################################################
sub get_dmesg
{
	my $fh = new FileHandle("dmesg | ");
	my $str = '';
	while (<$fh>) {
		$str .= $_;
	}
}

######################################################################
#   Main entry point.						     #
######################################################################
sub main
{
	Getopt::Long::Configure('require_order');
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'child',
		'count=s',
		'dtrace',
		'help',
		'loop=s',
		);

	usage() if $opts{help};
	$| = 1;

	$SIG{INT} = sub { $ctrl_c = 1; exit(0); };

	if ($opts{child}) {
		do_child();
		exit(0);
	}

	my $print_msg = ! -f ".test.prompt";
	new FileHandle(">.test.prompt");
	if ($print_msg) {
		print <<EOF;
You are about to run a serious of tests which attempt to do reasonable
coverage of dtrace in core areas. This deliberately involves forcing
page faults and GPFs in the kernel, in a recoverable and safe way.

Each test is logged to the /tmp/ directory with a file with the same
name as the test. You mostly dont need to worry about the output of a test,
except if your kernel crashes.

Progress messages and occasional output is presented, so that you can
feel secure knowing your system is still running.

These tests may crash your kernel - and better to know this up front before
you assume dtrace/linux is production worthy.

These tests will become less "noisy" and will be extended with additional
use cases. Dont worry about errors like:

dtrace: error on enabled probe ID 2 (ID 274009: syscall:x64:open:entry): invalid address (0xffff880013769ed8) in action #7

in the output for now. These will be tidied up.

Press <Enter> if you understand the above and would like to continue:
EOF
		my $ans = <STDIN>;
	}

	if (! -f "/proc/dtrace/stats") {
		print "dtrace driver does not appear to be loaded.\n";
		exit(1);
	}

	###############################################
	#   Grab   a  copy  of  dmesg.  If  anything  #
	#   changes during testing, abort the tests.  #
	###############################################
	$master_dmesg = get_dmesg();

	for (my $i = 0; $i < $opts{count}; $i++) {
		do_tests();
	}
	print time_string() . "All tests completed.\n";
	print <<EOF if $print_msg;

You can look at /proc/dtrace/stats to see the number of interrupts
and probes executed. You really want "probe_recursion" to be a small
or zero number. At present, it may be non-zero due to timer interrupts
interrupting another probe in progress.

For the 1/2 counters - "1" means we got the interrupt but it wasnt
for us; the "2" counter means we took a dtrace handled interrupt.

int3: breakpoint trap. (Used by fbt provider)
gpf:  general protection fault. (Shouldnt see these normally).
pf:   page faults. will see these for badaddr probes, e.g. copyinstr(arg)
snp:  segment not present. (Shouldnt see these normally).

EOF
#	dump_stats();
	exit($exit_code);
}
######################################################################
#   Execute command but try and avoid flooding terminal with output  #
#   from dtrace tests.						     #
######################################################################
sub spawn
{	my $cmd = shift;
	my $name = shift;

	my $user = getpwuid(getuid());
	my $fname = "/tmp/test-$user.$name.log";

	unlink($fname);
	if (-f $fname) {
		print "Couldnt remove $fname - maybe a permission issue.\n";
		exit(1);
	}
	$cmd .= " >$fname 2>&1";
	print $cmd, "\n";
	if (fork() == 0) {
		exec $cmd;
	}

	print time_string() . "=== Test: $name\n";
	while (1) {
		my $kid = waitpid(-1, WNOHANG);
		if ($kid > 0) {
			system("tail $fname");
			return $?;
		}
		sleep(1);
		my $fh = new FileHandle("/proc/dtrace/stats");
		my $probes = <$fh>;
		chomp($probes);
		my ($pname, $val) = split(/=/, $probes);

		$fh = new FileHandle("/proc/loadavg");
		my $lav = <$fh>;
		chomp($lav);

		printf time_string() . "  $pname=%15s Load: $lav\n", 
			commify($val);

	}
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
Usage: tests.pl [<test-name>]

  Tool to run the tests in a script file (tests/tests.d) which are
  small D scripts, typically used during development, to validate that
  various probe features work properly on a running kernel.

  If these scripts panic your kernel, please report which test is
  failing.

  When run with no arguments, the list of test names is listed.

  Testing will be aborted if this script detects a problem, e.g.
  kernel messages as a result of probing.

Switches:

  -child      Just run the child test - dont run dtrace at same time.
  -loop NN    Loop NN times instead of max $opts{loop} times.
  -count NN   Repeat all (or specified) tests NN times (default 1).

Examples:

   \$ tests.pl
   	Run all tests
   \$ tests.pl -count 20 BEGIN-fbt:exec-time
        Run specified test 20 times.

EOF

	exit(1);
}

main();
0;

