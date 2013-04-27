#! /usr/bin/perl

# $Header: Last edited: 29-Oct-2011 1.3 $ 

# 20090415 PDF Fix for when we are using the optional symbols (AS4)
# 20090416 PDF Add /boot/System.map support
# 20090416 PDF Add -unhandled switch support.
# 20090513 PDF Add copy of /etc/dtrace.conf to /dev/dtrace.
# 20090605 PDF Add -opcodes support for fbt provider.
# 20090718 PDF Add System.map26 support for Arch linux.
# 20111014 PDF Read /proc/kallsyms as root to avoid permission issue.
# 20111028 PDF Add -panic switch so I can maybe figure out why we are dying.
# 20120711 PDF Make /dev/fasttrap world read/writable.
# 20120822 PDF Remove some unneeded symbols (arch linux fix)

# Simple script to load the driver and get it ready.

use strict;
use warnings;

use File::Basename;
use FileHandle;
use Getopt::Long;
use IO::File;
use POSIX;
use Cwd;

my $SUDO = "setuid root";

#######################################################################
#   Command line switches.					      #
#######################################################################
my %opts = (
	here => 0,
	mem_alloc => 0,
	opcodes => 0,
	opcodes2 => 0,
	panic => 0,
	printk => 0,
	unhandled => 0,
	v => 0,
	);

sub main
{
	Getopt::Long::Configure('no_ignore_case');
	usage() unless GetOptions(\%opts,
		'fast',
		'help',
		'here',
		'mem_alloc',
		'opcodes',
		'opcodes2',
		'panic',
		'printk',
		'unhandled',
		'unload',
		'v+',
		);

	usage() if ($opts{help});
	if (! -f "$ENV{HOME}/bin/setuid" ) {
		$SUDO = "sudo";
		if (! -f "/usr/bin/sudo" && ! -f "/usr/sbin/sudo") {
			$SUDO = "";
		}
	}
	if (getuid() == 0) {
		$SUDO = "";
	}
	my $tstart = time();
	print time_string() . "Syncing...\n";
	spawn("sync ; sync");

	###############################################
	#   Get some kernel variables.		      #
	###############################################
	my $uname_m = `uname -m`;
	chomp($uname_m);
	my $kernel = `uname -r`;
	chomp($kernel);

	my $dtrace = "build/dtrace";
	if (! -f $dtrace) {
		###############################################
		#   Find   dtrace   -   we  may  be  in  the  #
		#   build/driver dir or some other place.     #
		###############################################
		my $dtrace_dir = Cwd::cwd();
		while ($dtrace_dir ne '/') {
			last if -f "$dtrace_dir/dtrace";
			$dtrace_dir = dirname($dtrace_dir);
		}
		$dtrace = "$dtrace_dir/dtrace";
		die "Cannot locate dtrace command" if ! -f $dtrace;
	}

	###############################################
	#   Safely remove the old driver.	      #
	###############################################
	if ( -e "/dev/dtrace" ) {
		my $rmmod;
		foreach my $p ("/sbin", "/usr/sbin", "/usr/bin") {
			my $p1 = "$p/rmmod";
			$rmmod = $p1 if -x $p1;
		}
		if (!$rmmod) {
			print "ERROR: Strange..cannot locate 'rmmod'\n";
			exit(0);
		}
		spawn("$SUDO $rmmod dtracedrv");
		spawn("sync ; sync");
		exit(0) if $opts{unload};
	}

	###############################################
	#   We  want  get_proc_addr() to run as soon  #
	#   as  possible,  so  bootstrap  the symbol  #
	#   finder.				      #
	###############################################
	my $kallsyms_lookup_name = `$SUDO grep -w kallsyms_lookup_name /proc/kallsyms`;
	chomp($kallsyms_lookup_name);
	$kallsyms_lookup_name =~ s/ .*$//;

	#####################################################################
	#   Here  goes...we could crash the kernel... We sleep for a bit at
	#   present  since we need the /dev entries to appear. Chmod 666 so
	#   we can avoid being root all the time whilst we debug.
	#####################################################################
	my $dtracedrv = "build-$kernel/driver/dtracedrv.ko";
	print time_string() . "Loading: $dtracedrv\n";
	my $opc_len = $opts{opcodes};
	$opc_len = 2 if $opts{opcodes2};
	my $insmod = "/sbin/insmod";
	$insmod = "/usr/bin/insmod" if ! -x $insmod;
	my $ret = spawn("$SUDO $insmod $dtracedrv dtrace_here=$opts{here}" .
		" fbt_name_opcodes=$opc_len" .
		" dtrace_unhandled=$opts{unhandled}" .
		" dtrace_mem_alloc=$opts{mem_alloc}" .
		" dtrace_printk=$opts{printk}" .
		" grab_panic=$opts{panic}" .
		" arg_kallsyms_lookup_name=0x$kallsyms_lookup_name"
		);
	if (! -d "/proc/dtrace") {
		print "/proc/dtrace does not exist. Maybe the driver didnt load properly.\n";
		exit(1);
	}

	if ($ret) {
		my $log = -f "/var/log/messages" ? "/var/log/messages" : 
			-f "/var/log/kern.log" ? "/var/log/kern.log" : "/var/log/kernel.log";
		print "\n";
		print "An error was detected loading the driver. Refer to\n";
		print "$log or 'dmesg' to see what the issue\n";
		print "might be. For your convenience, here is the last few\n";
		print "lines from $log:\n";
		print "\n";
		print "===== tail -10 $log\n";
		system("$SUDO tail -10 $log");
		exit(1);
	}
        my $sectop = "/sys/module/dtracedrv/sections/";
        if ($opts{v} > 1 && -e $sectop . ".text") {
          # print KGDB module load command
          print "# KGDB:\n";
          open(IN, $sectop . ".text");
          my $line = <IN>; chomp($line);
          my $cmd = "add-symbol-file dtracedrv.ko " . $line;
          foreach my $s (".bss", ".eh_frame", ".data", ".rodata",
                         ".init.text", ".exit.text") {
            my $ss = $sectop . $s;
            if (-e $ss && open(IN, $ss)) {
              $line = <IN>; chomp($line);
              $cmd .= " -s $s " . $line;
            }
          }
          print $cmd, "\n";
        }
	sleep(1);

	mkdev("/dev/dtrace");
	mkdev("/dev/dtrace_helper");
	mkdev("/dev/fasttrap");
	mkdev("/dev/fbt");
	
	###############################################
	#   Read  symbols  in  /proc/kallsyms into a  #
	#   hash.  Oh  dear. Recent kernels make the  #
	#   entries  in  /proc/kallsyms not readable  #
	#   if  we are not root (symaddr come out as  #
	#   zero).				      #
	###############################################
	my $fh = new FileHandle("$SUDO cat /proc/kallsyms |");
	die "Cannot proceed - /proc/kallsyms - $!" if !$fh;
	my %syms;
	while (<$fh>) {
		chomp;
		my $s = (split(" ", $_))[2];
		$syms{$s} = $_;
	}

	###############################################
	#   We  need  to know if we are a 32-bit cpu  #
	#   or not.				      #
	###############################################

	###############################################
	#   Just  in  case - read the system map. We  #
	#   try System.map26 for 'arch' linux, since  #
	#   it uses a different naming convention.    #
	###############################################
	my $fname = "/boot/System.map-$kernel";
	$fname = "/boot/System.map26" if ! -f $fname;
	$fh = new FileHandle($fname);
	if ($fh) {
		while (<$fh>) {
			chomp;
			###############################################
			#   Be  careful  in  case  /boot/System file  #
			#   doesnt  agree  with /proc/kallsyms; only  #
			#   on my hacked system.		      #
			###############################################
			my $s = (split(" ", $_))[2];
			$syms{$s} = $_ if !defined($syms{$s});
		}
	}
	###############################################
	#   Need  to  'export'  GPL  symbols  to the  #
	#   driver  so  we can call these functions.  #
	#   Until  this  is  done,  the  driver is a  #
	#   little unsafe(!) Items below labelled as  #
	#   'optional'  are  allowed to be missing -  #
	#   we   only  ever  wanted  them  for  some  #
	#   initial  debugging,  and  dont need them  #
	#   any more.				      #
	#   					      #
	#   Colon  separated entries let us fallback  #
	#   to  an  alternate mechanism to find what  #
	#   we are after (see fbt_linux.c)	      #
	###############################################
	print time_string() . "Preparing symbols...\n";
	my $err = 0;
	# Symbols we used to need, but no longer:
	# get_symbol_offset
	foreach my $s (qw/
		access_process_vm
		kallsyms_addresses:optional
		kallsyms_lookup_name
		modules:print_modules
		sys_call_table:optional
		ia32_sys_call_table:amd64
		syscall_call:optional
		xtime:optional
		__module_text_address
		add_timer_on
		/) {
		my $done = 0;
		my $amd64 = 0;
		my $real_name;
		foreach my $rawsym (split(":", $s)) {
			###############################################
			#   Handle case of ia32_sys_call_table which  #
			#   is only there on a 64-bit kernel.	      #
			###############################################
			if ($rawsym eq 'amd64') {
				$amd64 = 1;
				next;
			}
			###############################################
			#   If  symbol  is optional, we dont care if  #
			#   its not there.			      #
			###############################################
			if ($rawsym eq 'optional') {
				$done = 1;
				last;
			}
			next if !defined($syms{$rawsym});
			$real_name = $rawsym if !$real_name;
			my $addr = $syms{$rawsym};
			$addr = join(" ", (split(" ", $syms{$rawsym}))[0..1]) . " $real_name";

			my $fh = new FileHandle(">/dev/fbt");
			if (!$fh) {
				print "Cannot open /dev/fbt -- $!\n";
				$err++;
				last;
			}
                        if ($opts{v}) {
                        	print STDERR "echo \"$addr\" > /dev/fbt\n";
                        }
                        print $fh $addr . "\n" if $fh;
			$done = 1;
			last;
		}

		###############################################
		#   Handle  the  error  conditions.  We must  #
		#   stop the driver from being usable due to  #
		#   null-ptr derefs for symbols we must have  #
		#   and expect to have.			      #
		###############################################
		next if $done;

		###############################################
		#   Some symbols are cpu specific.	      #
		###############################################
		if ($amd64) { # && $uname_m =~ /64/) {
			next;
		}
		
		print "ERROR: This kernel is missing: '$s'\n";
		$err++;
	}

	if ( "$err" != 0 ) {
		print <<EOF;
 ======================================================
If your kernel is missing one or more symbols, there may be
many reasons, such as kernel not compiled with /proc/kallsyms
support (check that exists, and see if there lots of entries).
A typical kernel will have upwards of 20,000+ entries.

If /proc/kallsyms does not exist, you will not get anywhere,
unless this load module is modified to read the uncompressed
vmlinux file (feel free to report this including the kernel/distro
information).

Run tools/bug.sh to get some information to forward to the
driver maintainer(s).

EOF
		print "Please do not run dtrace - your kernel is not supported.\n";
		exit(1);
	}

	###############################################
	#   Tell driver we have done the init.	      #
	###############################################
	$fh = new FileHandle(">/dev/fbt");
	print "echo \"1 T END_SYMS\" >/dev/fbt\n" if $opts{v};
	print $fh "1 T END_SYMS\n"; # sentinel

	###############################################
	#   Keep a copy of probes to avoid polluting  #
	#   /var/log/messages   as   we   debug  the  #
	#   driver..				      #
	###############################################
	if (!$opts{fast}) {

		print time_string() . "Probes available: ";
		my $pname = "/tmp/probes";
		spawn("$SUDO rm -f /tmp/probes");
#		my $pname = strftime("/tmp/probes-%Y%m%d-%H:%M", localtime);
#		if ( -f "/tmp/probes.current" && ! -f $pname ) {
#			if (!rename("/tmp/probes.current", "/tmp/probes.prev")) {
#				print "rename error /tmp/probes.current -- $!\n";
#			}
#		}
		spawn("$SUDO $dtrace -l | tee $pname | wc -l ");
#		unlink("/tmp/probes.current");
#		if (!symlink($pname, "/tmp/probes.current")) {
#			print "symlink($pname, /tmp/probes.current) error -- $!\n";
#		}
	}

	###############################################
	#   Load   the   security  policy  into  the  #
	#   kernel.				      #
	###############################################
	if ( -f "/etc/dtrace.conf") {
		system("$SUDO sh -c \"cat /etc/dtrace.conf >/dev/dtrace\"");
	}
	print time_string() . "Time: ", time() - $tstart, "s\n";

	###############################################
	#   Defer  setting  up  dtrace_ctl  til  the  #
	#   symtabs are loaded.			      #
	###############################################
	mkdev("/dev/dtrace_ctl");
	
	###############################################
	#   For  my  personal benefit - make sure we  #
	#   have  an upto date symtab when using the  #
	#   vmware/gdb combination.		      #
	###############################################
	if (-f ".copy-kallsyms") {
		system("root cat /proc/kallsyms >/tmp/k");
		system("scp /tmp/k dixxy:/tmp");
	}

}
#####################################################################
#   If  the  /dev entries are not there then maybe we are on an old
#   kernel,   or  distro  (my  old  non-RAMfs  Knoppix  distro  for
#   instance).
#####################################################################
sub mkdev
{	my $dev = shift;

	my $name = basename($dev);
	if ( ! -e "/dev/$name" && -e "/sys/class/misc/$name/dev" ) {
		my $fh = new FileHandle("/sys/class/misc/$name/dev");
		my $dev = <$fh>;
		chomp($dev);
		$dev =~ s/:/ /;
		spawn("$SUDO mknod /dev/$name c $dev");
	}
	spawn("$SUDO chmod 666 /dev/$name");
}
sub spawn
{	my $cmd = shift;

	print time_string() . $cmd, "\n" if $opts{'v'};
	return system($cmd);
}
sub time_string
{
	return strftime("%H:%M:%S ", localtime);
}
#######################################################################
#   Print out command line usage.				      #
#######################################################################
sub usage
{
	print <<EOF;
load.pl: load dtrace driver and prime the links to the kernel.
Usage: load.pl [-v] [-here] [-fast] [-mem_alloc]

Description:

  Loads the dtrace driver into the kernel, prime the symbol tables,
  and load the /etc/dtrace.conf policy file.

Switches:

   -v         Be more verbose about loading process (repeat for more).
   -fast      Dont do 'dtrace -l' after load to avoid kernel messages.
   -here      Enable tracing to /var/log/messages.
   -mem_alloc Trace memory allocs (kmem_alloc/kmem_zalloc/kmem_free).
   -opcodes   Make probes named after x86 instruction opcodes to help
              debugging.
   -opcodes2  Make probes named after first two bytes of opcode.
   -panic     Intercept the panics which can occur. Might be helpful
              for dtrace developer to salvage some autopsy info.
   -printk    Use printk to print to console instead of /proc/dtrace/trace
   -unhandled Log FBT functions we couldnt handle because of unsupported/
              disassembly errors.
   -unload    Unload the dtrace driver.

EOF
	exit(1);
}

main();
0;


