#! /usr/bin/perl

# $Header:$

# Bits 0-54  page frame number (PFN) if present
# Bits 0-4   swap type if swapped
# Bits 5-54  swap offset if swapped
# Bits 55-60 page shift (page size = 1&lt;&lt;page shift)
# Bit  61    reserved for future use
# Bit  62    page swapped
# Bit  63    page present

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;

my @kflags = qw/
	LOCKED
	ERROR
	REFERENCED
	UPTODATE
	DIRTY
	LRU
	ACTIVE
	SLAB
	WRITEBACK
	RECLAIM
	BUDDY
	MMAP
	ANON
	SWAPCACHE
	SWAPBACKED
	COMPOUND_HEAD
	COMPOUND_TAIL
	HUGE
	UNEVICTABLE
	HWPOISON
	NOPAGE
	KSM
	/;

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
	my $pgcnt = new FileHandle("/proc/kpagecount");
	my $pgflg = new FileHandle("/proc/kpageflags");

	if (!$pgcnt || !$pgflg) {
		print "Need to be root to run this script, since we need access\n";
		print "to /proc/kpagecount and /proc/kpageflags.\n";
		print "Press <Enter> to continue anyway... ";
		my $ans = <STDIN>;
	}

	foreach my $pid (@ARGV) {
		my $fh = new FileHandle("/proc/$pid/pagemap");
		my $addr = 0;
		while (1) {
			my $b;
			my $t;
			my $flags;
			my $count;

			last if !read($fh, $b, 8);
			my $n = unpack("q", $b);
			next if ($n & (1 << 63)) == 0;
			my $pfn = $n & ((1 << 55) - 1);

			if ($pgcnt) {
				if (!seek($pgcnt, $pfn * 8, 0)) {
					print "seek failed -- $!\n";
					exit(0);
				}
				if (!read($pgcnt, $t, 8)) {
					print "read failed - $! - read $t\n";
					exit(0);
				}
				$count = unpack("q", $t);
			}
			if ($pgflg) {
				seek($pgflg, $pfn * 8, 0);
				read($pgflg, $t, 8);
				$flags = unpack("q", $t);
			}

			printf "%08x %08x ", $addr, $pfn;
			printf "cnt=0x%04x ", $count;
			print $n & (1 << 62) ? "swapped " : "not-swapped ";
			print $n & (1 << 2) ? "user  " : "super ";

			for (my $i = 0; $kflags[$i]; $i++) {
				if ($flags & (1 << $i)) {
					print "$kflags[$i] ";
				}
			}
#			print $n & (1 << 1) ? "RW " : "RO ";
#			print $n & (1 << 5) ? "Acc " : "    ";
#			print $n & (1 << 6) ? "D " : "  ";
			print "\n";
			$addr += 4096;
		}
	}
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
pagemap.pl - print out detailed process page table map
Usage: pagemap.pl <pid> ...

  You may need to be root to run this - as it will use the information
  in /proc/kpageflags and /proc/kpagecount to add details to each page
  in the process.

Links:

    http://www.mjmwired.net/kernel/Documentation/vm/pagemap.txt

Switches:

EOF

	exit(1);
}

main();
0;

