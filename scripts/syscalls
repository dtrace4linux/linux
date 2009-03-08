#! /bin/sh
# Syscalls by function/process
echo "System-calls ordered by frequency. Press Ctrl-C to terminate and view."
set -x
dtrace -n 'syscall:::entry { @num[probefunc] = count(); }'
