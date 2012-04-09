# Packages we need under Fedora
# yum provides
# yum list
yum install \
	binutils-devel \
	bison \
	crash \
	elfutils-devel \
	elfutils-libelf-devel \
	flex \
	gcc \
	glibc-devel \
	glibc-devel.i686 \
	kernel-devel \
	libdwarf \
	libdwarf-devel \
	libgcc.i686 \
	zlib-devel

# Optional:
#  openssh-clients

#binutils-devel
#elfutils-libelf-devel
#kernel-devel
#libdwarf
#libdwarf-devel
#zlib-devel
# yumdownloader --source kernel
# yum install rpmdevtools yum-utils
# http://fedoraproject.org/wiki/Building_a_custom_kernel
