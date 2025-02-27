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
    * build an executable in 3 lines of code `JBExecutable josh = {"josh"}; josh.sources = (const char *[]){"src/main.c", NULL}; jb_build_exe(&josh);`
  * `josh library` produces a copy of `josh_build.h` to be `#include`'d so that you never need the `josh` driver again
  * written by a person without an agenda

## Build

Run `./bootstrap.sh` to build the `josh` driver. For fun, `./build/josh build` will build the driver again but using the josh build system in `build.josh`.

## Usage

### Quick Start

Run `josh init` in an empty folder to create a template project. This creates a new `build.josh` and `src/main.c`. Run `josh build` to build the project template.

### Cross-compiling

Set `JBExecutable.toolchain` to instruct josh build to cross-compile. Find a target toolchain via `jb_find_toolchain()`.


```c
JBToolchain *toolchain = jb_find_toolchain(JB_ENUM(ARM64), JB_ENUM(Linux), JB_ENUM(GNU));
if (toolchain != jb_native_toolchain()) {
    if(!toolchain) {
        printf("WARNING: no toolchain for arm64-linux-gnu; skipping build...\n");
    }
    else {
        // Cross build linux executable
        JBExecutable josh = {"josh_linux"};
        josh.sources = (const char *[]){"src/main.c", NULL};
        josh.build_folder = "build";

        josh.toolchain = toolchain;
        jb_build_exe(&josh);
    }
}
else {
    printf("Host is running arm64-linux-gnu; skipping cross-build...\n");
}
```

By default, josh build searches for `/toolchains` in the current working directory. Call `jb_set_toolchain_directory()` to use a custom path.

The `toolchains` folder is typically expected to contain:
```
/bin/<target>-gcc
/bin/<target>-g++
/bin/<target>-ld

/<target>/sys-root/ -- sysroot containing system headers, libc, libc++, and other packages
```

`tools/toolchain_builder.josh` can be used to build a basic Linux cross-compiler with Linux kernel headers, glibc, libstdc++, etc...
Example:
```
josh build-file tools/toolchain_builder.josh arm64-linux-gnu
```
Using josh build's build.josh script:
```
josh build toolchain arm64-linux-gnu
```

## License

This program is beer-ware, see header in `src/josh_build.h`.
