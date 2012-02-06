#! /usr/bin/perl

# $Header:$

# Simple script to package up the master source into a tarball for
# downloading. Invoked by 'make release'.
#
# Author: Paul D. Fox
# January 2011

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;
use Cwd;

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
		'n',
		'nocopy',
		);

	usage() if $opts{help};

	my $fh = new FileHandle("Changes");
	my $msg;
	while (<$fh>) {
		chomp;
		next if !/^     /;
		$msg = "$_\n";
		while (<$fh>) {
			chomp;
			last if $_ eq '';
			$msg .= "$_\n";
		}
		last;
	}
	print "msg=$msg\n" if $opts{v};
	$fh = new FileHandle(">/tmp/msg");
	print $fh $msg;
	$fh->close();

	my $pwd = Cwd::cwd();

	my $rel = $ARGV[0] || strftime("%Y%m%d", localtime);
	###############################################
	#   Handle multireleases in a day.	      #
	###############################################
	my $pkg = "dtrace";
	if (-f "$ENV{HOME}/release/$pkg/$pkg-$rel.tar.bz2") {
		foreach my $letter (qw/a b c d e f g h i j k l m n o p q r s t u v w x y z/) {
			if (! -f "$ENV{HOME}/release/$pkg/$pkg-$rel$letter.tar.bz2") {
				$rel .= $letter;
				last;
			}
		}
	}
	$fh = new FileHandle(".release");
	die "Cannot open .release file -- $!\n" if !$fh;
	my %vars;
	while (<$fh>) {
		chomp;
		$_ =~ /^(.*)=(.*$)/;
		$vars{$1} = $2;
	}

	$vars{build}++;

	$fh = new FileHandle(">.release");
	print $fh "date=" . `date`;
	print $fh "release=dtrace-$rel\n";
	print $fh "build=$vars{build}\n";
	$fh->close();

	system("find . -name checksum.lst | xargs rm -f");
	chdir("..");
	rename("dtrace", "dtrace-$rel");
	system(
		"tar cvf - --exclude=*.o " .
			"--exclude=.*.cmd " .
			"--exclude=*.so " .
			"--exclude=*.mod.c " .
			"--exclude=build/ " .
			"--exclude=build-* " .
			"--exclude=dt_grammar.h " .
			"--exclude=dt_lex.c " .
			"--exclude=.tmp_versions " .
			"--exclude=Module.symvers " .
			"--exclude=*.ko " .
			"--exclude=*.a " .
			"--exclude=*.mod " .
			"--exclude=tags " .
			"--exclude=lwn " .
			"--exclude=.dtrace.nobug " .
			"--exclude=.test.prompt " .
			"--exclude=.first-time " .
			"dtrace-$rel | bzip2 >/tmp/dtrace-$rel.tar.bz2");
	rename("dtrace-$rel", "dtrace");

	if (!$opts{nocopy}) {
		spawn("rcp /tmp/dtrace-$rel.tar.bz2 minny:release/website/dtrace");
	}
	spawn("ls -l /tmp/dtrace-$rel.tar.bz2");
	spawn("mv /tmp/dtrace-$rel.tar.bz2 $ENV{HOME}/release/dtrace");

	spawn("twit 'Release: dtrace-b$vars{build} on ftp://crisp.dyndns-server.com/pub/release/website/dtrace/dtrace-$rel.tar.gz'");
	chdir($pwd);
	spawn("git commit -F /tmp/msg .");
}
sub spawn
{	my $cmd = shift;

	print strftime("%H:%M:%S ", localtime) . $cmd . "\n";
	return if $opts{n};
	return system($cmd);
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
mkrelease.pl -- create a new distribution release
Usage: mkrelease.pl [-nocopy]

Switches:

EOF

	exit(1);
}

main();
0;

