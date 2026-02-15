/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <josh> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Josh Huelsman
 * ----------------------------------------------------------------------------
 */

// The josh API is composed of 3 sets of functions:
// high-level driver functions (ie, build a build.josh file) start with josh_*
// project-level configuration/build functions
// utility functions

// Many josh build API functions use `char **` or `char *[]` arguments.
// When `string-array` is used in documentation, it refers to this data type and pattern:
// These represent arrays of strings where the final entry is followed by the value NULL.
// For example, `(char *[]){ "mycoolsrc.c", "myawesomesrc.c", NULL }`
// A convenience macro is provided: JB_STRING_ARRAY("mycoolsrc.c", "myawesomesrc.c")
// Convenience function to count entries in string array: jb_string_array_count(array)

// Generally, if a function returns a const-ptr such as `const char *`
// There is no new memory being used to store the contents of the pointer, the lifetime of
// the returned value may be dependent on the lifetime of one of the arguments.
// A plain pointer `char *` will require being free'd.

// Additionally, if using the `josh` driver program, or by building another build.josh file
// with josh_build(), the macro JB_BUILD_JOSH_PATH is defined to the path given as the first
// argument to josh_build(). For the `josh` driver, this path will be the absolute/realpath
// to the build.josh file.

#ifndef JOSH_BUILD_H
#define JOSH_BUILD_H

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#define JB_IS_MACOS   0
#define JB_IS_LINUX   0
#define JB_IS_WINDOWS 0

#if __APPLE__
#undef  JB_IS_MACOS
#define JB_IS_MACOS 1
#define JB_DEFAULT_VENDOR JB_ENUM(Apple)
#define JB_DEFAULT_RUNTIME JB_ENUM(Darwin)
#elif __linux__
#undef  JB_IS_LINUX
#define JB_IS_LINUX 1
#define JB_DEFAULT_VENDOR JB_ENUM(Linux)
#define JB_DEFAULT_RUNTIME JB_ENUM(GNU)
#elif defined(_WIN32)
#undef  JB_IS_WINDOWS
#define JB_IS_WINDOWS 1
#define JB_DEFAULT_VENDOR JB_ENUM(Windows)
#define JB_DEFAULT_RUNTIME JB_ENUM(MSVC)
#endif

#if JB_IS_WINDOWS
#define JB_PATH_SEPARATOR '/'
#define JB_DEBUG_BREAK() DebugBreak()
#define _jb_typeof(x) __typeof__(x)
#else
#define JB_PATH_SEPARATOR '/'
#define JB_DEBUG_BREAK() __builtin_debugtrap()
#define _jb_typeof(x) typeof(x)
#endif

#if (__x86_64__) || (_M_AMD64)
#define JB_DEFAULT_ARCH JB_ENUM(X86_64)
#elif (__aarch64__) || (_M_ARM64)
#define JB_DEFAULT_ARCH JB_ENUM(ARM64)
#endif

// Builds and runs a build.josh
void josh_build(const char *path, const char *exec_name, char *args[]);

// Parsers argv arguments and applies built-in options for recoginized switches.
// Returns a string-array with remaining arguments that we not consumed.
char **josh_parse_arguments(int argc, char *argv[]);

#define JOSH_BUILD(path, exec_name, ...) josh_build(path, exec_name, (char *[]){ __VA_ARGS__ __VA_OPT__(,) NULL })

#define JB_ENUM(x) JBEnum_ ## x

enum JBArch {
    JB_ENUM(INVALID_ARCH),
    JB_ENUM(X86_64),
    JB_ENUM(X86),
    JB_ENUM(ARM64),
};

enum JBVendor {
    JB_ENUM(INVALID_VENDOR),
    JB_ENUM(UNKNOWN_VENDOR),
    JB_ENUM(Apple),
    JB_ENUM(Linux),
    JB_ENUM(Windows),
};

enum JBRuntime {
    JB_ENUM(INVALID_RUNTIME),
    JB_ENUM(Darwin),
    JB_ENUM(GNU),
    JB_ENUM(MSVC),
    JB_ENUM(ELF), // freestanding elf target
};

typedef struct {
    enum JBArch arch;
    enum JBVendor vendor;
    enum JBRuntime runtime;

    char *name; // If given by the user, uses this name for tool lookups and error messaging
} JBTriple;

typedef struct {
    JBTriple triple;

    char *lib_dir;

    char *cc;
    char *cxx;
    char *ld;
    char *ar;

    char *clang;

    char *sysroot;
} JBToolchain;

void jb_set_toolchain_directory(const char *path);
JBToolchain *jb_native_toolchain();
JBToolchain *jb_find_toolchain(enum JBArch arch, enum JBVendor vendor, enum JBRuntime runtime);
JBToolchain *jb_find_llvm_toolchain(enum JBArch arch, enum JBVendor vendor, enum JBRuntime runtime);
JBToolchain *jb_find_toolchain_by_triple(const char *triple);
char *jb_get_triple(JBToolchain *toolchain);

// Find a tool in the target toolchains directory
char *jb_toolchain_find_tool(JBToolchain *toolchain, const char *tool);

#define _JB_TARGET_HEADER_COMMON \
    const char *name; \
    const char *build_folder; \
    const char **sources; \
    const char **ldflags; /* ignored for static libraries */ \
    const char **cflags; \
    const char **cxxflags; \
    const char **asflags; \
    const char **include_paths; \
    const char **frameworks; /* only applies to apple targets */ \
    const char **system_libraries; \
    struct JBLibrary **libraries; \
    JBToolchain *toolchain

typedef struct JBTarget {
    _JB_TARGET_HEADER_COMMON;
} JBTarget;

#define JB_LIBRARY_STATIC (0 << 0)
#define JB_LIBRARY_SHARED (1 << 0)

// if specified, uses the generated object files of the library instead of
// the generated library archive/executable, when linking this library against
// an executable.
#define JB_LIBRARY_USE_OBJECTS (1 << 1)

typedef struct JBLibrary {
    _JB_TARGET_HEADER_COMMON;

    int flags;
} JBLibrary;

void jb_build_lib(JBLibrary *lib);

#define JB_LIBRARY_ARRAY(...) (JBLibrary *[]){ __VA_ARGS__, NULL }

typedef struct {
    _JB_TARGET_HEADER_COMMON;
} JBExecutable;

void jb_build_exe(JBExecutable *exec);

void jb_compile_c(JBTarget *target, JBToolchain *tc, const char *source, const char *output);
void jb_compile_cxx(JBTarget *target, JBToolchain *tc, const char *source, const char *output);
void jb_compile_asm(JBTarget *target, JBToolchain *tc, const char *source, const char *output);

void jb_run_string(const char *cmd, char *const extra[], const char *file, int line);
void jb_run(char *const argv[], const char *file, int line);

struct JBRunResult {
    int exit_code;
    char *output;
};

struct JBRunResult jb_run_get_output(char *const argv[], const char *file, int line);

#define JB_CMD_ARRAY(...) (char **)(const char *const []){__VA_ARGS__ __VA_OPT__(,) NULL, }

#define JB_STRING_ARRAY(...) (const char *[]){__VA_ARGS__ __VA_OPT__(,) NULL, }

// Iterate null-terminated array
#define JBNullArrayFor(array) for (int index = 0; array && array[index]; index++)

int jb_string_array_count(char **array);

#define JB_RUN_CMD(...) jb_run(JB_CMD_ARRAY(__VA_ARGS__), __FILE__, __LINE__)

#define _JB_RUN(cmd, ...) jb_run_string(#cmd, JB_CMD_ARRAY(__VA_ARGS__), __FILE__, __LINE__)
#define JB_RUN(cmd, ...) _JB_RUN(cmd __VA_OPT__(,) __VA_ARGS__)

char *jb_concat(const char *lhs, const char *rhs);
char *jb_copy_string(const char *str);
char *jb_va_format_string(const char *fmt, va_list args);
char *jb_format_string(const char *fmt, ...);

const char *jb_filename(const char *path);
const char *jb_extension(const char *path);
char *jb_drop_last_path_component(const char *path);

int jb_file_exists(const char *path);

char *jb_file_fullpath(const char *path);

void jb_copy_file(const char *oldpath, const char *newpath);

void jb_mkdir(const char *path);

// Generates #embed-style text in output_file based on the contents of input_file
void jb_generate_embed(const char *input_file, const char *output_file);

char *jb_getcwd();

// Return 1 if source-file was last modified after dest-file.
// Fails if source doesnt exist.
// Returns 1 if source exists and dest does not.
int jb_file_is_newer(const char *source, const char *dest);

typedef struct {
    void *data;
    size_t count;
} JBArrayGeneric;

static inline void jb_array_push(JBArrayGeneric *arr, void *src, int tsize) {
    arr->data = realloc(arr->data, (arr->count+1) * tsize);
    arr->count += 1;

    char *dst = (char *)arr->data + (arr->count-1)*tsize;
    memcpy(dst, src, tsize);
}

#define _JB_ARRAY_TMP_NAME2(x, y) x ## y
#define _JB_ARRAY_TMP_NAME(x, y) _JB_ARRAY_TMP_NAME2(x, y)

#define JBArray(x) \
    union { struct { x *data; size_t count; }; JBArrayGeneric generic; }

#define JBArrayPush(x, v) \
    { \
        _jb_typeof(*(x)->data) _JB_ARRAY_TMP_NAME(tmp, __LINE__) = (v); \
        jb_array_push(&(x)->generic, &_JB_ARRAY_TMP_NAME(tmp, __LINE__), sizeof(*(x)->data)); \
    }

#define JBArrayRemove(x, index) \
    { \
        assert(index >= 0 && index < (x)->count); \
        memmove((x)->data + index, (x)->data + (index) + 1, sizeof(*(x)->data) * ((x)->count - ((index) + 1)) ); \
        (x)->count -= 1; \
    }

#define JBArrayInit(data, count) { { data, count} }

#define JBArrayFor(x) \
    for (size_t index = 0; index < (x)->count; index++)

#define JBArrayForEach(x) \
    for (_jb_typeof((x)->data) it = (x)->data; it < ((x)->data + (x)->count); it += 1)

typedef struct {
    void *data;
    size_t count;
    size_t reserved;
} JBVectorGeneric;

static inline void jb_vector_reserve(JBVectorGeneric *vec, size_t amt, int tsize) {
    if (amt < vec->reserved)
        return;

    vec->data = realloc(vec->data, amt * tsize);
    vec->reserved = amt;
}

static inline void jb_vector_maybe_grow(JBVectorGeneric *vec, int tsize) {
    if (vec->count + 1 <= vec->reserved)
        return;

    size_t growth_target = ((vec->count + 1) * 4) / 3;

    assert(growth_target > vec->count);

    jb_vector_reserve(vec, growth_target, tsize);
}

static inline void jb_vector_push(JBVectorGeneric *vec, void *src, int tsize) {
    jb_vector_maybe_grow(vec, tsize);

    assert(vec->count < vec->reserved);
    char *dst = (char *)vec->data + vec->count*tsize;
    memcpy(dst, src, tsize);
    vec->count += 1;
}

#define JBVector(x) \
    union { struct { x *data; size_t count; size_t reserved; }; JBVectorGeneric generic; }

#define JBVectorPush(x, v) \
    { \
        _jb_typeof(*(x)->data) _JB_ARRAY_TMP_NAME(tmp, __LINE__) = (v); \
        jb_vector_push(&(x)->generic, &_JB_ARRAY_TMP_NAME(tmp, __LINE__), sizeof(*(x)->data)); \
    }
#define JBVectorRemove(x, index) JBArrayRemove(x, index)

#define JBVectorPop(x) \
    { \
        assert((x)->count > 0); \
        JBVectorRemove((x), (x)->count-1); \
    }

#define JBVectorFor(x) \
    for (size_t index = 0; index < (x)->count; ++index)

#define JBVectorForReverse(x) \
    for (size_t index = (x)->count-1; index < (x)->count; --index)

#define JB_FAIL(msg, ...) do { jb_log_print(msg "\n" __VA_OPT__(,) __VA_ARGS__); exit(1); } while(0)

#define JB_ASSERT(cond, msg, ...) if (!(cond)) JB_FAIL(msg, __VA_ARGS__)

typedef struct {
    char *mem;
    size_t allocated;
    size_t current;
} JBArena;

void jb_arena_init(JBArena *arena, size_t size);
void *jb_arena_alloc(JBArena *arena, size_t amt, size_t alignment);

typedef struct {
    JBVector(JBArena) arenas;
} JBStringBuilder;

void jb_sb_init(JBStringBuilder *sb);
void jb_sb_free(JBStringBuilder *sb);
void jb_sb_putchar(JBStringBuilder *sb, int c);
void jb_sb_puts(JBStringBuilder *sb, const char *s);
char *jb_sb_to_string(JBStringBuilder *sb);

int jb_iswhitespace(int c);
int jb_isalpha(int c);
int jb_isnumber(int c);
int jb_isalphanumeric(int c);

void jb_log_set_file(const char *path);
void jb_va_log(const char *fmt, va_list args);
void jb_log(const char *fmt, ...);
void jb_log_print(const char *fmt, ...);

#define JB_LOG(fmt, ...) jb_log_print("[jb] " fmt __VA_OPT__(,) __VA_ARGS__)

#ifdef JOSH_BUILD_IMPL

#include <string.h>

#include <fcntl.h>

#if JB_IS_WINDOWS

#include <Windows.h>
#include <direct.h> // chdir
#include <io.h> // open, close, write, _commit

#else

#if JB_IS_MACOS
#include <util.h>
#else
#include <pty.h>
#endif

#include <poll.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/errno.h>

#endif // JB_IS_WINDOWS

int _jb_log_print_only = 0;
int _jb_verbose_show_commands = 0;
int _jb_log_fd = -1;

// set to 1 to make josh_build() make josh_runner debuggable:
// adds -g flag
// sets file and line numbers to correspond to intermediate build.josh.c file
// retains josh_runner and build.josh.c files
int _jb_debug_runner = 0;

// experimental: enable/disable psuedo-terminal mode;
// performs additional filtering when writing to log file to remove control sequences
// enables pretty, colored text in terminal output.
int _jb_use_pty = 1;

void jb_log_set_file(const char *path) {
    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0777);


    if (fd < 0) {
        // We can't use JB_ macros since they use the logging system and we may
        // get here from jb_va_log
        printf("could not open log file: %s", path);
        exit(1);
    }

    if (_jb_log_fd >= 0)
        close(_jb_log_fd);

    _jb_log_fd = fd;
}

void jb_va_log(const char *fmt, va_list args) {
    if (_jb_log_print_only)
        return;

    if (_jb_log_fd == -1) {
        jb_log_set_file("josh.log");
    }

    char *out = jb_va_format_string(fmt, args);

    if (_jb_use_pty) {
        char *filtered = malloc(strlen(out)+1);
        size_t bytes = 0;

        // General escape codes
        // https://gist.github.com/machinamentum/b20cb3fbe4b4afa62fc9f6ffaba821e3

        // hyperlink OSC
        // https://gist.github.com/machinamentum/184f4f073924972325a6a0e2d63bdd2a
        const char ESC = '\x1b';
        const char BEL = '\x07';
        char *c = out;
        while (*c) {
            if (*c == ESC) {
                c += 1;

                if (*c == ']') {
                    c += 1;

                    if (*c == '8')
                        c += 1;

                    if (*c == ';')
                        c += 1;

                    while (*c && *c != ';')
                        c += 1;

                    if (*c == ';')
                        c += 1;

                    // find ST or BEL
                    while (*c && (*c != BEL)) {
                        if (*c == ESC && *(c+1) == '\\') {
                            c += 1;
                            break;
                        }

                        c += 1;
                    }

                    if (*c)
                        c += 1;

                    while (*c && *c != ESC) {
                        filtered[bytes] = *c;
                        c += 1;
                        bytes += 1;
                    }

                    if (*c)
                        c += 1;

                    if (*c == ']')
                        c += 1;

                    if (*c == '8')
                        c += 1;

                    if (*c == ';')
                        c += 1;

                    if (*c == ';')
                        c += 1;

                    if (*c == BEL)
                        c += 1;
                    else if (*c == ESC && *(c+1) == '\\')
                        c += 2;

                    continue;
                }

                if (*c == '[')
                    c += 1;

                while (*c && !jb_isalpha(*c)) {
                    c += 1;
                }

                if (*c)
                    c += 1;
            }
            else {
                filtered[bytes] = *c;
                c += 1;
                bytes += 1;
            }
        }

        write(_jb_log_fd, filtered, bytes);
        free(filtered);
    }
    else {
        write(_jb_log_fd, out, strlen(out));
        free(out);
    }

#if JB_IS_WINDOWS
    _commit(_jb_log_fd);
#else
    fsync(_jb_log_fd);
#endif
}

void jb_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    jb_va_log(fmt, args);
    va_end(args);
}

void jb_log_print(const char *fmt, ...) {
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        // Required in the event that we are running in a josh_builder; since our parent process would be a .josh, printf will not flush stdout since
        // we are not writing to a tty. We could use forkpty() instead of fork() in _jb_run_internal but that may have additional performance overhead
        // to measure and consider.
        fflush(stdout);
    }

    {
        va_list args;
        va_start(args, fmt);
        jb_va_log(fmt, args);
        va_end(args);
    }
}

char *_jb_read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");

    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *out = malloc(len + 1);

    size_t read = fread(out, 1, len, f);

    if (read != len)
        return NULL;

    out[len] = 0;

    if (out_len)
        *out_len = len;

    return out;
}

extern const char _jb_josh_build_src[];

void josh_build(const char *path, const char *exec_name, char *args[]) {
    const char *build_folder = "build";
    jb_mkdir(build_folder);

    char *josh_builder_file = jb_format_string("%s/%s.c", build_folder, jb_filename(path));

    JBExecutable josh = {exec_name};
    josh.build_folder = build_folder;
    josh.sources = JB_STRING_ARRAY(josh_builder_file);

    char *josh_builder_exe = jb_format_string("%s/%s", build_folder, josh.name);

    if (jb_file_is_newer(path, josh_builder_exe)) {
        char *build_source = _jb_read_file(path, NULL);

        JB_ASSERT(build_source, "could not read file: %s", path);

        FILE *out = fopen(josh_builder_file, "wb");
        JB_ASSERT(out, "could not generate josh-builder file %s\n", josh_builder_file);

        const char *preamble = "#define JOSH_BUILD_IMPL\n";
        fputs(preamble, out);

        fprintf(out, "#define JB_BUILD_JOSH_PATH \"%s\"\n", path);

        if (!_jb_debug_runner)
            fputs("#line 1 \"josh_build.h\"\n", out);

        fwrite(_jb_josh_build_src, 1, strlen(_jb_josh_build_src), out);
        fputc('\n', out);

        {
            fputs("#ifdef JOSH_BUILD_IMPL\n", out);
            fputs("const char _jb_josh_build_src[] = {\n", out);

            const char *text = _jb_josh_build_src;
            while (*text) {
                fprintf(out, "0x%X", *text);

                text += 1;
                if (*text)
                    fprintf(out, ", ");
            }
            fputs("\n    , 0\n", out);

            fputs("};\n", out);
            fputs("#endif // JOSH_BUILD_IMPL\n", out);
        }

        if (!_jb_debug_runner)
            fputs("#line 1 \"build.josh.c\"\n", out);

        fwrite(build_source, 1, strlen(build_source), out);
        fclose(out);

        char *fullpath = jb_file_fullpath(path);
        char *folder_path = fullpath ? jb_drop_last_path_component(fullpath) : NULL;
        josh.cflags = JB_STRING_ARRAY(JB_IS_WINDOWS ? "/std:c11" : NULL);
        josh.include_paths = JB_STRING_ARRAY(folder_path);

        if (_jb_debug_runner)
            josh.cflags = JB_STRING_ARRAY("-g");

        jb_build_exe(&josh);

        free(fullpath);
        free(folder_path);
    }

    {
        JB_LOG("run %s\n", josh_builder_exe);
        JBVector(char *) cmds = {0};
        JBVectorPush(&cmds, josh_builder_exe);

        JBNullArrayFor(args) {
            JBVectorPush(&cmds, (char *)args[index]);
        }

        JBVectorPush(&cmds, NULL);

        jb_run(cmds.data, __FILE__, __LINE__);

        free(cmds.data);
    }

    if (!_jb_debug_runner) {
        remove(josh_builder_file);
        // remove(josh_builder_exe);
    }

    free(josh_builder_file);
    free(josh_builder_exe);
}

char **josh_parse_arguments(int argc, char *argv[]) {

    JBVector(char *) out = {0};

    const char *log_switch = "--log=";
    const char *log_level_none = "none";
    const char *log_level_default = "default";
    const char *log_level_verbose = "verbose";

    const char *verbose_switch = "--verbose"; // same as --log=verbose

    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], log_switch, strlen(log_switch)) == 0) {
            const char *level = argv[i] + strlen(log_switch);

            if (strcmp(level, log_level_verbose) == 0) {
                _jb_verbose_show_commands = 1;
            }
            else if (strcmp(level, log_level_default) == 0) {
                // no-op
            }
            else if (strcmp(level, log_level_none) == 0) {
                _jb_log_print_only = 1;
            }
            else {
                JB_FAIL("unrecognized log level: %s", level);
            }
        }
        else if (strcmp(argv[i], verbose_switch) == 0) {
            _jb_verbose_show_commands = 1;
        }
        else {
            JBVectorPush(&out, argv[i]);
        }
    }

    JBVectorPush(&out, NULL);
    return out.data;
}

typedef void (*_JBDrainPipeFn)(void *ctx, const char *str);

void _jb_pipe_drain_log_proxy(void *context, const char *str) {
    jb_log_print("%s", str);
}

void _jb_pipe_drain_sb_proxy(void *context, const char *str) {
    jb_sb_puts((JBStringBuilder *)context, str);
}

#if JB_IS_WINDOWS

int _jb_pipe_read_would_not_block(HANDLE fd) {
    DWORD bytes_available = 0;
    BOOL result = PeekNamedPipe(fd, NULL, 0, NULL, &bytes_available, NULL);

    if (!result) {
        jb_log_print("could not peek pipe\n");
        return 0;
    }

    return bytes_available > 0;
}

int _jb_pipe_has_data(HANDLE fd) {
    if (!_jb_pipe_read_would_not_block(fd))
        return 0;

    char c;
    DWORD bytes = 0;
    return ReadFile(fd, &c, 1, &bytes, NULL) && bytes > 0;
}

void _jb_drain_pipe(HANDLE fd, void *context, _JBDrainPipeFn fn) {
    enum { buffer_size = 4096*2 };
    char buffer[buffer_size];

    while (_jb_pipe_read_would_not_block(fd)) {
        DWORD bytes = 0;
        BOOL result = ReadFile(fd, buffer, buffer_size-1, &bytes, NULL);

        if (!result)
            break;

        if (bytes <= 0)
            break;

        if (bytes > 0) {
            buffer[bytes] = 0;
            fn(context, buffer);
        }
    }
}

int _jb_run_internal(char *const argv[], void *print_ctx, _JBDrainPipeFn print_fn, const char *file, int line) {
    if (_jb_verbose_show_commands) {
        JBNullArrayFor(argv) {
            jb_log_print("%s ", argv[index]);
        }

        jb_log_print("\n");
    }

    char *cmdline = NULL;
    {
        JBStringBuilder sb = {};
        jb_sb_init(&sb);

        JBNullArrayFor(argv) {
            jb_sb_puts(&sb, argv[index]);
            jb_sb_putchar(&sb, ' ');
        }

        cmdline = jb_sb_to_string(&sb);

        jb_sb_free(&sb);
    }

    int pty = _jb_use_pty;
    // TODO see if ConPTY is needed for some things
    // On mac/Linux, pty mode is needed to get the child process to output colored text controls
    // since if it cannot detect a terminal, it assumes output is redirected to a file
    SECURITY_ATTRIBUTES security_attr = {};
    security_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    security_attr.bInheritHandle = TRUE;

    HANDLE output_read = NULL;
    HANDLE output_write = NULL;

    HANDLE input_read = NULL;
    HANDLE input_write = NULL;

    if (!CreatePipe(&output_read, &output_write, &security_attr, 0)) {
        jb_log_print("could not create output pipe for %s\n", argv[0]);
        return 1;
    }

    if (!SetHandleInformation(output_read, HANDLE_FLAG_INHERIT, 0)) {
        jb_log_print("could not set output pipe flag for %s\n", argv[0]);
        return 1;
    }

    if (!CreatePipe(&input_read, &input_write, &security_attr, 0)) {
        jb_log_print("could not create input pipe for %s\n", argv[0]);
        return 1;
    }

     if (!SetHandleInformation(input_write, HANDLE_FLAG_INHERIT, 0)) {
        jb_log_print("could not set input pipe flag for %s\n", argv[0]);
        return 1;
    }

    STARTUPINFOA startup_info = {};
    startup_info.cb = sizeof(STARTUPINFOA);
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
    startup_info.hStdInput = input_read;
    startup_info.hStdOutput = output_write;
    startup_info.hStdError = output_write;

    PROCESS_INFORMATION process_info = {};

    if (!CreateProcessA(NULL, cmdline, NULL, NULL,
                        TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL,
                        &startup_info, &process_info)) {
        // TODO call GetLastError and report
        jb_log_print("Could not run %s, error ID 0x%X\n", argv[0], GetLastError());
        free(cmdline);
        return 1;
    }

    free(cmdline);

    do {
        _jb_drain_pipe(output_read, print_ctx, print_fn);
    } while (WaitForSingleObject(process_info.hProcess, 0) != WAIT_OBJECT_0);

    _jb_drain_pipe(output_read, print_ctx, print_fn);
    JB_ASSERT(!_jb_pipe_has_data(output_read), "data still in pipe");

    DWORD exit_code = 0;
    if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        jb_log_print("Could not get process exit code for %s\n", argv[0]);
        return 1;
    }

    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    CloseHandle(output_write);
    CloseHandle(input_read);
    return exit_code;
}

#else

int _jb_pipe_read_would_not_block(int fd) {
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    pfd.fd = fd;
    pfd.events = POLLIN;

    int result = poll(&pfd, 1, 0);
    return result > 0 && (pfd.revents & (POLLERR | POLLNVAL)) == 0;
}

int _jb_pipe_has_data(int fd) {
    if (!_jb_pipe_read_would_not_block(fd))
        return 0;

    char c;
    return read(fd, &c, 1) > 0;
}

void _jb_drain_pipe(int fd, void *context, _JBDrainPipeFn fn) {
    enum { buffer_size = 4096*2 };
    char buffer[buffer_size];

    while (_jb_pipe_read_would_not_block(fd)) {
        ssize_t bytes = read(fd, buffer, buffer_size-1);

        if (bytes <= 0)
            break;

        if (bytes > 0) {
            buffer[bytes] = 0;
            fn(context, buffer);
        }
    }
}

int _jb_run_internal(char *const argv[], void *print_ctx, _JBDrainPipeFn print_fn, const char *file, int line) {
    if (_jb_verbose_show_commands) {
        JBNullArrayFor(argv) {
            jb_log_print("%s ", argv[index]);
        }

        jb_log_print("\n");
    }

    int pty = _jb_use_pty;

    int pipefd[2];
    if (!pty)
        JB_ASSERT(pipe(pipefd) == 0, "could not open pipe");

    // fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    // fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
    pid_t pid = pty ? forkpty(&pipefd[0], NULL, NULL, NULL) : fork();
    JB_ASSERT(pid >= 0, "could not fork() process to execute %s\n", argv[0]);

    if (pid) {
        // parent

        // close write pipe
        if (!pty)
            close(pipefd[1]);

        int wstatus;
        pid_t w;
        
        errno = 0;
        do {

            w = waitpid(pid, &wstatus, WNOHANG);

            if (w == -1) {
                jb_log_print("WAIT FAILED %s\n", strerror(errno));
                // JB_FAIL("wait failed");
                return 0;
            }

            _jb_drain_pipe(pipefd[0], print_ctx, print_fn);

            // skip status checks if 0 because the child hasn't changed state.
            if (w > 0 && (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)))
                break;

        } while (1);

        JB_ASSERT(!_jb_pipe_has_data(pipefd[0]), "data still in pipe");

        close(pipefd[0]);

        if (WIFSIGNALED(wstatus)) {
            jb_log("%s:%d: %s: %s\n", file, line, argv[0], strsignal(WTERMSIG(wstatus)));
            return 1;
        }

        if (WEXITSTATUS(wstatus) != 0) {
            jb_log("%s:%d: %s: exit %d\n", file, line, argv[0], WEXITSTATUS(wstatus));
            return WEXITSTATUS(wstatus);
        }

        return WEXITSTATUS(wstatus);
    }
    else {
        // child

        if (!pty) {
            // close read pipe
            close(pipefd[0]);

            // Map stdout and stderr to the pipe
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
        }

        execvp(argv[0], argv);
        jb_log_print("Could not run %s\n", argv[0]);
        exit(1);
    }
}

#endif // JB_IS_WINDOWS

void jb_run(char *const argv[], const char *file, int line) {
    int result = _jb_run_internal(argv, NULL, _jb_pipe_drain_log_proxy, file, line);

    if (result)
        exit(1);
}

struct JBRunResult jb_run_get_output(char *const argv[], const char *file, int line) {
    JBStringBuilder sb;
    jb_sb_init(&sb);

    int result = _jb_run_internal(argv, &sb, _jb_pipe_drain_sb_proxy, file, line);

    char *output = jb_sb_to_string(&sb);

    jb_sb_free(&sb);

    jb_log("%s", output);

    struct JBRunResult out;
    out.exit_code = result;
    out.output = output;

    return out;
}

int jb_iswhitespace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v';
}

int jb_isalpha(int c) {
    c |= 0x20;
    return c >= 'a' && c <= 'z';
}

int jb_isnumber(int c) {
    return c >= '0' && c <= '9';
}

int jb_isalphanumeric(int c) {
    return jb_isalpha(c) || jb_isnumber(c);
}

void jb_run_string(const char *cmd, char *const extra[], const char *file, int line) {
    JBVector(char *) args = {0};

    const char *start = cmd;

    while (*start && jb_iswhitespace(*start))
        start += 1;

    const char *end = start;

    while (*start) {
        end += 1;

        if ((jb_iswhitespace(*end) || *end == 0) && start != end) {
            size_t len = end-start;
            char *out = malloc(len+1);

            memcpy(out, start, len);
            out[len] = 0;

            JBVectorPush(&args, out);

            while (*end && jb_iswhitespace(*end))
                end += 1;

            start = end;
        }
    }

    JBStringBuilder sb;
    jb_sb_init(&sb);

    JBArrayFor(&args) {
        char *start = args.data[index];
        char *input = start;

        while (*input) {
            if (*input == '$') {
                input += 1;
                char *env_start = input;

                while (*input && jb_isalphanumeric(*input)) {
                    input += 1;
                }

                char *env = jb_format_string("%.*s", input-env_start, env_start);

                char *value = getenv(env);

                while (value && *value) {
                    jb_sb_putchar(&sb, *value);
                    value += 1;
                }

                free(env);
                continue;
            }

            jb_sb_putchar(&sb, *input);
            input += 1;
        }

        free(start);
        args.data[index] = jb_sb_to_string(&sb);

        jb_sb_free(&sb);
        jb_sb_init(&sb);
    }

    jb_sb_free(&sb);

    size_t allocated_end = args.count;

    JBNullArrayFor(extra) {
        JBVectorPush(&args, extra[index]);
    }

    JBVectorPush(&args, NULL);
    jb_run(args.data, file, line);

    for (size_t i = 0; i < allocated_end; i++) {
        free(args.data[i]);
    }

    free(args.data);
}

int jb_string_array_count(char **array) {
    int i = 0;
    while (array && array[i])
        i += 1;

    return i;
}

char *jb_concat(const char *lhs, const char *rhs) {
    size_t l = strlen(lhs);
    size_t r = strlen(rhs);

    char *out = malloc(l+r+1);
    memcpy(out, lhs, l);
    memcpy(out+l, rhs, r);
    out[l+r] = 0;
    return out;
}

char *jb_copy_string(const char *str) {
    size_t l = strlen(str);

    char *out = malloc(l+1);
    memcpy(out, str, l+1);
    return out;
}

char *jb_va_format_string(const char *fmt, va_list args) {
    size_t size = 4096;

    char *buf = malloc(size);

    int result;

    {
        va_list args_copy;
        va_copy(args_copy, args);

        result = vsnprintf(buf, size, fmt, args_copy);

        va_end(args_copy);
    }

    if (result < 0) {
        free(buf);
        return NULL;
    }

    if (result < size)
        return buf;

    size = result+1;
    buf = realloc(buf, size);

    {
        va_list args_copy;
        va_copy(args_copy, args);

        result = vsnprintf(buf, size, fmt, args_copy);

        va_end(args_copy);
    }

    if (result < 0) {
        free(buf);
        return NULL;
    }

    return buf;
}

char *jb_format_string(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *out = jb_va_format_string(fmt, args);
    va_end(args);
    return out;
}

const char *jb_filename(const char *path) {
    char *slash = strrchr(path, JB_PATH_SEPARATOR);

    if (slash) {
        char *out = slash+1;

        if (strlen(out) == 0)
            return NULL;

        return out;
    }

    return path;
}

const char *jb_extension(const char *path) {
    const char *filename = jb_filename(path);

    const char *ext = strrchr(filename, '.');

    if (ext)
        ext += 1;

    return ext;
}

char *jb_drop_last_path_component(const char *path) {
    char *slash = strrchr(path, JB_PATH_SEPARATOR);

    if (slash) {
        char *end = slash;

        if (strlen(end) == 0)
            return jb_copy_string(path);

        size_t len = end-path;
        char *out = malloc(len+1);
        memcpy(out, path, len);
        out[len] = 0;
        return out;
    }

    return jb_copy_string(path);
}

const char *_jb_arch_string(enum JBArch arch) {
    switch (arch) {
    case JB_ENUM(X86_64):
        return "x86_64";
    case JB_ENUM(X86):
        return "x86";
    case JB_ENUM(ARM64):
        return "arm64";
    case JB_ENUM(INVALID_ARCH):
    default:
        return NULL;
    }
}

enum JBArch _jb_arch(const char *str) {
    if (strcmp(str, "x86_64") == 0)
        return JB_ENUM(X86_64);
    if (strcmp(str, "x86") == 0 || strcmp(str, "i386") == 0 || strcmp(str, "i686") == 0)
        return JB_ENUM(X86);
    if (strcmp(str, "arm64") == 0)
        return JB_ENUM(ARM64);
    if (strcmp(str, "aarch64") == 0)
        return JB_ENUM(ARM64);

    return JB_ENUM(INVALID_ARCH);
}

const char *_jb_vendor_string(enum JBVendor vendor) {
    switch (vendor) {
    case JB_ENUM(Apple):
        return "apple";
    case JB_ENUM(Linux):
        return "linux";
    case JB_ENUM(Windows):
        return "windows";
    case JB_ENUM(UNKNOWN_VENDOR):
        return "unknown";
    case JB_ENUM(INVALID_VENDOR):
    default:
        return NULL;
    }
}

enum JBVendor _jb_vendor(const char *str) {
    if (!str)
        return JB_ENUM(UNKNOWN_VENDOR);

    if (strcmp(str, "apple") == 0)
        return JB_ENUM(Apple);
    if (strcmp(str, "linux") == 0)
        return JB_ENUM(Linux);
    if (strcmp(str, "windows") == 0)
        return JB_ENUM(Windows);

    return JB_ENUM(UNKNOWN_VENDOR);
}

const char *_jb_runtime_string(enum JBRuntime runtime) {
    switch (runtime) {
    case JB_ENUM(Darwin):
        return "darwin";
    case JB_ENUM(GNU):
        return "gnu";
    case JB_ENUM(MSVC):
        return "msvc";
    case JB_ENUM(ELF):
        return "elf";
    case JB_ENUM(INVALID_RUNTIME):
    default:
        return NULL;
    }
}

enum JBRuntime _jb_runtime(const char *str) {
    if (strcmp(str, "darwin") == 0)
        return JB_ENUM(Darwin);
    if (strcmp(str, "gnu") == 0)
        return JB_ENUM(GNU);
    if (strcmp(str, "msvc") == 0)
        return JB_ENUM(MSVC);
    if (strcmp(str, "elf") == 0)
        return JB_ENUM(ELF);

    return JB_ENUM(INVALID_RUNTIME);
}

int _jb_is_objc_ext(const char *ext) {
    return strcmp(ext, "m") == 0 || strcmp(ext, "mm") == 0;
}

typedef JBVector(char *) _JBCommandVector;
void _jb_add_common_c_options(JBToolchain *tc, _JBCommandVector *cmd, const char *tool, const char **cflags, const char **include_paths) {
    char *triplet = jb_get_triple(tc);
    int is_msvc = (tc->triple.vendor == JB_ENUM(Windows));

    if (is_msvc) {
        JBVectorPush(cmd, "/nologo");
    }

    if (triplet && strstr(tool, "clang") != NULL) {
        JBVectorPush(cmd, "-target");
        JBVectorPush(cmd, triplet);

        // JBVectorPush(cmd, "--rtlib=compiler-rt");
    }


    if (tc->sysroot) {
        // TODO check if Apple LD or GNU LD
        // JBVectorPush(cmd, "-syslibroot");
        JBVectorPush(cmd, "--sysroot");
        JBVectorPush(cmd, tc->sysroot);
    }

    JBNullArrayFor(cflags) {
        JBVectorPush(cmd, (char *)cflags[index]);
    }

    JBNullArrayFor(include_paths) {
        if (is_msvc) {
            JBVectorPush(cmd, "/I");
        }
        else {
            JBVectorPush(cmd, "-I");
        }
        JBVectorPush(cmd, (char *)include_paths[index]);
    }

    if (is_msvc) {
        JBVectorPush(cmd, "/c");
    }
    else {
        JBVectorPush(cmd, "-c");
    }
}

char **_jb_get_dependencies_c(JBToolchain *tc, const char *tool, const char *source, const char **cflags, const char **include_paths) {
    int is_msvc = (tc->triple.vendor == JB_ENUM(Windows));

    _JBCommandVector cmd = {0};

    JBVectorPush(&cmd, (char *)tool);

    if (is_msvc) {
        JBVectorPush(&cmd, "/sourceDependencies-"); // '-' at the end dumps to stdout
        // tells compiler to do syntax check only (prevents generating .obj file)
        // Thankfully, as of 19.50 toolchain, this option still enables /sourceDependencies to generate output
        JBVectorPush(&cmd, "/Zs");
    }
    else {
        JBVectorPush(&cmd, "-MM");
    }

    _jb_add_common_c_options(tc, &cmd, tool, cflags, include_paths);

    JBVectorPush(&cmd, (char *)source);

    JBVectorPush(&cmd, NULL);

    struct JBRunResult run_result = jb_run_get_output(cmd.data, __FILE__, __LINE__);
    char *result = NULL;

    if (run_result.exit_code)
        free(run_result.output);
    else
        result = run_result.output;


    free(cmd.data);

    if (!result)
        return NULL;

    JBVector(char *) out = {0};

    if (is_msvc) {
        // TODO implement proper JSON parsing of /sourceDependencies
        // Here's a hacky solution: search for __"Includes": [__ string in result
        // then for each line following, search for first " and ending ",

        const char *SOURCE_TOKEN = "\"Source\":";
        char *source = strstr(result, SOURCE_TOKEN);
        if (!source)
            return NULL;

        {
            source += strlen(SOURCE_TOKEN);
            source = strchr(source, '\"');

            if (!source)
                return NULL;

            source += 1;

            char *end = strstr(source, "\",");
            if (!end)
                return NULL;

            char *entry = jb_format_string("%.*s", end-source, source);
            JBVectorPush(&out, entry);
        }

        const char *INCLUDES_TOKEN = "\"Includes\": [";
        char *includes = strstr(result, INCLUDES_TOKEN);

        if (!includes)
            return NULL;

        includes += strlen(INCLUDES_TOKEN);

        while (1) {
            includes = strchr(includes, '\"');

            if (!includes)
                break;

            includes += 1;

            char *end = strstr(includes, "\"");
            if (!end)
                break;

            char *entry = jb_format_string("%.*s", end-includes, includes);
            JBVectorPush(&out, entry);

            includes = end + 2;
        }
    }
    else {
        for (int i = 0; result[i]; i++) {
            char n = result[i+1];
            if (result[i] == '\\' && (n == '\r' || n == '\n')) {
                result[i] = ' ';
            }
        }

        char *start = strchr(result, ':')+1;

        while (*start) {
            while (*start && jb_iswhitespace(*start))
                start += 1;

            if (*start) {
                char *end = start;

                while (*end) {

                    {
                        if (*end == '\\' && *(end + 1) != 0) {
                            end += 2;
                            continue;
                        }
                    }

                    if (jb_iswhitespace(*end))
                        break;

                    end += 1;
                }

                char *entry = jb_format_string("%.*s", end-start, start);
                JBVectorPush(&out, entry);

                start = end;
            }
        }
    }

    JBVectorPush(&out, NULL);

    return out.data;
}

char **_jb_get_dependencies_asm(JBToolchain *tc, const char *tool, const char *source, const char **asflags, const char **include_paths) {
    // TODO dependency tracking for assembler sources
    JBVector(char *) out = {0};
    JBVectorPush(&out, (char *)source);
    JBVectorPush(&out, NULL);

    return out.data;
}

void jb_compile_c(JBTarget *target, JBToolchain *tc, const char *source, const char *output) {
    const char **cflags = target->cflags;
    const char **include_paths = target->include_paths;

    char *triplet = jb_get_triple(tc);

    JB_ASSERT(tc->cc, "Toolchain (%s) missing C compiler", triplet);

    char **deps = _jb_get_dependencies_c(tc, tc->cc, source, cflags, include_paths);

    int needs_build = 0;

    if (!deps) {
        jb_log("couldn't compute dependenices for %s, rebuilding...\n", source);
        needs_build = 1;
    }

    if (!needs_build) {
        JBNullArrayFor(deps) {
            if (jb_file_is_newer(deps[index], output)) {
                needs_build = 1;
                break;
            }
        }
    }

    free(deps);

    if (!needs_build)
        return;

    JB_LOG("compile %s\n", source);

    _JBCommandVector cmd = {0};

    JBVectorPush(&cmd, tc->cc);

    _jb_add_common_c_options(tc, &cmd, tc->cc, cflags, include_paths);
    int is_msvc = (tc->triple.vendor == JB_ENUM(Windows));

    if (is_msvc) {
        JBVectorPush(&cmd, "/Fo:");
    }
    else {
        JBVectorPush(&cmd, "-o");
    }
    JBVectorPush(&cmd, (char *)output);

    JBVectorPush(&cmd, "-c");
    JBVectorPush(&cmd, (char *)source);

    JBVectorPush(&cmd, NULL);
    jb_run(cmd.data, __FILE__, __LINE__);

    free(cmd.data);

    free(triplet);
}

void jb_compile_cxx(JBTarget *target, JBToolchain *tc, const char *source, const char *output) {
    const char **cxxflags = target->cxxflags;
    const char **include_paths = target->include_paths;

    char *triplet = jb_get_triple(tc);

    JB_ASSERT(tc->cxx, "Toolchain (%s) missing C++ compiler", triplet);

    char **deps = _jb_get_dependencies_c(tc, tc->cxx, source, cxxflags, include_paths);

    int needs_build = 0;

    if (!deps) {
        jb_log("couldn't compute dependenices for %s, rebuilding...\n", source);
        needs_build = 1;
    }

    if (!needs_build) {
        JBNullArrayFor(deps) {
            if (jb_file_is_newer(deps[index], output)) {
                needs_build = 1;
                break;
            }
        }
    }

    free(deps);

    if (!needs_build)
        return;

    JB_LOG("compile %s\n", source);

    _JBCommandVector cmd = {0};

    JBVectorPush(&cmd, tc->cxx);

    _jb_add_common_c_options(tc, &cmd, tc->cxx, cxxflags, include_paths);

    int is_msvc = (tc->triple.vendor == JB_ENUM(Windows));
    if (is_msvc) {
        JBVectorPush(&cmd, "/Fo:");
    }
    else {
        JBVectorPush(&cmd, "-o");
    }
    JBVectorPush(&cmd, (char *)output);

    JBVectorPush(&cmd, "-c");
    JBVectorPush(&cmd, (char *)source);

    JBVectorPush(&cmd, NULL);
    jb_run(cmd.data, __FILE__, __LINE__);

    free(cmd.data);

    free(triplet);
}

void jb_compile_asm(JBTarget *target, JBToolchain *tc, const char *source, const char *output) {
    const char **asflags = target->asflags;
    const char **include_paths = target->include_paths;

    char *triplet = jb_get_triple(tc);

    JB_ASSERT(tc->cc, "Toolchain (%s) missing assembler", triplet);

    char **deps = _jb_get_dependencies_asm(tc, tc->cc, source, asflags, include_paths);

    JB_ASSERT(deps, "couldn't compute dependenices for %s", source);

    int needs_build = 0;
    JBNullArrayFor(deps) {
        if (jb_file_is_newer(deps[index], output)) {
            needs_build = 1;
            break;
        }
    }

    free(deps);

    if (!needs_build)
        return;

    JB_LOG("compile %s\n", source);

    JBVector(char *) cmd = {0};

    JBVectorPush(&cmd, tc->cc);

    if (triplet && strstr(tc->cc, "clang") != NULL) {
        JBVectorPush(&cmd, "-target");
        JBVectorPush(&cmd, triplet);

        // JBVectorPush(&cmd, "--rtlib=compiler-rt");
    }

    if (tc->sysroot) {
        // TODO check if Apple LD or GNU LD
        // JBVectorPush(&cmd, "-syslibroot");
        JBVectorPush(&cmd, "--sysroot");
        JBVectorPush(&cmd, tc->sysroot);
    }

    JBNullArrayFor(asflags) {
        JBVectorPush(&cmd, (char *)asflags[index]);
    }

    JBNullArrayFor(include_paths) {
        JBVectorPush(&cmd, "-I");
        JBVectorPush(&cmd, (char *)include_paths[index]);
    }

    JBVectorPush(&cmd, "-o");
    JBVectorPush(&cmd, (char *)output);
    JBVectorPush(&cmd, "-c");
    JBVectorPush(&cmd, (char *)source);

    JBVectorPush(&cmd, NULL);
    jb_run(cmd.data, __FILE__, __LINE__);

    free(cmd.data);

    free(triplet);
}

int _jb_supported_source_ext(const char *ext) {
    return strcmp(ext, "c") == 0 || strcmp(ext, "cpp") == 0
        || strcmp(ext, "m") == 0 || strcmp(ext, "mm") == 0
        || strcmp(ext, "s") == 0;
}

void _jb_init_build(const char *build_folder, const char *object_folder)  {
    jb_mkdir(build_folder);
    jb_mkdir(object_folder);
}

char **_jb_collect_objects(JBTarget *target, JBToolchain *tc, const char *object_folder) {
    const char **sources = target->sources;
    int is_msvc = (tc->triple.vendor == JB_ENUM(Windows));

    JBVector(char *) object_files = {0};

    JBNullArrayFor(sources) {
        const char *filename = jb_filename(sources[index]);

        const char *ext = jb_extension(sources[index]);

        JB_ASSERT(ext && _jb_supported_source_ext(ext), "unsupported file type: %s", ext ? ext : "unknown");

        const char *o_ext = "o";
        if (is_msvc)
            o_ext = "obj";

        char *object = jb_format_string("%s%.*s%s", object_folder, strlen(filename)-strlen(ext), filename, o_ext);

        if (strcmp(ext, "c") == 0 || strcmp(ext, "m") == 0)
            jb_compile_c(target, tc, sources[index], object);
        else if (strcmp(ext, "cpp") == 0 || strcmp(ext, "mm") == 0)
            jb_compile_cxx(target, tc, sources[index], object);
        else if (strcmp(ext, "s") == 0)
            jb_compile_asm(target, tc, sources[index], object);

        JBVectorPush(&object_files, object);
    }

    JBVectorPush(&object_files, NULL);

    return object_files.data;
}

int _jb_target_has_cpp_source(JBTarget *target) {
    const char **sources = target->sources;

    JBNullArrayFor(sources) {
        const char *ext = jb_extension(sources[index]);

        JB_ASSERT(ext && _jb_supported_source_ext(ext), "unsupported file type: %s", ext ? ext : "unknown");

        if (strcmp(ext, "cpp") == 0 || strcmp(ext, "mm") == 0) {
            return 1;
        }
    }

    JBNullArrayFor(target->libraries) {
        JBLibrary *lib = target->libraries[index];

        if (_jb_target_has_cpp_source((JBTarget *)lib))
            return 1;
    }

    return 0;
}

char *_jb_get_link_command(JBToolchain *tc, JBTarget *target) {
    int is_msvc = (tc->triple.vendor == JB_ENUM(Windows));

    if (is_msvc)
        return tc->ld;

    char *link_command = tc->cc;

    if (_jb_target_has_cpp_source(target))
        link_command = tc->cxx;

    return link_command;
}

int _jb_need_to_build_target(const char *target, char **object_files) {
    JBNullArrayFor(object_files) {
        if (jb_file_is_newer(object_files[index], target)) {
            return 1;
        }
    }

    return 0;
}

char **_jb_get_library_objects(JBLibrary *target);

void _jb_link_shared(JBToolchain *tc, const char *link_command, const char **ldflags, const char **frameworks, char *output_exec, char **object_files, JBLibrary **libs, const char **system_libs, int is_lib) {

    char *triplet = jb_get_triple(tc);
    int is_msvc = (tc->triple.vendor == JB_ENUM(Windows));

    JB_LOG("link %s\n", output_exec);

    JBVector(char *) cmd = {0};

    JBVectorPush(&cmd, (char *)link_command);

    if (is_msvc) {
        JBVectorPush(&cmd, "/nologo");
    }

    if (strstr(link_command, "clang") != NULL) {
        JBVectorPush(&cmd, "-fuse-ld=lld");
    }

    // TODO triplet is always non-NULL here now. Either we always specify the target triple, or we get smarter about
    // wether toolchain is the system toolchain, or a cross-toolchain.
    if (triplet && strstr(link_command, "clang") != NULL) {
        JBVectorPush(&cmd, "-target");
        JBVectorPush(&cmd, triplet);

        JBVectorPush(&cmd, "--rtlib=compiler-rt");
    }

    if (tc->sysroot) {
        // TODO check if Apple LD or GNU LD
        // JBVectorPush(&cmd, "-syslibroot");
        JBVectorPush(&cmd, "--sysroot");
        JBVectorPush(&cmd, tc->sysroot);
    }

    if (is_lib) {
        JBVectorPush(&cmd, "-shared");
    }

    if (is_msvc) {
        // TODO check if cl.exe or link.exe is linker

        // cl.exe
        // JBVectorPush(&cmd, "/Fe:");
        // JBVectorPush(&cmd, output_exec); // Leak

        // link.exe
        JBVectorPush(&cmd, jb_format_string("/OUT:%s", output_exec)); // Leak
    }
    else {
        JBVectorPush(&cmd, "-o");
        JBVectorPush(&cmd, output_exec); // Leak
    }

    JBNullArrayFor(object_files) {
        JBVectorPush(&cmd, object_files[index]);
    }

    // JBVectorPush(&cmd, "-L/usr/local/lib");

    // if (tc->triple.runtime == JB_ENUM(Darwin)) {
    //     JBVectorPush(&cmd, "-lSystem");
    // }
    // else if (tc->triple.runtime == JB_ENUM(GNU)) {
    //     JBVectorPush(&cmd, "-lc");
    // }

#if JB_IS_LINUX
    // TODO more robust detection for gnu-ld and target-triple + toolchain
    if (strstr(link_command, "gcc")) {
        JBVectorPush(&cmd, "-Wl,--start-group");
    }
    else if(strstr(link_command, "ld")) {
        JBVectorPush(&cmd, "---start-group");
    }
#endif

    JBNullArrayFor(ldflags) {
        JBVectorPush(&cmd, (char *)ldflags[index]);
    }

    JBNullArrayFor(libs) {
        JBLibrary *lib = libs[index];

        // Leak
        if (lib->flags & JB_LIBRARY_USE_OBJECTS) {
            char **object_files = _jb_get_library_objects(lib); // Leak

            JBNullArrayFor(object_files) {
                char *obj = object_files[index];

                JBVectorPush(&cmd, obj);
            }
        }
        else {
            if (is_msvc) {
                JBVectorPush(&cmd, jb_format_string("/LIBPATH:%s", lib->build_folder));
                JBVectorPush(&cmd, jb_format_string("%s.lib", lib->name));
            }
            else {
                JBVectorPush(&cmd, jb_format_string("-L%s", lib->build_folder));
                JBVectorPush(&cmd, jb_format_string("-l%s", lib->name));
            }
        }
    }

    JBNullArrayFor(system_libs) {
        const char *lib = system_libs[index];

        // Leak
        if (is_msvc) {
            JBVectorPush(&cmd, jb_format_string("%s.lib", lib));
        }
        else {
            JBVectorPush(&cmd, jb_format_string("-l%s", lib));
        }
    }

#if JB_IS_LINUX
    // TODO more robust detection for gnu-ld and target-triple + toolchain
    if (strstr(link_command, "gcc")) {
        JBVectorPush(&cmd, "-Wl,--end-group");
    }
    else if(strstr(link_command, "ld")) {
        JBVectorPush(&cmd, "---end-group");
    }
#endif

    if (tc->triple.vendor == JB_ENUM(Apple)) {
        JBNullArrayFor(frameworks) {
            JBVectorPush(&cmd, "-framework");
            JBVectorPush(&cmd, (char *)frameworks[index]);
        }
    }

    JBVectorPush(&cmd, NULL);
    jb_run(cmd.data, __FILE__, __LINE__);

    free(cmd.data);
}

char *_jb_library_output_file(JBLibrary *target);

void jb_build_exe(JBExecutable *exec) {
    char *object_folder = jb_concat(exec->build_folder, "/object/");

    JBToolchain *tc = exec->toolchain ? exec->toolchain : jb_native_toolchain();

    JBNullArrayFor(exec->libraries) {
        JBLibrary *lib = exec->libraries[index];

        JB_ASSERT(lib->toolchain == exec->toolchain, "mismatch in toolchains used to build library %s and target %s", lib->name, exec->name);

        // TODO link directly against library objects to save some steps
        jb_build_lib(lib);
    }

    _jb_init_build(exec->build_folder, object_folder);

    char **object_files = _jb_collect_objects((JBTarget *)exec, tc, object_folder);

    char *link_command = _jb_get_link_command(tc, (JBTarget *)exec);

    int is_msvc = (tc->triple.vendor == JB_ENUM(Windows));
    char *output_exec = jb_format_string("%s/%s%s", exec->build_folder, exec->name, is_msvc ? ".exe" : "");

    int needs_build = 0;

    JBNullArrayFor(exec->libraries) {
        JBLibrary *lib = exec->libraries[index];
        char *lib_output_exe = _jb_library_output_file(lib);

        if (jb_file_is_newer(lib_output_exe, output_exec)) {
            free(lib_output_exe);
            needs_build = 1;
            break;
        }

        free(lib_output_exe);
    }

    if (!needs_build)
        needs_build = _jb_need_to_build_target(output_exec, object_files);

    if (needs_build) {
        _jb_link_shared(tc, link_command, exec->ldflags, exec->frameworks, output_exec, object_files, exec->libraries, exec->system_libraries, 0);
    }

    JBNullArrayFor(object_files)
        free(object_files[index]);

    free(object_files);
    free(object_folder);
    free(output_exec);
}

const char *_jb_lib_prefix(JBTriple triple) {
    switch (triple.vendor) {
    case JB_ENUM(Apple):
    case JB_ENUM(Linux):
        return "lib";
    case JB_ENUM(Windows):
    default:
        return "";
    }

    return "";
}

const char *_jb_lib_suffix(JBTriple triple, int flags) {
    if (flags & JB_LIBRARY_SHARED) {
        switch (triple.vendor) {
            case JB_ENUM(Apple):
                return "dylib";
            case JB_ENUM(Linux):
                return "so";
            case JB_ENUM(Windows):
                return "dll";
            default:
                return "";
        }
    }
    else {
        switch (triple.vendor) {
            case JB_ENUM(Apple):
            case JB_ENUM(Linux):
                return "a";
            case JB_ENUM(Windows):
                return "lib";
            default:
                return "";
        }
    }

    return "";
}

char *_jb_library_output_file(JBLibrary *target) {
    JBToolchain *tc = target->toolchain ? target->toolchain : jb_native_toolchain();
    char *output_exec = jb_format_string("%s/%s%s.%s", target->build_folder, _jb_lib_prefix(tc->triple), target->name, _jb_lib_suffix(tc->triple, target->flags));
    return output_exec;
}

char **_jb_get_library_objects(JBLibrary *target) {
    char *object_folder = jb_concat(target->build_folder, "/object/");

    JBToolchain *tc = target->toolchain ? target->toolchain : jb_native_toolchain();
    char **object_files = _jb_collect_objects((JBTarget *)target, tc, object_folder);

    free(object_folder);
    return object_files;
}

void jb_build_lib(JBLibrary *target) {

    char *object_folder = jb_concat(target->build_folder, "/object/");

    JBToolchain *tc = target->toolchain ? target->toolchain : jb_native_toolchain();

    JBNullArrayFor(target->libraries) {
        JBLibrary *lib = target->libraries[index];

        JB_ASSERT(lib->toolchain == target->toolchain, "mismatch in toolchains used to build library %s and target %s", lib->name, target->name);

        // TODO link directly against library objects to save some steps
        jb_build_lib(lib);
    }

    _jb_init_build(target->build_folder, object_folder);

    char **object_files = _jb_collect_objects((JBTarget *)target, tc, object_folder);

    char *link_command = _jb_get_link_command(tc, (JBTarget *)target);

    char *output_exec = _jb_library_output_file(target);

    int needs_build = 0;

    JBNullArrayFor(target->libraries) {
        JBLibrary *lib = target->libraries[index];
        char *lib_output_exe = _jb_library_output_file(lib);

        if (jb_file_is_newer(lib_output_exe, output_exec)) {
            free(lib_output_exe);
            needs_build = 1;
            break;
        }

        free(lib_output_exe);
    }

    if (!needs_build)
        needs_build = _jb_need_to_build_target(output_exec, object_files);

    if (needs_build) {
        if (target->flags & JB_LIBRARY_SHARED) {
            _jb_link_shared(tc, link_command, target->ldflags, target->frameworks, output_exec, object_files, target->libraries, target->system_libraries, 1);
        }
        else {
            char *triplet = jb_get_triple(tc);
            int is_msvc = (tc->triple.vendor == JB_ENUM(Windows));

            JB_ASSERT(tc->ar, "Toolchain (%s) missing AR", triplet);

            JB_LOG("built %s\n", output_exec);

            JBVector(char *) cmd = {0};

            JBVectorPush(&cmd, tc->ar);

            if (is_msvc) {
                JBVectorPush(&cmd, "/nologo");
                JBVectorPush(&cmd, jb_format_string("/OUT:%s", output_exec)); // Leak
            }
            else {
                JBVectorPush(&cmd, "rcs");
                JBVectorPush(&cmd, output_exec);
            }

            JBNullArrayFor(object_files) {
                JBVectorPush(&cmd, object_files[index]);
            }

            JBVectorPush(&cmd, NULL);
            jb_run(cmd.data, __FILE__, __LINE__);
            free(cmd.data);

            free(triplet);
        }
    }

    JBNullArrayFor(object_files) {
        free(object_files[index]);
    }

    free(object_files);
    free(object_folder);
    free(output_exec);
}

#if JB_IS_WINDOWS

char *_jb_convert_path_slashes(const char *path) {
    char *out = jb_copy_string(path);
    path = out;

    while (*path) {
        if (*path == '\\')
            *(char *)path = '/';
        path += 1;
    }

    return out;
}

char *_jb_unconvert_path_slashes(const char *path) {
    char *out = jb_copy_string(path);
    path = out;

    while (*path) {
        if (*path == '/')
            *(char *)path = '\\';
        path += 1;
    }

    return out;
}

int jb_file_exists(const char *path) {
    path = _jb_unconvert_path_slashes(path);
    DWORD attrib = GetFileAttributesA(path);
    free((char *)path);
    return attrib != INVALID_FILE_ATTRIBUTES;
}

void jb_copy_file(const char *_oldpath, const char *_newpath) {
    char *oldpath = _jb_unconvert_path_slashes(_oldpath);
    char *newpath = _jb_unconvert_path_slashes(_newpath);

    BOOL result = CopyFileA(oldpath, newpath, FALSE);
    JB_ASSERT(result, "could not copy file %s to %s", _oldpath, _newpath);
}

char *jb_file_fullpath(const char *path) {
    path = _jb_unconvert_path_slashes(path);

    DWORD bytes = GetFullPathNameA(path, 0, NULL, NULL);

    if (bytes == 0) {
        free((char *)path);
        return NULL;
    }

    char *buffer = malloc(bytes);
    DWORD result = GetFullPathNameA(path, bytes, buffer, NULL);

    JB_ASSERT(bytes >= result, "did not allocate enough bytes for jb_file_fullpath");

    char *output = _jb_convert_path_slashes(buffer);
    free(buffer);
    free((char *)path);
    return output;
}

char *jb_getcwd() {
    DWORD bytes = GetCurrentDirectory(0, NULL);

    char *buffer = malloc(bytes);

    DWORD result = GetCurrentDirectory(bytes, buffer);

    JB_ASSERT(bytes >= result, "did not allocate enough bytes for jb_getcwd");
    char *output = _jb_convert_path_slashes(buffer);
    free(buffer);
    return output;
}

FILETIME _jb_get_last_mod_time(const char *path) {
    if (!jb_file_exists(path))
        return (FILETIME){0};

    path = _jb_unconvert_path_slashes(path);
    HANDLE file = CreateFileA(path, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    free((char *)path);

    if (file == INVALID_HANDLE_VALUE) {
        return (FILETIME){0};
    }

    FILETIME last_write = {};
    if (!GetFileTime(file, NULL, NULL, &last_write)) {
        jb_log("GetFileTime failed\n");
        CloseHandle(file);
        return (FILETIME){0};
    }

    CloseHandle(file);

    return last_write;
}

int jb_file_is_newer(const char *source, const char *dest) {
    JB_ASSERT(jb_file_exists(source), "file not found: %s", source);

    FILETIME s = _jb_get_last_mod_time(source);
    FILETIME d = _jb_get_last_mod_time(dest);

    return CompareFileTime(&s, &d) > 0;
}

void jb_mkdir(const char *path) {
    char *copy = jb_copy_string(path);

    JBVector(char *) paths = {};

    do {
        JBVectorPush(&paths, copy);
        char *newstr = jb_drop_last_path_component(copy);
        copy = newstr;
    } while (strrchr(copy, JB_PATH_SEPARATOR));

    jb_log("mkdir with %zu sub directories\n", paths.count);

    JBVectorForReverse(&paths) {
        char *p = paths.data[index];

        if (!CreateDirectoryA(p, NULL)) {
            DWORD error = GetLastError();

            JB_ASSERT(error == ERROR_ALREADY_EXISTS, "could not create directory %s", p);
        }

        free(p);
    }
}

#else

int jb_file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

char *jb_file_fullpath(const char *path) {
    return realpath(path, NULL);
}

void jb_copy_file(const char *_oldpath, const char *_newpath) {
    JB_RUN(cp, _oldpath, _newpath);
}

char *jb_getcwd() {
    char buf[MAXPATHLEN+1];
    char *result = getcwd(buf, MAXPATHLEN+1);

    if (result)
        return jb_copy_string(result);

    return NULL;
}

struct timespec _jb_get_last_mod_time(const char *path) {
    if (!jb_file_exists(path))
        return (struct timespec){0};

    // TODO maybe JB_FAIL on failure ?
    struct stat st;
    stat(path, &st);

#if JB_IS_MACOS
    return st.st_mtimespec;
#else
    return st.st_mtim;
#endif
}

int jb_file_is_newer(const char *source, const char *dest) {

    JB_ASSERT(jb_file_exists(source), "file not found: %s", source);

    struct timespec s = _jb_get_last_mod_time(source);
    struct timespec d = _jb_get_last_mod_time(dest);

    if (s.tv_sec == d.tv_sec) {
        return s.tv_nsec > d.tv_nsec;
    }

    return s.tv_sec > d.tv_sec;
}

void jb_mkdir(const char *path) {
    JB_RUN_CMD("mkdir", "-p", path);
}

#endif // JB_IS_WINDOWS

const char *_jb_toolchain_dir = "toolchains";

void jb_set_toolchain_directory(const char *path) {
    _jb_toolchain_dir = path;
}

char *_jb_check_for_tool(const char *triplet, const char *tool, int is_llvm) {
    if (is_llvm) {
        char *tool_path = jb_format_string("%s/bin/%s", _jb_toolchain_dir, tool);
        if (!jb_file_exists(tool_path)) {
            free(tool_path);
            return NULL;
        }

        return tool_path;
    }
    else {
        char *toolchain_dir = jb_format_string("%s/%s", _jb_toolchain_dir, triplet);

        if (!jb_file_exists(toolchain_dir)) {
            JB_FAIL("Could not find a toolchain for %s in %s", triplet, _jb_toolchain_dir);
        }

        char *tool_path = jb_format_string("%s/bin/%s", toolchain_dir, tool);

        if (!jb_file_exists(tool_path)) {
            free(tool_path);

            tool_path = jb_format_string("%s/bin/%s-%s", _jb_toolchain_dir, triplet, tool);
        }

        free(toolchain_dir);

        if (!jb_file_exists(tool_path)) {
            free(tool_path);

            return NULL;
        }

        return tool_path;
    }
}

char *jb_toolchain_find_tool(JBToolchain *tc, const char *tool) {
    return _jb_check_for_tool(jb_get_triple(tc), tool, 0);
}

char *_jb_arch_from_triple(const char *target) {
    char *arch_end = strchr(target, '-');

    char *arch = jb_format_string("%.*s", arch_end-target, target);
    return arch;
}

char *_jb_runtime_from_triple(const char *target) {
    char *run_start = strrchr(target, '-');
    const char *run_end = target + strlen(target);

    run_start += 1; // skip '-'

    char *run = jb_format_string("%.*s", run_end-run_start, run_start);
    return run;
}

char *_jb_vendor_from_triple(const char *target) {
    char *vendor_start = strchr(target, '-');
    char *vendor_end = strrchr(target, '-');

    if (vendor_start == vendor_end)
        return NULL;

    vendor_start += 1;

    char *vendor = jb_format_string("%.*s", vendor_end-vendor_start, vendor_start);
    return vendor;
}

JBToolchain *jb_find_llvm_toolchain(enum JBArch arch, enum JBVendor vendor, enum JBRuntime runtime) {
    if (!jb_file_exists(_jb_toolchain_dir)) {
        return NULL;
    }

    JBToolchain *tc = malloc(sizeof(JBToolchain));
    memset(tc, 0, sizeof(JBToolchain));

    tc->triple.arch = arch;
    tc->triple.vendor = vendor;
    tc->triple.runtime = runtime;

    tc->clang = _jb_check_for_tool(NULL, "clang", 1);
    tc->ar = _jb_check_for_tool(NULL, "llvm-ar", 1);

    if (runtime == JB_ENUM(MSVC)) {
        tc->cc = _jb_check_for_tool(NULL, "clang-cl", 1);
        tc->ld = _jb_check_for_tool(NULL, "lld-link", 1);
    }
    else {
        tc->cc = tc->clang;
        tc->cxx = _jb_check_for_tool(NULL, "clang++", 1);
        // Should this be ld.lld for emulation mode?
        tc->ld = _jb_check_for_tool(NULL, "lld", 1);
    }

    return tc;
}

JBToolchain *jb_find_toolchain_by_triple(const char *triplet) {
    char *toolchain_dir = jb_format_string("%s/%s", _jb_toolchain_dir, triplet);
    if (!jb_file_exists(toolchain_dir)) {
        return NULL;
    }

    JBToolchain *tc = malloc(sizeof(JBToolchain));
    memset(tc, 0, sizeof(JBToolchain));

    char *arch = _jb_arch_from_triple(triplet);
    char *vendor = _jb_vendor_from_triple(triplet);
    char *runtime = _jb_runtime_from_triple(triplet);

    tc->triple.name = jb_copy_string(triplet);
    tc->triple.arch = _jb_arch(arch);
    tc->triple.vendor = _jb_vendor(vendor);
    tc->triple.runtime = _jb_runtime(runtime);

    tc->cc = _jb_check_for_tool(triplet, "gcc", 0);
    tc->cxx = _jb_check_for_tool(triplet, "g++", 0);
    tc->ld = _jb_check_for_tool(triplet, "ld", 0);
    tc->ar = _jb_check_for_tool(triplet, "ar", 0);

    tc->clang = _jb_check_for_tool(triplet, "clang", 1);

    if (!tc->cc)
        tc->cc = tc->clang;

    tc->sysroot = jb_format_string("%s/sys-root", toolchain_dir);
    return tc;
}

JBToolchain *jb_find_toolchain(enum JBArch arch, enum JBVendor vendor, enum JBRuntime runtime) {
    if (arch == JB_DEFAULT_ARCH && vendor == JB_DEFAULT_VENDOR && runtime == JB_DEFAULT_RUNTIME)
        return jb_native_toolchain();

    const char *_arch = _jb_arch_string(arch);
    const char *_vendor = _jb_vendor_string(vendor);
    const char *_runtime = _jb_runtime_string(runtime);

    // TODO this should be much more robust to support weird triples like i686-elf
    char *triplet = jb_format_string("%s-%s-%s", _arch, _vendor, _runtime);
    return jb_find_toolchain_by_triple(triplet);
}

static JBToolchain __jb_native_toolchain;

JBToolchain *jb_native_toolchain() {
    JBToolchain tc = {0};
    tc.triple.arch = JB_DEFAULT_ARCH;
    tc.triple.vendor = JB_DEFAULT_VENDOR;
    tc.triple.runtime = JB_DEFAULT_RUNTIME;

#if JB_IS_WINDOWS
    tc.cc = "cl";
    tc.cxx = "cl";
    tc.ld = "link";
    tc.ar = "lib";
#else
    tc.cc = "gcc";
    tc.cxx = "g++";
    tc.ld = "ld";
    tc.ar = "ar";
#endif

#if JB_IS_MACOS
    tc.sysroot = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk";
#endif

    __jb_native_toolchain = tc;
    return &__jb_native_toolchain;
}

char *jb_get_triple(JBToolchain *toolchain) {
    if (toolchain->triple.name)
        return jb_copy_string(toolchain->triple.name);

    const char *_arch = _jb_arch_string(toolchain->triple.arch);
    const char *_vendor = _jb_vendor_string(toolchain->triple.vendor);
    const char *_runtime = _jb_runtime_string(toolchain->triple.runtime);

    char *triple = jb_format_string("%s-%s-%s", _arch, _vendor, _runtime);
    return triple;
}

void jb_generate_embed(const char *input, const char *output) {
    size_t len = 0;
    char *text = _jb_read_file(input, &len);

    JB_ASSERT(text, "could not read file: %s", input);

    FILE *out = fopen(output, "wb");

    JB_ASSERT(out, "could not open file for writing: %s", output);

    for (size_t i = 0; i < len; i++) {
        fprintf(out, "0x%X", text[i]);

        if (i < (len-1))
            fprintf(out, ", ");
    }

    fputc('\n', out);

    fclose(out);
}

void jb_arena_init(JBArena *arena, size_t size) {
    arena->mem = (char *)malloc(size);
    arena->allocated = size;
    arena->current = 0;
}

size_t _jb_pad_to_alignment(size_t value, size_t align) {
    size_t mask = align-1;
    size_t alignment = ((align - (value & mask)) & mask);

    return value + alignment;
}

void *jb_arena_alloc(JBArena *arena, size_t amt, size_t alignment) {

    size_t offset = _jb_pad_to_alignment(arena->current, alignment);

    size_t next = offset + amt;

    if (next > arena->allocated || offset >= arena->allocated)
        return NULL;

    char *ptr = arena->mem + offset;
    arena->current = next;
    return ptr;
}

JBArena *_jb_sb_new_arena(JBStringBuilder *sb, size_t size) {
    JBArena tmp;
    jb_arena_init(&tmp, (size < 4096) ? 4096 : size);
    JBVectorPush(&sb->arenas, tmp);

    return &sb->arenas.data[sb->arenas.count-1];
}

void jb_sb_init(JBStringBuilder *sb) {
    memset(sb, 0, sizeof(JBStringBuilder));

    _jb_sb_new_arena(sb, 4096);
}

void jb_sb_free(JBStringBuilder *sb) {
    JBArrayForEach(&sb->arenas) {
        free(it->mem);
    }

    free(sb->arenas.data);
}


void jb_sb_putchar(JBStringBuilder *sb, int c) {
    JBArena *arena = &sb->arenas.data[sb->arenas.count-1];

    char *out = jb_arena_alloc(arena, 1, 1);

    if (!out) {
        arena = _jb_sb_new_arena(sb, 1);
        out = jb_arena_alloc(arena, 1, 1);

        if (!out)
            return;
    }

    *out = c;
}

void jb_sb_puts(JBStringBuilder *sb, const char *s) {
    JBArena *arena = &sb->arenas.data[sb->arenas.count-1];

    char *out = jb_arena_alloc(arena, strlen(s), 1);

    if (!out) {
        arena = _jb_sb_new_arena(sb, strlen(s));
        out = jb_arena_alloc(arena, strlen(s), 1);

        if (!out)
            return;
    }

    memcpy(out, s, strlen(s));
}

char *jb_sb_to_string(JBStringBuilder *sb) {
    size_t total = 1; // +1 for null byte

    JBArrayForEach(&sb->arenas) {
        total += it->current;
    }

    char *out = (char *)malloc(total);
    size_t off = 0;

    JBArrayForEach(&sb->arenas) {
        memcpy(out + off, it->mem, it->current);
    }

    out[total-1] = 0;

    return out;
}

#endif // JOSH_BUILD_IMPL

#endif // JOSH_BUILD_H
