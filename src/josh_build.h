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

#ifndef JOSH_BUILD_H
#define JOSH_BUILD_H

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#define JB_IS_MACOS   0
#define JB_IS_LUNIX   0
#define JB_IS_WINDOWS 0

#if __APPLE__
#undef  JB_IS_MACOS
#define JB_IS_MACOS 1
#define JB_DEFAULT_VENDOR JB_ENUM(Apple)
#define JB_DEFAULT_RUNTIME JB_ENUM(Darwin)
#elif __linux__
#undef  JB_IS_LUNIX
#define JB_IS_LUNIX 1
#define JB_DEFAULT_VENDOR JB_ENUM(Linux)
#define JB_DEFAULT_RUNTIME JB_ENUM(GNU)
#elif defined(WIN32)
#undef  JB_IS_WINDOWS
#define JB_IS_WINDOWS 1
#define JB_DEFAULT_VENDOR JB_ENUM(Windows)
#define JB_DEFAULT_RUNTIME JB_ENUM(MSVC)
#endif

#if (__x86_64__) || (_M_AMD64)
#define JB_DEFAULT_ARCH JB_ENUM(X86_64)
#elif (__aarch64__) || (_M_ARM64)
#define JB_DEFAULT_ARCH JB_ENUM(ARM64)
#endif

// Builds and runs a build.josh
void josh_build(const char *path, char *args[]);

// Parsers argv arguments and applies built-in options for recoginized switches.
// Returns a string-array with remaining arguments that we not consumed.
char **josh_parse_arguments(int argc, char *argv[]);

#define JOSH_BUILD(path, ...) josh_build(path, (char *const []){ __VA_ARGS__ __VA_OPT__(,) NULL })

#define JB_ENUM(x) JBEnum_ ## x

enum JBArch {
    JB_ENUM(INVALID_ARCH),
    JB_ENUM(X86_64),
    JB_ENUM(ARM64),
};

enum JBVendor {
    JB_ENUM(INVALID_VENDOR),
    JB_ENUM(Apple),
    JB_ENUM(Linux),
    JB_ENUM(Windows),
};

enum JBRuntime {
    JB_ENUM(INVALID_RUNTIME),
    JB_ENUM(Darwin),
    JB_ENUM(GNU),
    JB_ENUM(MSVC),
};

typedef struct {
    enum JBArch arch;
    enum JBVendor vendor;
    enum JBRuntime runtime;
} JBTriple;

typedef struct {
    JBTriple triple;

    char *lib_dir;

    char *cc;
    char *cxx;
    char *ld;
    char *ar;

    char *sysroot;
} JBToolchain;

void jb_set_toolchain_directory(const char *path);
JBToolchain *jb_native_toolchain();
JBToolchain *jb_find_toolchain(enum JBArch arch, enum JBVendor vendor, enum JBRuntime runtime);
JBToolchain *jb_find_toolchain_by_triple(const char *triple);

#define JB_LIBRARY_STATIC (0 << 0)
#define JB_LIBRARY_SHARED (1 << 0)

typedef struct JBLibrary {
    const char *name;
    const char **sources;

    const char *build_folder;

    const char **ldflags; // ignored for static libraries

    const char **cflags;
    const char **cxxflags;

    struct JBLibrary **libraries;

    int flags;

    JBToolchain *toolchain;
} JBLibrary;

void jb_build_lib(JBLibrary *lib);

#define JB_LIBRARY_ARRAY(...) (JBLibrary *[]){ __VA_ARGS__, NULL }

typedef struct {
    const char *name;
    const char **sources;

    const char *build_folder;

    const char **ldflags;
    const char **cflags;
    const char **cxxflags;

    JBLibrary **libraries;

    JBToolchain *toolchain;
} JBExecutable;

void jb_build_exe(JBExecutable *exec);

void jb_compile_c(JBToolchain *tc, const char *source, const char *output, const char **cflags);
void jb_compile_cxx(JBToolchain *tc, const char *source, const char *output, const char **cxxflags);

void jb_run_string(const char *cmd, char *const extra[], const char *file, int line);
void jb_run(char *const argv[], const char *file, int line);
char *jb_run_get_output(char *const argv[], const char *file, int line);

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

int jb_file_exists(const char *path);

// Generates #embed-style text in output_file based on the contents of input_file
void jb_generate_embed(const char *input_file, const char *output_file);

char *jb_getcwd();

struct timespec jb_get_last_mod_time(const char *path);

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
        typeof(*(x)->data) _JB_ARRAY_TMP_NAME(tmp, __LINE__) = (v); \
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
    for (typeof((x)->data) it = (x)->data; it < ((x)->data + (x)->count); it += 1)

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
        typeof(*(x)->data) _JB_ARRAY_TMP_NAME(tmp, __LINE__) = (v); \
        jb_vector_push(&(x)->generic, &_JB_ARRAY_TMP_NAME(tmp, __LINE__), sizeof(*(x)->data)); \
    }
#define JBVectorRemove(x, index) JBArrayRemove(x, index)

#define JBVectorPop(x) \
    { \
        assert((x)->count > 0); \
        JBVectorRemove((x), (x)->count-1); \
    }


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
int jb_isalphanumeric(int c);

void jb_log_set_file(const char *path);
void jb_va_log(const char *fmt, va_list args);
void jb_log(const char *fmt, ...);
void jb_log_print(const char *fmt, ...);

#define JB_LOG(fmt, ...) jb_log_print("[jb] " fmt __VA_OPT__(,) __VA_ARGS__)

#ifdef JOSH_BUILD_IMPL

#include <string.h>

#include <unistd.h>

#include <fcntl.h>
#include <poll.h>

#include <sys/wait.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/errno.h>

int _jb_log_print_only = 0;
int _jb_verbose_show_commands = 0;
int _jb_log_fd = -1;

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

    write(_jb_log_fd, out, strlen(out));
    fsync(_jb_log_fd);
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

void josh_build(const char *path, char *args[]) {
    char *build_source = _jb_read_file(path, NULL);

    JB_ASSERT(build_source, "could not read file: %s", path);

    char *josh_builder_file = jb_format_string("%s.c", path);

    FILE *out = fopen(josh_builder_file, "wb");
    const char *preamble = "#define JOSH_BUILD_IMPL\n#line 1 \"josh_build.h\"\n";
    fwrite(preamble, 1, strlen(preamble), out);
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

    fputs("#line 1 \"build.josh.c\"\n", out);
    fwrite(build_source, 1, strlen(build_source), out);
    fclose(out);

    JBExecutable josh = {"josh_builder"};
    josh.sources = JB_STRING_ARRAY(josh_builder_file);
    josh.build_folder = "build";
    jb_build_exe(&josh);

    char *josh_builder_exe = jb_format_string("%s/%s", josh.build_folder, josh.name);
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

    remove(josh_builder_file);
    remove(josh_builder_exe);

    free(josh_builder_file);
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

    return out.data;
}

typedef void (*_JBDrainPipeFn)(void *ctx, const char *str);

void _jb_pipe_drain_log_proxy(void *context, const char *str) {
    jb_log_print("%s", str);
}

void _jb_pipe_drain_sb_proxy(void *context, const char *str) {
    jb_sb_puts((JBStringBuilder *)context, str);
}

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
    const size_t buffer_size = 4096*2;
    char buffer[buffer_size];

    while (_jb_pipe_read_would_not_block(fd)) {
        ssize_t bytes = read(fd, buffer, buffer_size-1);

        if (bytes == 0)
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

    int pipefd[2];
    JB_ASSERT(pipe(pipefd) == 0, "could not open pipe");
    // fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    // fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

    pid_t pid = fork();
    JB_ASSERT(pid >= 0, "could not fork() process to execute %s\n", argv[0]);
    if (pid) {
        // parent

        // close write pipe
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
            jb_log_print("%s:%d: %s: %s\n", file, line, argv[0], strsignal(WTERMSIG(wstatus)));
            return 0;
        }

        if (WEXITSTATUS(wstatus) != 0) {
            jb_log_print("%s:%d: %s: exit %d\n", file, line, argv[0], WEXITSTATUS(wstatus));
            return 0;
        }

        return 1;
    }
    else {
        // child

        // close read pipe
        close(pipefd[0]);

        // Map stdout and stderr to the pipe
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        execvp(argv[0], argv);
        jb_log_print("Could not run %s\n", argv[0]);
        exit(1);
    }
}

void jb_run(char *const argv[], const char *file, int line) {
    int result = _jb_run_internal(argv, NULL, _jb_pipe_drain_log_proxy, file, line);

    if (!result)
        exit(1);
}

char *jb_run_get_output(char *const argv[], const char *file, int line) {
    JBStringBuilder sb;
    jb_sb_init(&sb);

    int result = _jb_run_internal(argv, &sb, _jb_pipe_drain_sb_proxy, file, line);

    char *output = jb_sb_to_string(&sb);

    if (!result) {
        jb_log_print("%s", output);
        exit(1);
    }

    jb_sb_free(&sb);
    return output;
}

int jb_iswhitespace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v';
}

int jb_isalpha(int c) {
    c |= 0x20;
    return c >= 'a' && c <= 'z';
}

int jb_isalphanumeric(int c) {
    return jb_isalpha(c) || (c >= '0' && c <= '9');
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
    char *slash = strrchr(path, '/');

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

const char *_jb_arch_string(enum JBArch arch) {
    switch (arch) {
    case JB_ENUM(X86_64):
        return "x86_64";
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
    case JB_ENUM(INVALID_VENDOR):
    default:
        return NULL;
    }
}

enum JBVendor _jb_vendor(const char *str) {
    if (strcmp(str, "apple") == 0)
        return JB_ENUM(Apple);
    if (strcmp(str, "linux") == 0)
        return JB_ENUM(Linux);
    if (strcmp(str, "windows") == 0)
        return JB_ENUM(Windows);

    return JB_ENUM(INVALID_VENDOR);
}

const char *_jb_runtime_string(enum JBRuntime runtime) {
    switch (runtime) {
    case JB_ENUM(Darwin):
        return "darwin";
    case JB_ENUM(Linux):
        return "gnu";
    case JB_ENUM(Windows):
        return "msvc";
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

    return JB_ENUM(INVALID_RUNTIME);
}

char **_jb_get_dependencies_c(JBToolchain *tc, const char *tool, const char *source, const char **cflags) {
    const char *_arch = _jb_arch_string(tc->triple.arch);
    const char *_vendor = _jb_vendor_string(tc->triple.vendor);
    const char *_runtime = _jb_runtime_string(tc->triple.runtime);

    char *triplet = jb_format_string("%s-%s-%s", _arch, _vendor, _runtime);

    JBVector(char *) cmd = {0};

    JBVectorPush(&cmd, (char *)tool);

    JBVectorPush(&cmd, "-MM");

    if (triplet && strstr(tool, "clang") != NULL) {
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

    JBNullArrayFor(cflags) {
        JBVectorPush(&cmd, (char *)cflags[index]);
    }

    JBVectorPush(&cmd, "-c");
    JBVectorPush(&cmd, (char *)source);

    JBVectorPush(&cmd, NULL);
    char *result = jb_run_get_output(cmd.data, __FILE__, __LINE__);

    free(cmd.data);

    free(triplet);

    if (!result)
        return NULL;

    for (int i = 0; result[i]; i++) {
        if (result[i] == '\\' && result[i+1] == '\n')
            result[i] = ' ';
    }

    JBVector(char *) out = {0};

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

    JBVectorPush(&out, NULL);

    return out.data;
}

void jb_compile_c(JBToolchain *tc, const char *source, const char *output, const char **cflags) {

    JB_ASSERT(tc->cc, "Toolchain missing C compiler");

    char **deps = _jb_get_dependencies_c(tc, tc->cc, source, cflags);

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

    const char *_arch = _jb_arch_string(tc->triple.arch);
    const char *_vendor = _jb_vendor_string(tc->triple.vendor);
    const char *_runtime = _jb_runtime_string(tc->triple.runtime);

    char *triplet = jb_format_string("%s-%s-%s", _arch, _vendor, _runtime);

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

    JBNullArrayFor(cflags) {
        JBVectorPush(&cmd, (char *)cflags[index]);
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

void jb_compile_cxx(JBToolchain *tc, const char *source, const char *output, const char **cflags) {

    JB_ASSERT(tc->cxx, "Toolchain missing C++ compiler");

    char **deps = _jb_get_dependencies_c(tc, tc->cxx, source, cflags);

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

    const char *_arch = _jb_arch_string(tc->triple.arch);
    const char *_vendor = _jb_vendor_string(tc->triple.vendor);
    const char *_runtime = _jb_runtime_string(tc->triple.runtime);

    char *triplet = jb_format_string("%s-%s-%s", _arch, _vendor, _runtime);

    JBVector(char *) cmd = {0};

    JBVectorPush(&cmd, tc->cxx);

    if (triplet && strstr(tc->cxx, "clang") != NULL) {
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

    JBNullArrayFor(cflags) {
        JBVectorPush(&cmd, (char *)cflags[index]);
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
    return strcmp(ext, "c") == 0 || strcmp(ext, "cpp") == 0;
}

void _jb_init_build(const char *build_folder, const char *object_folder)  {
    JB_RUN_CMD("mkdir", "-p", build_folder);
    JB_RUN_CMD("mkdir", "-p", object_folder);
}

char **_jb_collect_objects(JBToolchain *tc, const char *object_folder, const char **sources, const char **cflags, const char **cxxflags) {
    JBVector(char *) object_files = {0};

    JBNullArrayFor(sources) {
        const char *filename = jb_filename(sources[index]);

        const char *ext = jb_extension(sources[index]);

        JB_ASSERT(ext && _jb_supported_source_ext(ext), "unsupported file type: %s", ext ? ext : "unknown");

        char *object = jb_concat(object_folder, filename);

        char *o_ext = object+strlen(object)-strlen(ext);
        memcpy(o_ext, "o", 2); // 2 to include NULL byte

        if (strcmp(ext, "c") == 0)
            jb_compile_c(tc, sources[index], object, &cflags[0]);
        else if (strcmp(ext, "cpp") == 0)
            jb_compile_cxx(tc, sources[index], object, &cxxflags[0]);

        JBVectorPush(&object_files, object);
    }

    JBVectorPush(&object_files, NULL);

    return object_files.data;
}

char *_jb_get_link_command(JBToolchain *tc, const char **sources) {

    char *link_command = tc->cc;

    JBNullArrayFor(sources) {
        const char *ext = jb_extension(sources[index]);

        JB_ASSERT(ext && _jb_supported_source_ext(ext), "unsupported file type: %s", ext ? ext : "unknown");

        if (strcmp(ext, "c") == 0) {

        }
        else if (strcmp(ext, "cpp") == 0) {
            link_command = tc->cxx;
            break;
        }
    }

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

void _jb_link_shared(JBToolchain *tc, const char *link_command, const char **ldflags, char *output_exec, char **object_files, JBLibrary **libs, int is_lib) {

    const char *_arch = _jb_arch_string(tc->triple.arch);
    const char *_vendor = _jb_vendor_string(tc->triple.vendor);
    const char *_runtime = _jb_runtime_string(tc->triple.runtime);

    char *triplet = jb_format_string("%s-%s-%s", _arch, _vendor, _runtime);

    JB_LOG("link %s\n", output_exec);

    JBVector(char *) cmd = {0};

    JBVectorPush(&cmd, (char *)link_command);

    if (strstr(link_command, "clang") != NULL) {
        JBVectorPush(&cmd, "-fuse-ld=lld");
    }

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

    JBVectorPush(&cmd, "-o");
    JBVectorPush(&cmd, output_exec); // Leak

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

    JBNullArrayFor(ldflags) {
        JBVectorPush(&cmd, (char *)ldflags[index]);
    }

    JBNullArrayFor(libs) {
        JBLibrary *lib = libs[index];

        JBVectorPush(&cmd, jb_format_string("-L%s", lib->build_folder));
        JBVectorPush(&cmd, jb_format_string("-l%s", lib->name));
    }

    JBVectorPush(&cmd, NULL);
    jb_run(cmd.data, __FILE__, __LINE__);

    free(cmd.data);
}

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

    char **object_files = _jb_collect_objects(tc, object_folder, exec->sources, exec->cflags, exec->cxxflags);

    char *link_command = _jb_get_link_command(tc, exec->sources);

    char *output_exec = jb_concat(exec->build_folder, jb_concat("/", exec->name));

    int needs_build = _jb_need_to_build_target(output_exec, object_files);

    if (needs_build) {
        _jb_link_shared(tc, link_command, exec->ldflags, output_exec, object_files, exec->libraries, 0);
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

    char **object_files = _jb_collect_objects(tc, object_folder, target->sources, target->cflags, target->cxxflags);

    char *link_command = _jb_get_link_command(tc, target->sources);

    char *output_exec = jb_format_string("%s/%s%s.%s", target->build_folder, _jb_lib_prefix(tc->triple), target->name, _jb_lib_suffix(tc->triple, target->flags));

    int needs_build = _jb_need_to_build_target(output_exec, object_files);

    if (needs_build) {
        if (target->flags & JB_LIBRARY_SHARED) {
            _jb_link_shared(tc, link_command, target->ldflags, output_exec, object_files, target->libraries, 1);
        }
        else {

            JB_ASSERT(tc->ar, "Toolchain missing AR");

            JBVector(char *) cmd = {0};

            JBVectorPush(&cmd, tc->ar);

            JBVectorPush(&cmd, "rcs");

            JBVectorPush(&cmd, output_exec);

            JBNullArrayFor(object_files) {
                JBVectorPush(&cmd, object_files[index]);
            }

            JBVectorPush(&cmd, NULL);
            jb_run(cmd.data, __FILE__, __LINE__);
            free(cmd.data);
        }
    }

    JBNullArrayFor(object_files) {
        free(object_files[index]);
    }

    free(object_files);
    free(object_folder);
    free(output_exec);
}

int jb_file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

char *jb_getcwd() {
    char buf[MAXPATHLEN+1];
    char *result = getcwd(buf, MAXPATHLEN+1);

    if (result)
        return jb_copy_string(result);

    return NULL;
}

struct timespec jb_get_last_mod_time(const char *path) {
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

    struct timespec s = jb_get_last_mod_time(source);
    struct timespec d = jb_get_last_mod_time(dest);

    if (s.tv_sec == d.tv_sec) {
        return s.tv_nsec > d.tv_nsec;
    }

    return s.tv_sec > d.tv_sec;
}

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

JBToolchain *jb_find_toolchain(enum JBArch arch, enum JBVendor vendor, enum JBRuntime runtime) {

    if (arch == JB_DEFAULT_ARCH && vendor == JB_DEFAULT_VENDOR && runtime == JB_DEFAULT_RUNTIME)
        return jb_native_toolchain();

    const char *_arch = _jb_arch_string(arch);
    const char *_vendor = _jb_vendor_string(vendor);
    const char *_runtime = _jb_runtime_string(runtime);

    char *triplet = jb_format_string("%s-%s-%s", _arch, _vendor, _runtime);

    char *toolchain_dir = jb_format_string("%s/%s", _jb_toolchain_dir, triplet);
    if (!jb_file_exists(toolchain_dir)) {
        return NULL;
    }

    JBToolchain *tc = malloc(sizeof(JBToolchain));
    memset(tc, 0, sizeof(JBToolchain));

    tc->triple.arch = arch;
    tc->triple.vendor = vendor;
    tc->triple.runtime = runtime;

    tc->cc = _jb_check_for_tool(triplet, "gcc", 0);
    tc->cxx = _jb_check_for_tool(triplet, "g++", 0);
    tc->ld = _jb_check_for_tool(triplet, "ld", 0);
    tc->ar = _jb_check_for_tool(triplet, "ar", 0);

    tc->sysroot = jb_format_string("%s/sys-root", toolchain_dir);
    return tc;
}

JBToolchain *jb_find_toolchain_by_triple(const char *triple) {
    char *arch = jb_format_string("%.*s", strchr(triple, '-') - triple, triple);
    char *vendor = jb_format_string("%.*s", strrchr(triple, '-') - (triple + strlen(arch) + 1), triple + strlen(arch) + 1);
    char *runtime = jb_format_string("%.*s", strlen(triple) - (strlen(arch) + strlen(vendor) + 2), triple + strlen(arch) + strlen(vendor) + 2);

    enum JBArch _arch = _jb_arch(arch);
    enum JBVendor _vendor = _jb_vendor(vendor);
    enum JBRuntime _runtime = _jb_runtime(runtime);

    if (_arch == JB_ENUM(INVALID_ARCH)) 
        JB_FAIL("Unsupported architecture %s", arch);

    if (_vendor == JB_ENUM(INVALID_VENDOR))
        JB_FAIL("Unsupported vendor '%s'", vendor);

    if (_runtime == JB_ENUM(INVALID_RUNTIME))
        JB_FAIL("Unsupported runtime %s", runtime);

    return jb_find_toolchain(_arch, _vendor, _runtime);
}

static JBToolchain __jb_native_toolchain;

JBToolchain *jb_native_toolchain() {
    JBToolchain tc = {0};
    tc.triple.arch = JB_DEFAULT_ARCH;
    tc.triple.vendor = JB_DEFAULT_VENDOR;
    tc.triple.runtime = JB_DEFAULT_RUNTIME;

    tc.cc = "gcc";
    tc.cxx = "g++";
    tc.ld = "ld";
    tc.ar = "ar";

#if JB_IS_MACOS
    tc.sysroot = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk";
#endif

    __jb_native_toolchain = tc;
    return &__jb_native_toolchain;
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
        arena = _jb_sb_new_arena(sb, strlen(out));
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
