#! /usr/bin/perl

# $Header: Last edited: 23-Jun-2009 1.1 $ 

# 23-Jun-2009 PDF Add check for bison/yacc/flex on the system.

# Script to do a 'make all'

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
		'make=s',
		);

	usage() if ($opts{help});

	my $make_cmd = $opts{make} || "all0";

	###############################################
	#   Look for the tools we are going to need.  #
	###############################################
	find_binary("bison", "yacc");
	find_binary("flex");

	###############################################
	#   Create  the  build  directory.  Dont let  #
	#   crisp   see   the  build  directory  for  #
	#   tagging purposes.			      #
	###############################################
	my $build_dir = shift @ARGV;
	my $uname_m = shift @ARGV;
	usage() if !$uname_m;
	$uname_m = $ENV{BUILD_ARCH} if defined($ENV{BUILD_ARCH});

	$ENV{BUILD_DIR} = $build_dir;
	die "\$BUILD_DIR not set. Please use 'make all'\n" if !$build_dir;
	if (! -d $build_dir ) {
		mkdir($build_dir, 0755) || die "Cannot mkdir $build_dir -- $!";
		my $fh = new FileHandle(">$build_dir/.notags");
	}
	if (! -d "$build_dir/driver" ) {
		mkdir("$build_dir/driver", 0755) || die "Cannot mkdir $build_dir/driver -- $!";
	}

	if (! -f "/usr/include/gelf.h") {
		print "Error: you dont appear to have /usr/include/elf.h, which means\n";
		print "compilation will fail. You should add the libelf-dev package to\n";
		print "your system and retry the 'make all'.\n";
		exit(1);
	}

	###############################################
	#   Ensure build is a symlink to the current  #
	#   build  area  (normally  a  native kernel  #
	#   build, but might not be.		      #
	###############################################
	unlink("build");
	symlink($build_dir, "build") || die "symlink($build_dir, build) failed -- $!";

	###############################################
	#   Create a config file for extra env vars.  #
	###############################################
	my $fh = new FileHandle(">$build_dir/config.sh");
	die "Cannot create $build_dir/config.sh -- $!" if !$fh;

	###############################################
	#   Some precursors to check us out.	      #
	###############################################
	spawn("tools/check_dep.pl");
	spawn("tools/mkport.pl");
	spawn("tools/libgcc.pl");

	if ($uname_m =~ /x86.*64/) {
		spawn("tools/mksyscall.pl x86-64");
		print $fh "export CPU_BITS=64\n";
	} elsif ($uname_m =~ /i.*86/) {
		$ENV{BUILD_i386} = "1";
		$ENV{CPU_BITS} = "32";
		$ENV{PTR32} = "-D_ILP32 -D_LONGLONG_TYPE";
	  	print $fh "export PTR32=\"$ENV{PTR32}\"\n";
		print $fh "export BUILD_i386=$ENV{BUILD_i386}\n";
		print $fh "export CPU_BITS=$ENV{CPU_BITS}\n";
		spawn("tools/mksyscall.pl x86");
	} else {
		die "Unsupported cpu architecture: $uname_m\n";
	}
	$fh->close();
	my $ret = spawn("make $make_cmd", 0);
	if ($ret) {
		spawn("tools/bug.sh") if !$ENV{MAKE_KERNELS};
		exit(1);
	}
}
sub find_binary
{	my @bins = @_;

	foreach my $p (split(":", $ENV{PATH})) {
		foreach my $b (@bins) {
			return 1 if -f "$p/$b";
		}
	}
	print "Sorry - but I cannot find " . join(" or ", @bins) . " on your system.\n";
	print "You may need to install more packages. See tools/get-deps.pl\n";
	print "for a script to semi-automate this for you.\n";
	print "Continue ? [y/n] ";
	my $ans = <STDIN>;
	exit(1) if $ans !~ /^y/;
	return 0;
}
sub spawn
{	my $cmd = shift;
	my $no_exit = shift;

	print $cmd, "\n";
	my $ret = system($cmd);
	exit(1) if $no_exit && !$ret;
	return $ret;
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
build.pl: dtrace build rule
Usage: build.pl \$BUILD_DIR \$UNAME_M

Switches:

EOF

	exit(1);
}

main();
0;

