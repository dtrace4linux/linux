#! /usr/bin/perl

my $cnt = 0;

while (1) {
	printf("PID: $$ cnt=$cnt\n");
	mkdir("/etc/hosts", 0755); # dont do anything but attempt to.
	sleep(1);
	$cnt++;
}
