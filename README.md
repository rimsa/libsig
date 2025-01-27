# LibSIG

libsig is a valgrind plugin to track when the code crosses boundaries
from a code section, the text section of a program for example, to
somewhere outside of it, a library code for example. Thus, this
can be used as a runtime signature for a program.

## Building

To build libsig, first download and unpack valgrind (3.24.0):

    $ wget -qO - https://sourceware.org/pub/valgrind/valgrind-3.24.0.tar.bz2 | tar jxv

Then, enter directory and clone libsig github repository.
Apply a patch to add the tool in the compilation chain.

    $ cd valgrind-3.24.0
    $ git clone https://github.com/rimsa/libsig.git libsig
    $ patch -p1 < libsig/libsig.patch

Build valgrind with libsig:

    $ ./autogen.sh
    $ ./configure
    $ make -j4
    $ sudo make install

## Testing

Compile and use a test program that orders numbers given in the argument's list:

    $ cd libsig/tests
    $ gcc test.c
    $ valgrind -q --tool=libsig --records=records.csv -- ./a.out 15 4 8 16 42 23
    4 8 15 16 23 42

Since this plugin supports multithreaded programs, the output file contains a commented line
with the thread id followed by the recorded boundaries crosses with its address and how many
times it was crossed consecutively. By default, the plugin considers the text section of the
instrumentation program as the address range to track, but this can be changed by the `--bound`
command line argument.

    $ cat records.csv
    # Thread: 1
    0x4001100,1
    0x4873f90,1
    0x109000,1
    0x4874010,1
    0x1090c0,1
    0x1090d0,6
    0x1090b0,6
    0x1090a0,1
    0x109090,1
    0x4874083,1
    0x109080,1
    0x4011f6b,1

In this example, the addresses `0x1090d0` and `0x1090b0` corresponds, respectively, to the
`atoi` and `printf` libc functions (hint: use the [libsig_symbols](libsig_symbols) script to
find those names). As can be observed, each one of them was called 6 times for this execution.
