#! /usr/bin/perl

# $Header:$

# extract vmlinux ELF image from vmlinuz - so we can poke at it.
# dd if=/boot/vmlinuz skip=`grep -a -b -o -m 1 -e $'\x1f\x8b\x08\x00' /boot/vmlinuz | cut -d: -f 1` bs=1 | zcat > /tmp/vmlinux
# 

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

	my $fname = "/boot/vmlinuz-" . `uname -r`;
	chomp($fname);
	my $fh = new FileHandle($fname);
#	$fh->binmode();

	my $data;
	my $pos = 0;
	do {
		$data = "";
		$fh->seek($pos++, 0);
		$fh->read($data, 3);
	} until ($data eq "\x1f\x8b\x08" || $fh->eof());

	if ($fh->eof()) {
		print STDERR "Couldnt find gzip marker\n";
		exit(1);
	}
	$fh->seek(--$pos, 0);
	$data = "";

	while (!$fh->eof()) {
		my $buf;
		$fh->read($buf, 4096);
		$data .= $buf;
	}
	$fh = new FileHandle("| gunzip >/tmp/vmlinux");
	print $fh $data;
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

