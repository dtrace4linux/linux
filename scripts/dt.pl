#! /usr/bin/perl

# $Header:$

# Author: Paul D. Fox
#
# Tool to wrap dtrace and provide convenience high level access
# to some system probes (mostly syscalls)
#
# Avoids need to find a script when you dont know the name of the thing.
# Designed to allow extensions to add higher level D scripts.
#

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
	dir	=> "getdents",
	exec 	=> "exec*",
	fbt     => "",
	file 	=> "chdir chmod chown mkdir open* rmdir symlink unlink",
	fork 	=> "fork* vfork* clone*",
	io      => "",
	mkdir 	=> "mkdir",
	mmap  	=> "mmap munmap",
	open 	=> "open*",
	remove 	=> "unlink* rmdir ",
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
		'v',
		);

	usage() if ($opts{help});

	my $cmd = shift @ARGV;
	my $mode = shift @ARGV || "scroll";
	usage() if !$cmd;
	usage() if !defined($calls{$cmd});
	my $d = ""; #"#pragma D option quiet\n";

	my $comma = "";
	my $width = 16;
	if ($cmd eq 'fbt') {
		$d .= "fbt:::entry";
		$width = 25;
	} elsif ($cmd eq 'io') {
		$d .= "#pragma options quiet\n";
		$d .= "io::: /execname != \"dtrace\"/ {\n";
		$d .= "printf(\"%-8s %5d %s\\n\", probename, pid, execname);\n";
		$d .= "}\n";
		do_dtrace($d);
		return;
	} else {
		foreach my $call (split(" ", $calls{$cmd})) {
			$d .= "${comma}syscall::$call:entry";
			$comma = ",\n";
		}
	}

	$d .= " {\n";
	if (!$mode) {
		print "DTrace $cmd $mode: list probes as they occur. Ctrl-C to exit.\n";
		$d .= "\tprinta(\"%5d %-${width}s %-32s %d\\n\", pid, probefunc, execname, count());\n";
	} elsif ($mode eq 'count') {
		print "DTrace $cmd $mode: collect probes until Ctrl-C.\n";
		$d .= "\t\@num[probefunc] = count();\n";
	} elsif ($mode eq 'count1') {
		print "DTrace $cmd $mode: collect probes until Ctrl-C.\n";
		$d .= "\t\@num[probefunc, execname] = count();\n";
		$d .= "}\n";
		$d .= "END {\n";
		$d .= "\tprintf(\"Grand summary:\\n\");\n";
		$d .= "\tprinta(\"%-${width}s %-32s %\@d\\n\", \@num);\n";
	} elsif ($mode eq 'count2') {
		print "DTrace $cmd $mode: list probes every $opts{secs}s.\n";
		print "Ctrl-C to exit.\n";
		$d .= <<EOF;
	\@num[probefunc, execname] = count();
	\@tot[probefunc, execname] = count();
}
tick-$opts{secs}sec {
	printf("\\n");
	printa("%-${width}s %-32s %\@d\\n", \@num);
	clear(\@num);
}
END {
	printf("Grand summary:\\n");
	printa("%-${width}s %-32s %\@d\\n", \@tot);
EOF
	}
	$d .= "}\n";

	do_dtrace($d);
}
sub do_dtrace
{	my $d = shift;

	$d = "dtrace -n '$d'";

	if ($opts{v} || $opts{l}) {
		print "### Script:\n";
		print $d, "\n";
		print "### End of script\n";
	}
	exit(0) if $opts{l};

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
  dir     => getdents
  fbt     =>  not-applicable
  file    => chdir chmod chown mkdir open* rmdir remove symlink unlink
  fork    => fork* vfork* clone*
  mkdir   => mkdir
  mmap    => mmap
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
  -secs NN   When dumping periodically, dump after NN seconds.
  -v         List program but continue executing.

Examples:

  dt fork count2

EOF

	exit(1);
}

main();
0;

