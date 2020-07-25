
# gdbstub

GDB Remote Serial Protocol stub in C

`gdbstub` aims to allow quick integration of the GDB Remote Serial Protocol into your project.
It only supports a subset of the features provided by the GDB RSP, with a focus on enabling GDB
support in emulator projects.

Protocol Specification:
https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html

## Building

gdbstub is a header only library. To use it, simply copy `gdbstub.h` to your project, or add it to your include path.

You may also install it using cmake, like so:

```
cmake path/to/source
sudo make install
```

This will install both CMake and pkg-config configuration files.

## Using

To use gdbstub in your program, put the following in a single source file:

```c
// If you would like gdbstub to print debug messages, you can optionally include this next line
#define GDBSTUB_DEBUG

#define GDBSTUB_IMPLEMENTATION
#include <gdbstub.h>
```

## Example

There is a full example file in `examples/example.c`
