# libSAVL

Simple AVL tree library

## Introduction

libSAVL is a simple, lightweight, low-level
[AVL tree](https://en.wikipedia.org/wiki/AVL_tree) implementation for use in C
programs.

## Building and installing the library

### RPM-based systems

On RPM-based systems (Fedora, Red Hat Enterprise Linux, CentOS, etc.), the
recommended way to install the library is to

```
gcc -O3 -Wall -Wextra -shared -fPIC -Wl,-soname,libsavl.so.0.0 -o libsavl.so.0.0.0 savl.c
```

## Install the library

```
sudo cp libsavl.so.0.0.0 /usr/local/lib64/
sudo ldconfig /usr/local/lib64
```

## [Optional] Install the development files

```
sudo ln -s libsavl.so.0.0.0 /usr/local/lib64/libsavl.so

```

(The last step is only needed on systems that will

## [Optional] Build the test program

```
gcc -O3 -Wall -Wextra -o avl test.c -lsavl
```
