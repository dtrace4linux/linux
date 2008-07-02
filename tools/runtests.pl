#! /usr/bin/perl

# $Header:$

use strict;
use warnings;

use FileHandle;
use Getopt::Long;
use POSIX;

my $cnt = 0;
my $errs = 0;

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
	print $fh $s;

	run_tests("demo");

	$s = strftime("%Y%m%d %H:%M:%S Testing completed. See build/dtrace.report for details\n", localtime);	

	print $s;
	print "Files tested   : $cnt\n";
	print "Errors detected: $errs\n";

	print $fh $s;
	print $fh "Files tested   : $cnt\n";
	print $fh "Errors detected: $errs\n";

}
sub run_tests
{	my $dir = shift;

	foreach my $f (glob("$dir/*")) {
		if (-d $f) {
			run_tests($f);
			next;
		}
		next if $f !~ /\.d$/;

		my $s = strftime("%Y%m%d %H:%M:%S File: $f\n", localtime);	
		print $fh $s;
		my $ret = system("build/dtrace -e -s $f >>build/dtrace.report 2>&1 ");
		if ($ret) {
			$errs++;
		}
		$cnt++;
	}
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

