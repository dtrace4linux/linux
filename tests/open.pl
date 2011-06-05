#! /usr/bin/perl

use FileHandle;
use IO::File;
use POSIX;

my $n = 0;
my $err = 0;
my $lnerr = 0;
my $t = time();
while (1) {
	if (time() >= $t + 5) {
		print strftime("%H:%M:%S", localtime), " opens=$n err=$err lnerr=$lnerr\n";
		$t = time();
	}
	my $fh = new FileHandle("/etc/hosts");
	$n++;
	my $str = <$fh>;
	if (!$str) {
		print "error reading line\n";
		$lnerr++;
	}
	next if $fh;
	$err++;
	print strftime("%H:%M:%S", localtime), " $n/$err/$lnerr failed to open /etc/hosts -- $!\n";
}
