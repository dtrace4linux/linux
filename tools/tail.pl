#! /usr/bin/perl

# $Header:$

use strict;
use warnings;

use FileHandle;
use Getopt::Long;
use POSIX;

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts;

sub main
{
	Getopt::Long::Configure('require_order');
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'grep=s',
		'help',
		);

	usage() if $opts{help};

	$| = 1;
	my %history;

	my $fname = shift @ARGV || "/proc/dtrace/trace";
	my $grep = $opts{grep};
	while (1) {
		my $fh = new FileHandle($fname);
		if (!$fh) {
			print strftime("%Y%m%d %H:%M:%S ", localtime()), "Cannot open $fname -- $!\n";
			sleep(2);
			next;
		}
		while (<$fh>) {
			next if defined($history{$_});

			$history{$_} = 1;
			if ($grep && /$grep/) {
				$_ =~ s/($grep)/\033[31;1m$1\033[0;37m/g;
			}
			print;
		}
		select(undef, undef, undef, 0.5);
	}
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
tail.pl -- like "tail" but understands character devices
Usage: tail.pl [-grep pattern] <filename>

  Do tail -f on a file, but keep retrying on EOF. Normal GNU tail
  doesnt seem to handle /proc/dtrace/trace properly.

Switches:

  -grep pattern     Highlight matching term on lines.

EOF

	exit(1);
}

main();
0;

