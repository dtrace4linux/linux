#! /usr/bin/perl

# $Header:$
#
#
# Script to run a set of validation tests against the available D
# scripts to prove we have no parsing issues.

# Paul Fox July 2008
#

use strict;
use warnings;

use FileHandle;
use Getopt::Long;
use POSIX;

my %missing_probes;
my $cnt = 0;
my $errs = 0;
my $syntax = 0;

my $fh;

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts;

sub main
{
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'help',
		);

	usage() if ($opts{help});

	unlink("build/dtrace.report");
	$fh = new FileHandle(">>build/dtrace.report");
	die "Cannot create build/dtrace.report -- $!" if !$fh;

	$fh->autoflush();

	my $s = strftime("%Y%m%d %H:%M:%S Testing started...\n", localtime);	
	print $s;
	print $fh $s;

	run_tests("demo");

	$s = strftime("%Y%m%d %H:%M:%S Testing completed. See build/dtrace.report for details\n", localtime);	

	plog($s);
	plog("Files tested   : $cnt\n");
	plog("Errors detected: $errs\n");
	plog("Syntax errors  : $syntax\n");
	plog("Missing probes:\n");
	foreach my $k (sort(keys(%missing_probes))) {
		plog("  $k\n");
	}

}
sub plog
{	my $str = shift;

	print $str;
	print $fh $str;
}
sub run_tests
{	my $dir = shift;

	foreach my $f (glob("$dir/*")) {
		if (-d $f) {
			run_tests($f);
			next;
		}
		next if $f !~ /\.d$/;

		$cnt++;
		print $fh "\n";
		my $s = strftime("%Y%m%d %H:%M:%S #$cnt File: $f\n", localtime);	
		print $fh $s;
		print $s;

		###############################################
		#   Whether  we  can run a test depends on a  #
		#   number of things. Some require a command  #
		#   line arg. Some want access to a probe we  #
		#   dont currently or can never support. (We  #
		#   will  need to emulate these for Linux if  #
		#   thats the case)			      #
		###############################################
		my $cmd = "build/dtrace -e -s $f";
		my $cfh = new FileHandle("$cmd 2>&1 ; echo STATUS=\$? |");
		my $status;
		my $need_macro = 0;
		while (<$cfh>) {
			if (/^STATUS=(\d+)/) {
				$status = $1;
				last;
			}
			$syntax++ if /syntax error near/;
			$need_macro = 1 if /macro argument/;
			if (/probe description (\S+) does not match/) {
				$missing_probes{$1}++;
			}
			print $fh $_;
		}

		if ($need_macro) {
			$cmd = "build/dtrace -e -s $f xxx";
			print $fh timestamp() . "...Alternate: $cmd\n";
			$cfh = new FileHandle("$cmd 2>&1 ; echo STATUS=$? |");
			while (<$cfh>) {
				if (/^STATUS=(\d+)/) {
					$status = $1;
					last;
				}
				print $fh $_;
			}
		}
		if ($status) {
			$errs++;
		}
	}
}
sub timestamp
{
	return strftime("%Y%m%d %H:%M:%S ", localtime);
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print "runtests.pl -- run the regression tests\n";
	print "Usage: runtests.pl\n";
	print "\n";
	print "Switches:\n";
	print "\n";
	exit(1);
}

main();
0;

