# josh build - C build scripts without an agenda

Many existing build systems have issues that impose problems onto the engineer that is just trying to get shit done.
Usually there is a combination of:
 * adopting a new programming language
 * learning some insurmountable API, the documentation for which could fill entire libraries
 * downloading the build system program to every machine that you must build from
 * interacting with crazy people

`josh build` does not require the user to deal with any of the bullshit:
  * build scripts are plain C code that get concatenated with a josh_build.h header
  * the most useful bits of the API take seconds to understand
    * `JB_RUN(mkdir -p build);` executes `mkdir -p build` just as it would on a command line
    * build an executable in 3 lines of code `JBExecutable josh = {"josh"}; josh.sources = (const char *[]){"src/main.c", NULL}; jb_build(&josh);`
  * `josh library` produces a copy of `josh_build.h` to be `#include`'d so that you never need the `josh` driver again
  * written by a person without an agenda

## Build

Run `./bootstrap.sh` to build the `josh` driver. For fun, `./build/josh build` will build the driver again but using the josh build system in `josh.build`.

## Usage

### Quick Start

Run `josh init` in an empty folder to create a template project. This creates a new `josh.build` and `src/main.c`. Run `josh build` to build the project template.

## License

This program is beer-ware, see header in `src/josh_build.h`.
