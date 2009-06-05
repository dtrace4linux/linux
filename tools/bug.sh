#! /bin/sh

if [ "$BUG_RUNNING" = 1 ]; then
        exit 1
fi
if [ "$BUG_NOBUG" != "" -o -f .dtrace.nobug ]; then
        exit 1
fi

export BUG_RUNNING=1
file=/tmp/dtrace-bug.$$
cat <<EOF | tee $file
======================
== Sorry - but dtrace failed to compile on your system.
== Please forward the following file to:
==
== file: $file
== mail: dtrace@crisp.demon.co.uk
==
== and the information provided will be used to help
== enhance the tool and fix the underlying issue.
==
== Latest news and blog updates on dtrace available here. Please
== check for latest problem reports.
==
== http://www.crisp.demon.co.uk/blog/
==
== Latest downloads available from here:
==
== ftp://crisp.dynalias.com/pub/release/website/dtrace
== ======================
(generating a make run - this may take a few moments...)
EOF

(
cat .release
pwd
date
uname -a
gcc -v
ld -v
flex --version
bison --version
cat /proc/cpuinfo | grep processor
grep model /proc/cpuinfo | sort -u
echo =========================
make all
) >$file 2>&1
exit 1
