# Packages we need under Fedora
# yum provides
# yum list
yum install bison crash flex gcc elfutils-devel elfutils-libelf-devel zlib-devel \
	kernel-devel libdwarf libdwarf-devel \
	glibc-devel.i686 binutils-devel

# yumdownloader --source kernel
# yum install rpmdevtools yum-utils
# http://fedoraproject.org/wiki/Building_a_custom_kernel
