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

#ifndef JOSH_BUILD_H
#define JOSH_BUILD_H

#include <stdlib.h>
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

    char *sysroot;
} JBToolchain;

void jb_set_toolchain_directory(const char *path);
JBToolchain *jb_native_toolchain();
JBToolchain *jb_find_toolchain(enum JBArch arch, enum JBVendor vendor, enum JBRuntime runtime);
JBToolchain *jb_find_toolchain_by_triple(const char *triple);

typedef struct {
    const char *name;
    const char **sources;

    const char *build_folder;

    const char **cflags;

    JBToolchain *toolchain;
} JBExecutable;

void jb_build(JBExecutable *exec);

void jb_compile_c(JBToolchain *tc, const char *source, const char *output, const char **cflags);

void jb_run_string(const char *cmd, char *const extra[], const char *file, int line);
void jb_run(char *const argv[], const char *file, int line);
char *jb_run_get_output(char *const argv[], const char *file, int line);

#define JB_STRING_ARRAY(...) (const char *[]){__VA_ARGS__ __VA_OPT__(,) NULL, }
#define JB_CMD_ARRAY(...) (char **)(const char *const []){__VA_ARGS__ __VA_OPT__(,) NULL, }

#define JB_RUN_CMD(...) jb_run(JB_CMD_ARRAY(__VA_ARGS__), __FILE__, __LINE__)

#define _JB_RUN(cmd, ...) jb_run_string(#cmd, JB_CMD_ARRAY(__VA_ARGS__), __FILE__, __LINE__)
#define JB_RUN(cmd, ...) _JB_RUN(cmd __VA_OPT__(,) __VA_ARGS__)

char *jb_concat(const char *lhs, const char *rhs);
char *jb_copy_string(const char *str);
char *jb_format_string(const char *fmt, ...);

const char *jb_filename(const char *path);

int jb_file_exists(const char *path);

// Generates #embed-style text in output_file based on the contents of input_file
void jb_generate_embed(const char *input_file, const char *output_file);

char *jb_getcwd();

time_t jb_get_last_mod_time(const char *path);

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


#define JB_FAIL(msg, ...) do { printf(msg "\n" __VA_OPT__(,) __VA_ARGS__); exit(1); } while(0)

#define JB_ASSERT(cond, msg, ...) if (!(cond)) JB_FAIL(msg, __VA_ARGS__);

#ifdef JOSH_BUILD_IMPL

#include <string.h>
#include <stdarg.h>

#include <unistd.h>

#include <fcntl.h>

#include <sys/wait.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/errno.h>

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
    }

    fputs("#line 1 \"build.josh.c\"\n", out);
    fwrite(build_source, 1, strlen(build_source), out);
    fclose(out);

    JBExecutable josh = {"josh_builder"};
    josh.sources = JB_STRING_ARRAY(josh_builder_file);
    josh.build_folder = "build";
    jb_build(&josh);

    char *josh_builder_exe = jb_format_string("%s/%s", josh.build_folder, josh.name);
    {
        printf("[jb] run %s\n", josh_builder_exe);
        JBVector(char *) cmds = {0};
        JBVectorPush(&cmds, josh_builder_exe);

        for (int i = 0; args && args[i]; i++) {
            JBVectorPush(&cmds, (char *)args[i]);
        }

        JBVectorPush(&cmds, NULL);

        jb_run(cmds.data, __FILE__, __LINE__);

        free(cmds.data);
    }

    remove(josh_builder_file);
    remove(josh_builder_exe);

    free(josh_builder_file);
}

int _jb_verbose_show_commands = 0;

void jb_run(char *const argv[], const char *file, int line) {

    if (_jb_verbose_show_commands) {
        for (int i = 0; argv[i]; i++) {
            printf("%s ", argv[i]);
        }

        printf("\n");
    }

    pid_t pid = fork();
    if (pid) {
        // parent

        int wstatus;
        pid_t w;

        do {
            w = waitpid(pid, &wstatus, 0);

            if (w == -1) {
                printf("WAIT FAILED\n");
                return;
            }

        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));

        if (WIFSIGNALED(wstatus)) {
            printf("%s:%d: %s: %s\n", file, line, argv[0], strsignal(WTERMSIG(wstatus)));
            exit(1);
        }

        if (WEXITSTATUS(wstatus) != 0) {
            printf("%s:%d: %s: exit %d\n", file, line, argv[0], WEXITSTATUS(wstatus));
            exit(1);
        }
    }
    else {
        // child
        execvp(argv[0], argv);
        printf("Could not run %s\n", argv[0]);
        exit(1);
    }
}

char *jb_run_get_output(char *const argv[], const char *file, int line) {

    if (_jb_verbose_show_commands) {
        for (int i = 0; argv[i]; i++) {
            printf("%s ", argv[i]);
        }

        printf("\n");
    }

    int pipefd[2];
    pipe(pipefd);
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

    pid_t pid = fork();
    if (pid) {
        // parent

        // close write pipe
        close(pipefd[1]);

        int wstatus;
        pid_t w;

        do {
            w = waitpid(pid, &wstatus, 0);

            if (w == -1) {
                printf("WAIT FAILED\n");
                return NULL;
            }

        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));

        size_t buffer_size = 4096*16;
        char *buffer = malloc(buffer_size);
        memset(buffer, 0, buffer_size);
        size_t off = 0;

        do {
            ssize_t bytes = read(pipefd[0], buffer+off, buffer_size-off);

            if (bytes == 0)
                break;

            if (bytes > 0)
                off += bytes;

            if (off > buffer_size)
                break;
        } while (errno == EAGAIN);

        JB_ASSERT(buffer_size > off, "ran out of space when capturing command output");

        buffer[off] = 0;

        if (WIFSIGNALED(wstatus)) {
            puts(buffer);
            printf("%s:%d: %s: %s\n", file, line, argv[0], strsignal(WTERMSIG(wstatus)));
            exit(1);
        }

        if (WEXITSTATUS(wstatus) != 0) {
            puts(buffer);
            printf("%s:%d: %s: exit %d\n", file, line, argv[0], WEXITSTATUS(wstatus));
            exit(1);
        }

        return buffer;
    }
    else {
        // child

        // close read pipe
        close(pipefd[0]);

        // Map stdout and stderr to the pipe
        dup2(pipefd[1], 0);
        dup2(pipefd[1], 1);

        execvp(argv[0], argv);
        printf("Could not run %s\n", argv[0]);
        exit(1);
    }
}

int jb_iswhitespace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v';
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

    size_t allocated_end = args.count;

    for (int i = 0; extra && extra[i]; i++) {
        JBVectorPush(&args, extra[i]);
    }

    JBVectorPush(&args, NULL);
    jb_run(args.data, file, line);

    for (size_t i = 0; i < allocated_end; i++) {
        free(args.data[i]);
    }

    free(args.data);
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

char *jb_format_string(const char *fmt, ...) {
    size_t size = 4096;

    char *buf = malloc(size);

    int result;

    {
        va_list args;
        va_start(args, fmt);

        result = vsnprintf(buf, size, fmt, args);

        va_end(args);
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
        va_list args;
        va_start(args, fmt);

        result = vsnprintf(buf, size, fmt, args);

        va_end(args);
    }

    if (result < 0) {
        free(buf);
        return NULL;
    }

    return buf;
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

const char *_jb_arch_string(enum JBArch arch) {
    switch (arch) {
    case JB_ENUM(INVALID_ARCH):
        return NULL;
    case JB_ENUM(X86_64):
        return "x86_64";
    case JB_ENUM(ARM64):
        return "arm64";
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
    case JB_ENUM(INVALID_VENDOR):
        return NULL;
    case JB_ENUM(Apple):
        return "apple";
    case JB_ENUM(Linux):
        return "linux";
    case JB_ENUM(Windows):
        return "windows";
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
    case JB_ENUM(INVALID_RUNTIME):
        return NULL;
    case JB_ENUM(Darwin):
        return "darwin";
    case JB_ENUM(Linux):
        return "gnu";
    case JB_ENUM(Windows):
        return "msvc";
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

char **_jb_get_dependencies_c(JBToolchain *tc, const char *source, const char **cflags) {
    const char *_arch = _jb_arch_string(tc->triple.arch);
    const char *_vendor = _jb_vendor_string(tc->triple.vendor);
    const char *_runtime = _jb_runtime_string(tc->triple.runtime);

    char *triplet = jb_format_string("%s-%s-%s", _arch, _vendor, _runtime);

    JBVector(char *) cmd = {0};

    JBVectorPush(&cmd, tc->cc);

    JBVectorPush(&cmd, "-MM");

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

    for (int i = 0; cflags && cflags[i]; i++) {
        JBVectorPush(&cmd, (char *)cflags[i]);
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

    char **deps = _jb_get_dependencies_c(tc, source, cflags);

    JB_ASSERT(deps, "couldn't compute dependenices for %s", source);

    int needs_build = 0;
    for (int i = 0; deps && deps[i]; i++) {
        if (jb_file_is_newer(deps[i], output)) {
            needs_build = 1;
            break;
        }
    }

    free(deps);

    if (!needs_build)
        return;

    printf("[jb] compile %s\n", source);

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

    for (int i = 0; cflags && cflags[i]; i++) {
        JBVectorPush(&cmd, (char *)cflags[i]);
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

void jb_build(JBExecutable *exec) {

    char *object_folder = jb_concat(exec->build_folder, "/object/");

    JBToolchain *tc = exec->toolchain ? exec->toolchain : jb_native_toolchain();

    const char *_arch = _jb_arch_string(tc->triple.arch);
    const char *_vendor = _jb_vendor_string(tc->triple.vendor);
    const char *_runtime = _jb_runtime_string(tc->triple.runtime);

    char *triplet = jb_format_string("%s-%s-%s", _arch, _vendor, _runtime);

    if (!tc->cc) {
        JB_FAIL("Toolchain missing C compiler");
    }

    JB_RUN_CMD("mkdir", "-p", exec->build_folder);
    JB_RUN_CMD("mkdir", "-p", object_folder);

    JBVector(char *) object_files = {0};

    for (int i = 0; exec->sources[i]; i++) {
        const char *filename = jb_filename(exec->sources[i]);
        char *object = jb_concat(object_folder, filename);
        size_t len = strlen(object);

        object[len-1] = 'o'; // .c -> .o

        jb_compile_c(tc, exec->sources[i], object, &exec->cflags[0]);

        JBVectorPush(&object_files, object);
    }

    char *output_exec = jb_concat(exec->build_folder, jb_concat("/", exec->name));
    int needs_build = 0;

    JBArrayFor(&object_files) {
        if (jb_file_is_newer(object_files.data[index], output_exec)) {
            needs_build = 1;
            break;
        }
    }

    if (needs_build) {

        printf("[jb] link %s\n", output_exec);

        JBVector(char *) cmd = {0};

        JBVectorPush(&cmd, tc->cc);

        if (strstr(tc->cc, "clang") != NULL) {
            JBVectorPush(&cmd, "-fuse-ld=lld");
        }

        if (triplet && strstr(tc->cc, "clang") != NULL) {
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

        JBVectorPush(&cmd, "-o");
        JBVectorPush(&cmd, output_exec); // Leak

        JBArrayForEach(&object_files) {
            JBVectorPush(&cmd, *it);
        }

        // JBVectorPush(&cmd, "-L/usr/local/lib");

        // if (tc->triple.runtime == JB_ENUM(Darwin)) {
        //     JBVectorPush(&cmd, "-lSystem");
        // }
        // else if (tc->triple.runtime == JB_ENUM(GNU)) {
        //     JBVectorPush(&cmd, "-lc");
        // }

        JBVectorPush(&cmd, NULL);
        jb_run(cmd.data, __FILE__, __LINE__);
    }

    JBArrayForEach(&object_files) {
        free(*it);
    }

    free(object_files.data);
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

time_t jb_get_last_mod_time(const char *path) {
    if (!jb_file_exists(path))
        return 0;

    // TODO maybe JB_FAIL on failure ?
    struct stat st;
    stat(path, &st);

    return st.st_mtime;
}

int jb_file_is_newer(const char *source, const char *dest) {

    JB_ASSERT(jb_file_exists(source), "file not found: %s", source);

    time_t s = jb_get_last_mod_time(source);
    time_t d = jb_get_last_mod_time(dest);

    return s > d;
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

#endif // JOSH_BUILD_IMPL

#endif // JOSH_BUILD_H
