#! /usr/bin/perl

# $Header:$

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;

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
	patch_module_h();
}
######################################################################
#   This  function  patches the include/linux/module.h file for the  #
#   probe data members needed for the various drivers.		     #
######################################################################
sub patch_module_h
{
	my $fh = new FileHandle
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print "Some help...\n";
	print "Usage: \n";
	print "\n";
	print "Switches:\n";
	print "\n";
	exit(1);
}

main();
0;

