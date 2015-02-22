set auto-load safe-path /
set confirm no
set history expansion 1
dir libctf
dir libdtrace
dir libproc
dir liblinux
dir cmd/dtrace
define z
 stepi
 x/i $pc
end
