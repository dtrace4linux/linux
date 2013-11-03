#! /usr/bin/perl

# $Header:$
#
# Author: Paul D Fox
# Date: November 2013

use strict;
use warnings;

use FileHandle;
use Getopt::Long;
use IO::File;

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
		);

	usage() if $opts{help};

	my %types;
	my %struct;

	my $fname = shift @ARGV;
	my $fh = new FileHandle("readelf --debug-dump=info $fname |");
	my $sec = '';
	my $sou = '';
	my $addr = '';
	while (<$fh>) {
		chomp;
		if (/(DW_TAG_structure_type)/)  {
			$sec = $1;
			if (defined($struct{$sec})) {
				delete($struct{$sec});
			}
			next;
		}
		if (/(DW_TAG_structure_type|DW_TAG_member)/)  {
			$sec = $1;
			next;
		}
		if (/<[^>]*><([0-9a-f]+)>.*DW_TAG_base_type/)  {
			$addr = $1;
			while (<$fh>) {
				chomp;
				if (/DW_AT_name.*: ([^\s:]*)/) {
					my $name = $1;
					$types{$addr} = $1;
#print "base:$addr='$1'\n";
					last;
				}
			}
			next;
		}
		if (/Abbrev Number: 0$/) {
			$sou = '';
			$sec = '';
			next;
		}
		next if !$sec;

		if ($sec eq 'DW_TAG_structure_type' && /DW_AT_name.*: ([^:]*)$/) {
			$sou = $1;
			$sou =~ s/\s+$//;
			next;
		}
		if ($sec eq 'DW_TAG_member' && /DW_AT_name.*: ([^:]*)$/) {
			my $mem = $1;
			$mem =~ s/\s+$//;
			my $loc = -1;
			my $type = "anon";
			while (<$fh>) {
				if (/DW_AT_type.*:.*<(.*)>/) {
					$type = $1;
					next;
				}
				if (/location: (\d+)/) {
					$loc = $1;
					last;
				}
			}
			my %info;
			$info{name} = $mem;
			$type =~ s/^0x//;
			$info{offset} = $loc;
			$info{type} = $type;
			push @{$struct{$sou}}, \%info;
			next;
		}
	}

	foreach my $s (sort(keys(%struct))) {
		print "struct $s {\n";
		foreach my $mem (@{$struct{$s}}) {
			printf "    %-16s ", $types{$mem->{type}} || "??";
			printf "    %s; // @%03d $mem->{type}\n",
				$mem->{name},
				$mem->{offset}; 
		}
		print "};\n";
	}
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
readelf.pl -- script to read struct info from an ELF file
Usage:

  This tool uses readelf to extract the DWARF info from an
  executable or object file, with debugging information. This
  is a portable mechanism, vs libdwarf or libdw, given the
  utter mess the many versions of these libraries present.

Switches:

EOF

	exit(1);
}

main();
0;

