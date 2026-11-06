freebsd_CFLAGS=
freebsd_CXXFLAGS=
freebsd_LDFLAGS=

freebsd_release_CFLAGS=-O2
freebsd_release_CXXFLAGS=$(freebsd_release_CFLAGS)

freebsd_debug_CFLAGS=-O1 -g
freebsd_debug_CXXFLAGS=$(freebsd_debug_CFLAGS)

ifeq (86,$(findstring 86,$(build_arch)))
x86_64_freebsd_CC=clang -m64
x86_64_freebsd_CXX=clang++ -m64
x86_64_freebsd_AR=ar
x86_64_freebsd_RANLIB=ranlib
x86_64_freebsd_NM=nm
x86_64_freebsd_STRIP=strip
else
x86_64_freebsd_CC=$(default_host_CC) -m64
x86_64_freebsd_CXX=$(default_host_CXX) -m64
endif

freebsd_cmake_system_name=FreeBSD
