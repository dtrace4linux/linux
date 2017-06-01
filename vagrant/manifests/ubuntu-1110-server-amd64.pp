group { "puppet":
  ensure => "present",
}

File { owner => 0, group => 0, mode => 0644 }

file { '/etc/motd':
  content => "DTrace4Linux (https://github.com/dtrace4linux/linux) development environment virtual machine\ncreated by Marc Abramowitz <marc@marc-abramowitz.com>\nUbuntu 11.10 (\"Oneiric\")\n\n",
}

file { '/etc/apt/apt.conf.d/71AssumeYes':
  content => "APT {\n  Get {\n    Assume-Yes \"true\";\n  };\n};\n",
}

exec { 'git_clone':
  require   => Package['git-core'],
  user      => 'vagrant',
  cwd       => '/home/vagrant',
  command   => '/usr/bin/git clone --quiet --branch get-deps-kernel-headers https://github.com/msabramo/linux.git dtrace4linux',
  logoutput => "on_failure",
}

exec { 'get_deps':
  require     => [ File['/etc/apt/apt.conf.d/71AssumeYes'], Exec['git_clone'], ],
  user        => 'vagrant',
  environment => 'DEBIAN_FRONTEND=noninteractive',
  cwd         => '/home/vagrant/dtrace4linux',
  command     => '/home/vagrant/dtrace4linux/tools/get-deps.pl',
  timeout     => 600,
  logoutput   => "on_failure",
}

exec { 'make':
  require   => Exec['get_deps'],
  cwd       => '/home/vagrant/dtrace4linux',
  command   => '/usr/bin/make all && \
                /usr/bin/make install && \
                /usr/bin/make load && \
                /sbin/lsmod | /bin/grep dtracedrv',
  logoutput => "on_failure",
}

exec { 'test':
  require => Exec['make'],
  command => '/sbin/lsmod | /bin/grep dtracedrv; /usr/sbin/dtrace -l | /usr/bin/wc -l',
  logoutput => true,
}

package { "git-core":                     ensure => present }
package { "linux-headers-$kernelrelease": ensure => present }
