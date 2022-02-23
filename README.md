# libSAVL

Simple AVL tree library

## Introduction

libSAVL is a simple, lightweight, low-level
[AVL tree](https://en.wikipedia.org/wiki/AVL_tree) implementation for use in C
programs.

## Building and installing the library

### RPM-based systems

On RPM-based systems (Fedora, Red Hat Enterprise Linux, CentOS, etc.), the
recommended way to install the library is to build RPMs.  First, download the
`tar.gz` version of the most recent tag from
[this page](https://github.com/ipilcher/libsavl/tags).  Save the file in your
`~/rpmbuild/SOURCES` directory.

The RPMs can now be built "locally," with the `rpmbuild` command.  For example:

```
$ rpmbuild -tb ~/rpmbuild/SOURCES/libsavl-0.7.0.tar.gz
︙
Wrote: /home/pilcher/rpmbuild/RPMS/x86_64/libsavl-devel-0.7.0-3.fc35.x86_64.rpm
Wrote: /home/pilcher/rpmbuild/RPMS/x86_64/libsavl-debuginfo-0.7.0-3.fc35.x86_64.rpm
Wrote: /home/pilcher/rpmbuild/RPMS/x86_64/libsavl-0.7.0-3.fc35.x86_64.rpm
Wrote: /home/pilcher/rpmbuild/RPMS/x86_64/libsavl-debugsource-0.7.0-3.fc35.x86_64.rpm
Wrote: /home/pilcher/rpmbuild/RPMS/x86_64/libsavl-doc-0.7.0-3.fc35.x86_64.rpm
︙
+ exit 0
```

Alternatively, the `mock` command can be used to build the RPMs in an isolated,
clean environment.  (`mock` can also be used to build `i686` packages on
`x86_64` platforms or to build packages for older distributions.  For example,
`mock` can be used to build CentOS 7 packages on Fedora 35.

```
$ cat /etc/fedora-release
Fedora release 35 (Thirty Five)

$ rpmbuild -ts ~/rpmbuild/SOURCES/libsavl-0.7.0.tar.gz
setting SOURCE_DATE_EPOCH=1645488000
Wrote: /home/pilcher/rpmbuild/SRPMS/libsavl-0.7.0-3.fc35.src.rpm

$ mock -r centos-7-x86_64 /home/pilcher/rpmbuild/SRPMS/libsavl-0.7.0-3.fc35.src.rpm
︙
Wrote: /builddir/build/RPMS/libsavl-0.7.0-3.el7.x86_64.rpm
Wrote: /builddir/build/RPMS/libsavl-devel-0.7.0-3.el7.x86_64.rpm
Wrote: /builddir/build/RPMS/libsavl-doc-0.7.0-3.el7.x86_64.rpm
Wrote: /builddir/build/RPMS/libsavl-debuginfo-0.7.0-3.el7.x86_64.rpm
︙
INFO: Done(/home/pilcher/rpmbuild/SRPMS/libsavl-0.7.0-3.fc35.src.rpm) Config(centos-7-x86_64) 0 minutes 6 seconds
INFO: Results and/or logs in: /var/lib/mock/centos-7-x86_64/result
Finish: run
```

Once the packages have been built, install them with the usual package manager,
`yum`, `dnf`, etc.

> **NOTE:** RPM builds are expected to work on Fedora and Red Hat Enterprise
> Linux-derived distributions.  Results on other RPM-based distributions, such
> as openSUSE may vary.

#### Development/debug RPMs

This repository also contains a second SPEC file, `libsavl-dev.spec~` which can
be used to easily build RPMs from the repository working directory.  (The `~`
character at the end of the file name is required for the `rpmbuild -t...` to
work.)  This SPEC file, when used from within the repository directory,
automatically detects the latest tag and uses that tag as the package version.

For example:

```
$ git clone https://github.com/ipilcher/libsavl.git
Cloning into 'libsavl'...
remote: Enumerating objects: 48, done.
remote: Counting objects: 100% (44/44), done.
remote: Compressing objects: 100% (33/33), done.
remote: Total 48 (delta 17), reused 37 (delta 11), pack-reused 4
Receiving objects: 100% (48/48), 58.61 KiB | 1.30 MiB/s, done.
Resolving deltas: 100% (17/17), done.

$ cd libsavl

$ # Make sure Git doesn't change the SPEC file during the build
$ cp libsavl-dev.spec~ libsavl-dev.spec

$ rpmbuild -bb libsavl-dev.spec
︙
+ git checkout v0.7.0
Note: switching to 'v0.7.0'.
︙
+ git checkout main
Previous HEAD position was b0e8c9e Rename devel SPEC file to make rpmbuild -t work
Switched to branch 'main'
Your branch is up to date with 'origin/main'.
︙
Wrote: /home/pilcher/rpmbuild/RPMS/x86_64/libsavl-devel-0.7.0-3.dev.fc35.x86_64.rpm
Wrote: /home/pilcher/rpmbuild/RPMS/x86_64/libsavl-0.7.0-3.dev.fc35.x86_64.rpm
︙
+ exit 0
```

This SPEC file builds the library without optimizations (`-O0`) to make
debugging easier.

### Manually building and installing

If needed, determine the latest Git tag.

```
$ git describe --tags --abbrev=0
v0.7.0
```

The full version of the library (`${VERSION}` below) is the tag without its
leading "v", `0.7.0` in this case.  The "`${SO_VERSION}`" is the full version
without its final component, `0.7` in this case.

Build the library.

```
$ gcc -std=gnu99 -O3 -Wall -Wextra -shared -fPIC \
	-Wl,-soname,libsavl.so.${SO_VERSION} -o libsavl.so.${VERSION} savl.c
```

Copy the library to your distribution's standard location (usually `/usr/lib`
or `/usr/lib64`, refered to as `${LIB_DIR}` below), create the required symbolic
links, and run `ldconfig`.

```
$ sudo cp libsavl.so.${VERSION} ${LIB_DIR}
$ sudo ldconfig
```

If development files are required, install the header file and the unversioned
library link.

```
$ sudo cp savl.h /usr/include/
$ sudo ln -s cp libsavl.so.${VERSION} ${LIB_DIR}/libsavl.so
```

## Using the library

libSAVL's API is deliberately minimal.  Applications using the library are
left to implement any additional abstractions themselves.

The most notable feature of the API is that there is no separate data structure
for tree nodes.  Instead, one or more `savl_node` structures is included
directly in the data structure that is stored in a tree (or multiple trees).
Given a pointer to a node in a particular tree, the `SAVL_NODE_CONTAINER()`
macro can be used to access a pointer to the containing data structure.

See [`examples/ex1.c`](examples/ex1.c) for an example.
