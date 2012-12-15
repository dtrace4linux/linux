#! /usr/bin/perl

# $Header: Last edited: 07-Oct-2011 1.4 $ 
#
# Script to poke around the kernel to see what files or structs are
# available that we really want. Usually there in a later kernel, but
# may not be in an earlier one.
#
# 27-Jun-2009 PDF Add support for HAVE_STACKTRACE_OPS
# 24-Jul-2009 PDF Add support for FUNC_SMP_CALL_FUNCTION_SINGLE_5_ARGS
# 06-Jul-2010 PDF Add 'nonatomic' as a way to autodetect FUNC_SMP_CALL_FUNCTION_SINGLE_5_ARGS on Centos 5.5
# 15-Jul-2010 PDF Better parsing for SMP_CALL_FUNCTION_SINGLE_ARGS and SMP_CALL_FUNCTION_ARGS
# 20-Mar-2011 PDF Add better HAVE_ELF_GETSHDRSTRNDX detection for FC-14.
# 01-Feb-2012 PDF Handle ebx vs. bx register name for i386
# 11-Mar-2012 PDF Default HAVE_EBX_REGISTER for <=2.6.21 kernels.
# 23-Aug-2012 PDF Add support for PTRACE_O_TRACEEXEC
# 10-Oct-2012 PDF Add support for HAVE_LINUX_MIGRATE_H
# 26-Oct-2012 PDF Add support for HAVE_LINUX_FDTABLE_H
# 15-Dec-2012 CJD Search in kernel source file for stacktrace.h since Debian systems put it there.

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;

my $kern;
my $kern_src;
my $build;

my $username = getpwuid(getuid());

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts;

sub main
{
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'help',
		'v',
		);

	usage() if ($opts{help});
	usage() if !$ENV{BUILD_DIR};

	my $fh;
	my $fname = "$ENV{BUILD_DIR}/port.h";
	$build = $ENV{BUILD_DIR};
	$build =~ s/^build-//;
	my $inc = "";
	$kern = "/lib/modules/$build/build";
	$kern_src = "/lib/modules/$build/source";
	$kern_src = $kern if ! -d $kern_src;
	$kern_src = "/usr/src/kernels/$build" if ! -d $kern_src;
	foreach my $f (qw{
		include/linux/cred.h
		include/linux/slab.h
		}) {
		my $val = 0;
		if (-f "$kern/$f") {
			$val = 1;
		}
#		print "$val $kern/$f\n";
		my $name = $f;
		$name =~ s/[\/.]/_/g;
		$name = uc($name);
		$inc .= "# define HAVE_$name $val\n";
	}

	###############################################
	#   Generate notifiers from /proc/kallsyms.   #
	###############################################
	$fh = new FileHandle("grep ' [bd] .*notifier' /proc/kallsyms |");
	my $ofh = new FileHandle(">build-$build/notifiers.h");
	while (<$fh>) {
		chomp;
		my ($addr, $type, $name) = split(" ");
		print $ofh "{\"\", \"$name\"},\n";
	}
	$ofh->close();

	###############################################
	#   Annoying lex differences.		      #
	###############################################
	my $lex = check_lex_yytext();
	if ($lex) {
		$inc .= "# define $lex 1\n";
	}

	###############################################
	#   Look  for  whether  we  are bx or ebx in  #
	#   i386/ptrace.h.			      #
	###############################################
	$inc .= check_bx_vs_ebx();

	###############################################
	#   Check out <sys/ptrace.h>		      #
	###############################################
	$inc .= check_ptrace_h();

	###############################################
	#   Handle  differing  naming conventions of  #
	#   the per-cpu data structures.	      #
	###############################################
	if (have("per_cpu__cpu_number", "/proc/kallsyms")) {
		$inc .= "# define HAVE_PER_CPU__CPU_NUMBER 1\n";
	}
	if (have("atomic_notifier_chain_register", "/proc/kallsyms")) {
		$inc .= "# define HAVE_ATOMIC_NOTIFIER_CHAIN_REGISTER 1\n";
	}
	if (have(" cpu_number", "/proc/kallsyms")) {
		$inc .= "# define HAVE_CPU_NUMBER 1\n";
	}
	if (have("pda_cpunumber", "include/asm/offset.h")) {
		$inc .= "# define HAVE_INCLUDE_ASM_OFFSET_H 1\n";
		$inc .= "# define HAVE_PDA_CPUNUMBER 1\n";
	}

	###############################################
	#   Avoid  complexity  in  intr.c  for older  #
	#   i386 kernels.			      #
	###############################################
	if (have("vmalloc_sync_all", "/proc/kallsyms")) {
		$inc .= "# define HAVE_VMALLOC_SYNC_ALL 1\n";
	}

	###############################################
	#   Check  for  problems  with libbfd having  #
	#   cplus_demangle()   function   needed  by  #
	#   ctfconvert/dwarf.c.			      #
	###############################################
	if (have_cplus_demangle()) {
		$inc .= "# define HAVE_CPLUS_DEMANGLE 1\n";
	}

	###############################################
	#   Old kernels have ioctl() only, but later  #
	#   kernels    have   unlocked_ioctl()   and  #
	#   compat_ioctl(). Detect this here.	      #
	###############################################
	my $fs_h = "/lib/modules/$build/build/include/linux/fs.h";
	$fs_h = "$kern_src/include/linux/fs.h" if ! -f $fs_h;
	my $str = `grep "(.ioctl) (struct inode" $fs_h`;
	chomp($str);
	if ($str ne '') {
		$inc .= "# define HAVE_OLD_IOCTL 1\n";
	}
	
	###############################################
	#   Some   versions   of   elf   dont   have  #
	#   elf_getshdrstrndx().   But  then  again,  #
	#   some versions are brokenly buggy and its  #
	#   annoying  me ! I am looking at you FC-14  #
	#   where  there is a single function mapped  #
	#   to  the  same  code, but they dont agree  #
	#   with the API for the return value.	      #
	###############################################
	foreach my $elf ("/usr/lib/libelf.so", "/usr/lib64/libelf.so") {
		my $ret = system(" objdump -T $elf 2>/dev/null | grep elf_getshdrstrndx >/dev/null");
		if ($ret == 0) {
			$inc .= "# define HAVE_ELF_GETSHDRSTRNDX 1\n";
			last;
		}
	}

	###############################################
	#   Check for zlib functions in the kernel.   #
	###############################################
	$str = `grep zlib /proc/kallsyms`;
	chomp($str);
	if ($str eq '') {
		$inc .= "# define DO_NOT_HAVE_ZLIB_IN_KERNEL 1\n";
	}

	###############################################
	#   Check for stack trace, used by stack().   #
	###############################################
        my $stacktrace_fh = have("stacktrace_ops", "include/asm/stacktrace.h") ||
		have("stacktrace_ops", "arch/x86/include/asm/stacktrace.h", 1);
	if ($stacktrace_fh) {
		$inc .= "# define HAVE_STACKTRACE_OPS \n";
		$inc .= find_dump_trace_args($stacktrace_fh);
	}

	###############################################
	#   Check for ELF_C_READ_MMAP		      #
	###############################################
	$str = `grep ELF_C_READ_MMAP /usr/include/libelf.h`;
	chomp($str);
	if ($str ne '') {
		$inc .= "# define HAVE_ELF_C_READ_MMAP 1\n";
	}

	###############################################
	#   Check    for    smp_call_function_single  #
	#   having 4 or 5 args.			      #
	###############################################
	$inc .= smp_call_function_single();

	###############################################
	#   Handle kernel compile flags.	      #
	###############################################
	my @kinc = (
		"include/linux/fdtable.h:HAVE_LINUX_FDTABLE_H",
		"include/linux/migrate.h:HAVE_LINUX_MIGRATE_H",
		"include/asm/kdebug.h:HAVE_INCLUDE_ASM_KDEBUG_H",
		"include/asm/semaphore.h:HAVE_INCLUDE_ASM_SEMAPHORE_H",
		"include/asm/stacktrace.h:HAVE_INCLUDE_ASM_STACKTRACE_H",
		"include/linux/asm/msr-index.h:HAVE_INCLUDE_ASM_MSR_INDEX_H",
		"include/linux/hrtimer.h:HAVE_INCLUDE_LINUX_HRTIMER_H",
		"include/linux/kdebug.h:HAVE_INCLUDE_LINUX_KDEBUG_H",
		"include/linux/mutex.h:HAVE_INCLUDE_LINUX_MUTEX_H",
		"include/linux/semaphore.h:HAVE_INCLUDE_LINUX_SEMAPHORE_H",
		);
	foreach my $k (@kinc) {
		my ($file, $name) = split(":", $k);
#print "$kern/$file\n";
#print "$kern_src/$file\n";
		if (-f "$kern/$file" || -f "$kern_src/$file") {
			$inc .= "# define $name 1\n";
		}
	}
	###############################################
	#   Read old file.			      #
	###############################################
	$inc = join("\n", sort(split("\n", $inc)));
	$inc = "/* This file is automatically generated via mkport.pl */\n" .
		"/* kern=$kern */\n" .
		"/* kern_src=$kern_src */\n" .
		"\n" .
		$inc . "\n";
	print $inc if $opts{v};
	$fh = new FileHandle($fname);
	my $old = "";
	if ($fh) {
		while (<$fh>) {
			$old .= $_;
		}
		exit(0) if $old eq $inc;
	}

	if (! -d "$ENV{BUILD_DIR}") {
		mkdir("$ENV{BUILD_DIR}", 0755);
	}
	$fh = new FileHandle(">$ENV{BUILD_DIR}/port.h");
	die "Cannot create $ENV{BUILD_DIR}/port.h" if !$fh;
	print $fh $inc;

}

######################################################################
#   Seems  to  be some confusion on BX vs EBX register for i386. In  #
#   the  middle  of  the  2.6.2x  series,  it seemed to temporarily  #
#   change  name.  Probably  at  the point the i386 got merged into  #
#   asm-x86.							     #
######################################################################
sub check_bx_vs_ebx
{
	my $file;

	return "# define HAVE_EBX_REGISTER 1\n" if $build le "2.6.21";

	$file = "$kern/include/asm-i386/ptrace.h";
	return "# define HAVE_BX_REGISTER 1\n" if -f $file;
	$file = "$kern/include/asm-x86/ptrace.h";
	return "# define HAVE_BX_REGISTER 1\n" if ! -f $file;

	my $fh = new FileHandle($file);
	while (<$fh>) {
		if (/\<bx;/) {
			return "# define HAVE_BX_REGISTER 1\n";
		}
	}
	return "# define HAVE_EBX_REGISTER 1\n";
}
######################################################################
#   Determine  the  type of yytext so that libdtrace/dt_lex.l works  #
#   properly.							     #
######################################################################
sub check_lex_yytext
{
	my $fh = new FileHandle(">/tmp/$username-lex.l");
	if (!$fh) {
		print "Cannot create /tmp/$username-lex.l -- $!\n";
		print "Please check the permissions on the file.\n";
		exit(1);
	}
	print $fh "%%\n";
	print $fh "%%\n";
	$fh->close();
	system("cd /tmp ; lex /tmp/$username-lex.l");
	
	$fh = new FileHandle("/tmp/lex.yy.c");
	if (!$fh) {
		print "Warning: cannot determine style of lex/yytext.\n";
		return;
	}
	my $ret = "";
	while (<$fh>) {
		if (/^extern char \*yytext;/) {
			$ret = "HAVE_LEX_YYTEXT_PTR";
			last;
		}
		if (/^extern char yytext\[\];/) {
			$ret = "HAVE_LEX_YYTEXT_ARRAY";
			last;
		}
	}
	$fh->close();
	remove("/tmp/lex.yy.c");
	return $ret;
}

######################################################################
#   Definition  of PTRACE_O_TRACEEXEC is an enum but the header may  #
#   disagree with the kernel, so handle that here.		     #
######################################################################
sub check_ptrace_h
{
	my $fh = new FileHandle("/usr/include/sys/ptrace.h");
	return "" if !$fh;
	while (<$fh>) {
		return "" if /PTRACE_O_TRACEEXEC/;
	}
	return "# define PTRACE_O_TRACEEXEC 0x10\n";
}
######################################################################
#   The dump_trace() function for dumping a stack via callbacks has  #
#   either  5  or  6 arguments, but what it has is not dependent on  #
#   the   X.Y.Z   kernel   release,  so  we  need  to  handle  this  #
#   dynamically.						     #
######################################################################
sub find_dump_trace_args
{
	my $fh = shift;
	return "# define FUNC_DUMP_TRACE_ARGS 5\n" if !$fh;
	while (<$fh>) {
		next if !/^void\s+dump_trace/;
		my $str = $_;
		while (<$fh>) {
			$str .= $_;
			last if /\);/;
		}
		$str =~ s/[^,]//g;
		return "# define FUNC_DUMP_TRACE_ARGS " . (length($str) + 1) . "\n";
	}
	return "";
}
######################################################################
#   Grep a file to see if something is where we want it.	     #
#   Return the file handle to the file if it exists.		     #
######################################################################
sub have
{	my $name = shift;
	my $file = shift;
        my $look_in_src = shift;

	my @search_in;
	if ($file =~ /^\//) {
		push(@search_in, '');
	} else {
		push(@search_in, $kern);
		if ($look_in_src) {
			push(@search_in, $kern_src);
		}
        }

        FILE: foreach my $path (@search_in) {
                my $f = "$path/$file";
#print "opening .... $f\n";
		my $fh = new FileHandle($f);
		if (!$fh && $opts{v}) {
			print "Cannot open $f\n";
		}
		next FILE if (!$fh);
		while (<$fh>) {
			return $fh if /$name/;
		}
	}
}
######################################################################
#   Some  libbfd libraries dont have cplus_demangle, so detect that  #
#   here.							     #
######################################################################
sub have_cplus_demangle
{
	my $fh = new FileHandle(">/tmp/demangle.c");
	if (!$fh) {
		print "Cannot create /tmp/demangle.c - please remove it and try again.\n";
		print "Error: $!\n";
		exit(1);
	}
	print $fh <<EOF;
int main(int argc, char **argv)
{
	return cplus_demangle("", 0);
}
EOF
	$fh->close();
	system("gcc -o /tmp/$ENV{USER}.demangle /tmp/demangle.c -lbfd");
	return -f "/tmp/$ENV{USER}.demangle";
}

######################################################################
#   Handle  funkyness  of smp_call_function_single - may/may not be  #
#   there, may have differing arguments.			     #
######################################################################
sub smp_call_function_single
{
	my $inc = "";

	my %hash;

	foreach my $smp_h ("asm/smp.h", "linux/smp.h") {
#print "opening ... $kern/include/$smp_h\n";
		my $fh = new FileHandle("$kern/include/$smp_h");
		$fh = new FileHandle("$kern_src/include/$smp_h") if !$fh;
		next if !$fh;
		while (<$fh>) {
			next if /^#define/;
			if (/smp_call_function_single/) {
				my $line = <$fh>;
				###############################################
				#   Assume   both   smp_call_function()  and  #
				#   smp_call_function_single()     can    be  #
				#   handled with the same 'extra' parameter.  #
				#   Handles AS4.0 kernels.		      #
				###############################################
				if ($line =~ /retry|nonatomic/) {
					$hash{SMP_CALL_FUNCTION_SINGLE_ARGS} = 5;
				} else {
					$hash{SMP_CALL_FUNCTION_SINGLE_ARGS} = 4;
				}
				next;
			}
			if (/int\s+smp_call_function\(/) {
				chomp;
				$_ =~ s/[^,]//g;
				$hash{SMP_CALL_FUNCTION_ARGS} = length($_) + 1;
				next;
			}

		}
	}
	if (scalar(keys(%hash)) == 0) {
		###############################################
		#   Around  2.6.18,  they  went  from 5 to 4  #
		#   args. So lets default to 5.		      #
		###############################################
		return 
			"# define FUNC_SMP_CALL_FUNCTION_SINGLE_MISSING 1\n" .
			"# define SMP_CALL_FUNCTION_SINGLE_ARGS 51\n";
	}
	foreach my $k (sort(keys(%hash))) {
		$inc .= "# define $k $hash{$k}\n";
	}
	return $inc;
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
mkport.pl: script to set up portability workarounds
Usage: BUILD_DIR=x.y.z mkport.pl

This script setups a file called "port.h" which is needed to determine
which features or include files are available on this kernel, so we can
detect at compile time what we need to work around.

Switches:

EOF

	exit(1);
}

main();
0;

