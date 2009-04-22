#! /usr/bin/perl

# $Header:$

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;

######################################################################
#   Map of common groups of calls to invoke.			     #
######################################################################
my %calls = (
	chdir 	=> "chdir",
	connect => "connect accept listen bind",
	exec 	=> "exec*",
	file 	=> "chdir chmod chown mkdir open* rmdir remove symlink unlink",
	fork 	=> "fork* vfork* clone*",
	mkdir 	=> "mkdir",
	open 	=> "open*",
	remove 	=> "unlink* rmdir remove*",
	socket  => "socket connect accept listen bind shutdown setsockopt getsockopt",
	stat 	=> "lstat* stat*",
	symlink => "symlink",
	);

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts = (
	secs => 5
	);

sub main
{
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'help',
		'l',
		'secs=s',
		);

	usage() if ($opts{help});

	my $cmd = shift @ARGV;
	my $mode = shift @ARGV || "scroll";
	usage() if !$cmd;
	usage() if !defined($calls{$cmd});
	my $d = "#pragma D option quiet\n";

	my $comma = "";
	foreach my $call (split(" ", $calls{$cmd})) {
		$d .= "${comma}syscall::$call:entry";
		$comma = ",\n";
	}
	$d .= " {\n";
	if (!$mode) {
		print "DTrace $call $count: list probes as they occur. Ctrl-C to exit.\n";
		$d .= "\tprinta(\"%5d %-16s %-32s %d\\n\", pid, probefunc, execname, count());\n";
	} elsif ($mode eq 'count') {
		print "DTrace $call $count: collect probes until Ctrl-C.\n";
		$d .= "\t\@num[probefunc] = count();\n";
	} elsif ($mode eq 'count1') {
		print "DTrace $call $count: collect probes until Ctrl-C.\n";
		$d .= "\t\@num[probefunc, execname] = count();\n";
		$d .= "}\n";
		$d .= "END {\n";
		$d .= "\tprintf(\"Grand summary:\\n\");\n";
		$d .= "\tprinta(\"%-16s %-32s %\@d\\n\", \@num);\n";
	} elsif ($mode eq 'count2') {
		print "DTrace $call $count: list probes every $opts{secs}s.\n";
		print "Ctrl-C to exit.\n";
		$d .= "\t\@num[probefunc] = count();\n";
		$d .= "\t\@tot[probefunc] = count();\n";
		$d .= "}\n";
		$d .= "tick-$opts{secs}sec {\n";
		$d .= "\tprinta(\"%-16s %-32s %\@d\\n\", \@num);\n";
		$d .= "\tclear(\@num);\n";
		$d .= "}\n";
		$d .= "END {\n";
		$d .= "\tprintf(\"Grand summary:\\n\");\n";
		$d .= "\tprinta(\"%-16s %-32s %\@d\\n\", \@num);\n";
	}
	$d .= "}\n";

	$d = "dtrace -n '$d'";

	if ($opts{l}) {
		print $d, "\n";
		exit(0);
	}
	return system($d);
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
dt -- simple interface to useful dtrace scripts
Usage: dt <cmd> <mode>

Description:

  dt is a simple wrapper around dtrace providing access to a library
  of useful examples and scenarios.

Commands:

  The following commands are available, and expand into traces against
  the listed system calls.

  chdir   => chdir
  connect => connect accept listen bind
  exec    => exec*
  file    => chdir chmod chown mkdir open* rmdir remove symlink unlink
  fork    => fork* vfork* clone*
  mkdir   => mkdir
  open    => open*
  remove  => unlink* rmdir remove*
  stat    => lstat* stat*
  socket  => socket connect accept listen bind shutdown setsockopt getsockopt
  symlink => symlink

Mode:

  This word is used to refine the type of collection. Possible values are

  (empty)    List probe events as they occur.
  count      Collect probes until user types Ctrl-C.
  count1     Same as 'count', but include execname.
  count2     Same as 'count1', but also dump exec $opts{secs}s.

Switches:

  -l         List the D program without executing.

Examples:

  dt fork count2

EOF

	exit(1);
}

main();
0;

