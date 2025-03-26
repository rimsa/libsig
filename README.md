# LibSIG

libsig is a valgrind plugin to track when the code crosses boundaries
from a code section, the text section of a program for example, to
somewhere outside of it, a library code for example. Thus, this
can be used as a runtime signature for a program.

## Building

To build libsig, first download and unpack valgrind (3.24.0).

    $ wget -qO - https://sourceware.org/pub/valgrind/valgrind-3.24.0.tar.bz2 | tar jxv

Then, enter directory and clone libsig github repository.
Apply a patch to add the tool in the compilation chain.

    $ cd valgrind-3.24.0
    $ git clone https://github.com/rimsa/libsig.git libsig
    $ patch -p1 < libsig/libsig.patch

Build valgrind with libsig.

    $ ./autogen.sh
    $ ./configure
    $ make -j4
    $ sudo make install

## Testing

Compile a test program that orders numbers given in the argument's list. libsig works even when all of its symbols are stripped.

    $ cd libsig/tests
    $ gcc -o test test.c
    $ strip -s test

Now use this tool to record the libraries signature of this program.

    $ valgrind -q --tool=libsig --records=records.csv -- ./test 15 4 8 16 42 23
    4 8 15 16 23 42

Since this plugin supports multithreaded programs, the output file contains a commented line
with the thread id followed by the recorded boundaries crosses with its address and how many
times it was crossed consecutively. By default, the plugin considers the text section of the
instrumentation program as the address range to track, but this can be changed by the `--bound`
command line argument.

    $ cat records.csv
    # Thread: 1
    0x4001100
    0x4873f90
    0x109000
    0x4874010
    0x1090c0
    0x1090d0
    0x1090d0
    0x1090d0
    0x1090d0
    0x1090d0
    0x1090d0
    0x1090b0
    0x1090b0
    0x1090b0
    0x1090b0
    0x1090b0
    0x1090b0
    0x1090a0
    0x109090
    0x4874083
    0x109080
    0x4011f6b

To translate addresses to library calls in the PLT section of a program, first
use [libsig_symbols](libsig_symbols) to collect the program's symbols.
Then, use the [libsig_translator](libsig_translator) to translate records file
to symbol names.

    $ libsig_symbols test > test.symbols
    $ libsig_translator test.symbols records.csv
    # Thread: 1
    0x4001100
    0x4873f90
    .init
    0x4874010
    malloc@plt
    atoi@plt
    atoi@plt
    atoi@plt
    atoi@plt
    atoi@plt
    atoi@plt
    printf@plt
    printf@plt
    printf@plt
    printf@plt
    printf@plt
    printf@plt
    putchar@plt
    free@plt
    0x4874083
    __cxa_finalize@plt
    0x4011f6b

In this example, we can see that the test program calls `malloc` and `free` once, converts strings to numbers via `atoi` 6 times, prints the numbers using `printf` 6 times and ends with a call to `putchar`
to print the end of line.
