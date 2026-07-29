linux_CFLAGS=-pipe
linux_CXXFLAGS=$(linux_CFLAGS)

linux_release_CFLAGS=-O2
linux_release_CXXFLAGS=$(linux_release_CFLAGS)

linux_debug_CFLAGS=-O1
linux_debug_CXXFLAGS=$(linux_debug_CFLAGS)

linux_debug_CPPFLAGS=-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

ifeq (86,$(findstring 86,$(build_arch)))
i686_linux_CC=gcc -m32
i686_linux_CXX=g++ -m32
i686_linux_AR=ar
i686_linux_RANLIB=ranlib
i686_linux_NM=nm
i686_linux_STRIP=strip

x86_64_linux_CC=gcc -m64
x86_64_linux_CXX=g++ -m64
x86_64_linux_AR=ar
x86_64_linux_RANLIB=ranlib
x86_64_linux_NM=nm
x86_64_linux_STRIP=strip
else
i686_linux_CC=$(default_host_CC) -m32
i686_linux_CXX=$(default_host_CXX) -m32
x86_64_linux_CC=$(default_host_CC) -m64
x86_64_linux_CXX=$(default_host_CXX) -m64
endif

# Native aarch64/arm64: depends always sets host_toolchain=$(HOST)- even for
# native builds, so without these overrides boost/b2 looks for
# aarch64-unknown-linux-gnu-g++ which does not exist on the runner.
ifeq ($(build_arch),aarch64)
aarch64_linux_CC=gcc
aarch64_linux_CXX=g++
aarch64_linux_AR=ar
aarch64_linux_RANLIB=ranlib
aarch64_linux_NM=nm
aarch64_linux_STRIP=strip
endif
ifeq ($(build_arch),arm64)
arm64_linux_CC=gcc
arm64_linux_CXX=g++
arm64_linux_AR=ar
arm64_linux_RANLIB=ranlib
arm64_linux_NM=nm
arm64_linux_STRIP=strip
endif
