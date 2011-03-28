#! /usr/bin/perl

# $Header:$

# This script is used to extract a working ELF executable from vmlinuz.
# I want to see whats in the kernels in my VMs, so this helps when I
# am dealing with old and legacy distros.

# Author: Paul Fox
# March 2011

use strict;
use warnings;

no warnings 'portable';  # Support for 64-bit ints required
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
		'n',
		);

	usage() if ($opts{help});

	my $fname = shift @ARGV;
	my $system_map = shift @ARGV;

	if (!$fname) {
		$fname = "/boot/vmlinuz-" . `uname -r`;
		chomp($fname);
	}
	if (!$system_map) {
		$system_map = "/boot/System.map-" . `uname -r`;
		chomp($system_map);
	}
	my $fh;
	my $fh1;

	###############################################
	#   Read system.map.			      #
	###############################################
	$fh = new FileHandle($system_map);
	my %syms;
	my %addr;
	while (<$fh>) {
		chomp;
		my ($addr, $type, $name) = split(/ /);
		my $seen = $syms{$name};
		$syms{$name}{type} = $type;
		$syms{$name}{addr} = $addr;
		$addr{$addr} = $name if !$seen;
	}

	my $_text = $syms{_text}{addr};

	print "Reading $fname\n";
	$fh = new FileHandle($fname);
#	$fh->binmode();

	###############################################
	#   Find  the  compressed payload, and write  #
	#   it out uncompressed.		      #
	###############################################
	my $data;
	my $pos = 0;
	do {
		$data = "";
		$fh->seek($pos++, 0);
		$fh->read($data, 4);
	} until ($data eq "\x1f\x8b\x08\x00" || $fh->eof());
	printf "Kernel at offset 0x%x\n", $pos - 1;
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
	print "Generating /tmp/vmlinux.tmp\n";
	$fh = new FileHandle("| gunzip >/tmp/vmlinux.tmp");
	print $fh $data;
	$fh->close();

	###############################################
	#   Now  create  an  assembly file so we can  #
	#   create an ELF file.			      #
	###############################################
	$fh1 = new FileHandle(">/tmp/vmlinux.s");
	print "Generating /tmp/vmlinux.s\n";
#	foreach my $s (sort(keys(%syms))) {
#		print $fh1 ".globl $s\n";
#		if ($syms{$s}{type} =~ /t/i) {
#			print $fh1 "\t.type $s, \@function\n";
#			print $fh1 "\t.size $s, 4\n";
#		} else {
#			print $fh1 "\t.set $s, 0x$syms{$s}{addr}\n";
#		}
#	}
	$fh = new FileHandle("/tmp/vmlinux.tmp");
	print $fh1 "\t.text\n";
	print $fh1 "linuxstart:\n";
	print $fh1 "_start:\n";
	print $fh1 "\t .globl _start\n";
	my $a = hex($_text);
	printf "_start=%x\n", $a;
	while (!$fh->eof()) {
		$data = "";
		$fh->read($data, 1);
		last if !defined($data);
		my $astr = sprintf("%x", $a);
		if (defined($addr{$astr})) {
			print $fh1 ".globl $addr{$astr}\n";
			print $fh1 "\t.type $addr{$astr}, \@function\n";
			print $fh1 "$addr{$astr}:\n";
		}
		printf $fh1 "\t.byte 0x%x\n", ord($data);
		$a += 1;
	}
	print $fh1 "\t.size linuxstart, .-linuxstart\n";

	###############################################
	#   Create linker script.		      #
	###############################################
	print "Generating /tmp/vmlinux.ld\n";
	$fh1 = new FileHandle(">/tmp/vmlinux.ld");
	print $fh1 "SECTIONS {\n";
	print $fh1 "  .text 0x$_text : { *(.text) }\n";
	print $fh1 "  .data 0 : { *(.data) }\n";
	print $fh1 "  .bss  : { *(.bss) *(COMMON) }\n";
	print $fh1 "}\n";
	$fh1->close();

	###############################################
	#   Now build the new ELF file.		      #
	###############################################
	spawn("cd /tmp ; gcc -c vmlinux.s");
	spawn("cd /tmp ; ld vmlinux.ld -o v vmlinux.o");

	###############################################
	#   This  extracts  the boot block code - we  #
	#   dont want/care about this.		      #
	###############################################
#	$fh = new FileHandle("/tmp/vmlinux.tmp");
#	my $fh1 = new FileHandle(">/tmp/vmlinux");
#	print "Generating /tmp/vmlinux\n";
#	while (1) {
#		$data = "";
#		$fh->read($data, 4);
#		if ($data eq "\x7fELF") {
#			print $fh1 $data;
#			while (!$fh->eof()) {
#				$data = "";
#				$fh->read($data, 4096);
#				print $fh1 $data;
#			}
#			last;
#		}
#	}
}

sub spawn
{	my $cmd = shift;

	print $cmd, "\n";
	return if $opts{n};
	return system($cmd);
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
get-vmlinux.pl -- tool to extract kernel from a vmlinuz file
Usage: get-vmlinux.pl [vmlinuz] [System.map]

  This tool is used to extract the kernel image from a boot up 
  vmlinuz file. It helps to have the System.map file for the
  image, so that the symbol table can be populated.

  The goal of this exercise is to allow browsing a distro kernel
  when the original kernel source/object files are not available.

Switches:

EOF

	exit(1);
}

main();
0;

