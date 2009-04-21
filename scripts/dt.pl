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
	exec 	=> "exec*",
	file 	=> "chdir chmod chown mkdir open* rmdir remove symlink unlink",
	fork 	=> "fork* vfork* clone*",
	mkdir 	=> "mkdir",
	open 	=> "open*",
	remove 	=> "unlink* rmdir remove*",
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
	$d .= "{\n";
	if ($mode eq 'count') {
		$d .= "\t\@num[probefunc] = count();\n";
	} elsif ($mode eq 'count2') {
		$d .= "\t\@num[probefunc, execname] = count();\n";
	} elsif ($mode eq 'count3') {
		$d .= "\t\@num[probefunc] = count();\n";
		$d .= "\t\@tot[probefunc] = count();\n";
		$d .= "}\n";
		$d .= "tick-$opts{secs}sec { printa(\@num); clear(\@num); }\n";
		$d .= "END {\n";
		$d .= "\tprintf(\"Grand summary:\\n\");\n";
		$d .= "\tprinta(\@num);\n";
	} elsif ($mode eq 'scroll') {
	} else {
		$d .= "\tprintf(\"%d %s:%s %s\\n\", pid, execname, probefunc, copyinstr(arg0));\n";
	}
	$d .= "}\n";

	if ($opts{l}) {
		print $d, "\n";
		exit(0);
	}
	
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

