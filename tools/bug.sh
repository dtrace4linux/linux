#! /bin/sh

if [ "$BUG_RUNNING" = 1 ]; then
        exit 1
fi
if [ "$BUG_NOBUG" != "" -o -f .dtrace.nobug ]; then
        exit 1
fi

export BUG_RUNNING=1
file=/tmp/dtrace-bug.$$.txt
cat <<EOF | tee $file
======================
== Sorry - but dtrace failed to compile on your system.
== Please forward the following file to:
==
== file: $file
== mail: Crisp.Editor@gmail.com
== mail: dtrace@crisp.demon.co.uk
==
== and the information provided will be used to help
== enhance the tool and fix the underlying issue.
==
== Latest news and blog updates on dtrace available here. Please
== check for latest problem reports.
==
== http://crtags.blogspot.com
== http://www.crisp.demon.co.uk/blog/
==
== Latest downloads available from here:
==
== ftp://crisp.dyndns-server.com/pub/release/website/dtrace
== ======================
(generating a make run - this may take a few moments...)
EOF

(
cat .release
pwd
date
echo "\$ uname -a:" ; uname -a
for f in /etc/lsb-release /etc/redhat-release
do
	if [ -f $f ]; then
		echo "\$ cat $f" ; cat $f
	fi
done
echo "\$ cat /etc/motd" ; cat /etc/motd
echo "\$ cat build/port.h" ; cat build/port.h
echo "\$ gcc -v" ; gcc -v
echo "\$ ld -v" ; ld -v
flex --version
bison --version
cat /proc/cpuinfo | grep processor
grep model /proc/cpuinfo | sort -u
echo =========================
make all
) >$file 2>&1
touch .dtrace.nobug
exit 1
