#ifndef JOSH_BUILD_H
#define JOSH_BUILD_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define JB_IS_MACOS 1

void jb_run_string(const char *cmd, char *extra[], const char *file, int line);
void jb_run(char *argv[], const char *file, int line);

#define JB_CMD_ARRAY(...) (char **)(const char *const []){__VA_ARGS__ __VA_OPT__(,) NULL, }

#define JB_RUN_CMD(...) jb_run(JB_CMD_ARRAY(__VA_ARGS__), __FILE__, __LINE__)

#define _JB_RUN(cmd, ...) jb_run_string(#cmd, JB_CMD_ARRAY(__VA_ARGS__), __FILE__, __LINE__)
#define JB_RUN(cmd, ...) _JB_RUN(cmd __VA_OPT__(,) __VA_ARGS__)

typedef struct {
    const char *name;
    const char **sources;

    const char *build_folder;

    const char **cflags;
} JBExecutable;

char *jb_concat(const char *lhs, const char *rhs);
char *jb_copy_string(const char *str);
char *jb_format_string(const char *fmt, ...);

const char *jb_filename(const char *path);

void jb_build(JBExecutable *exec);

int jb_file_exists(const char *path);

char *jb_getcwd();

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


#ifdef JOSH_BUILD_IMPL

#include <stdarg.h>

#include <unistd.h>

#include <sys/wait.h>
#include <sys/param.h>

void jb_run(char *argv[], const char *file, int line) {

	for (int i = 0; argv[i]; i++) {
		printf("%s ", argv[i]);
	}

	printf("\n");

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

int jb_iswhitespace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v';
}

void jb_run_string(const char *cmd, char *extra[], const char *file, int line) {
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

void jb_build(JBExecutable *exec) {

    char *object_folder = jb_concat(exec->build_folder, "/object/");

    JB_RUN_CMD("mkdir", "-p", exec->build_folder);
    JB_RUN_CMD("mkdir", "-p", object_folder);

    JBVector(char *) object_files = {0};

    for (int i = 0; exec->sources[i]; i++) {
        const char *filename = jb_filename(exec->sources[i]);
        char *object = jb_concat(object_folder, filename);
        size_t len = strlen(object);

        object[len-1] = 'o'; // .c -> .o

        JBVector(char *) cmd = {0};

        JBVectorPush(&cmd, "cc");

        for (int i = 0; exec->cflags && exec->cflags[i]; i++) {
        	JBVectorPush(&cmd, (char *)exec->cflags[i]);
        }

        JBVectorPush(&cmd, "-o");
        JBVectorPush(&cmd, object);
        JBVectorPush(&cmd, "-c");
        JBVectorPush(&cmd, (char *)exec->sources[i]);

        JBVectorPush(&cmd, NULL);
        jb_run(cmd.data, __FILE__, __LINE__);

        free(cmd.data);

        JBVectorPush(&object_files, object);
    }

    {
        JBVector(char *) cmd = {0};

        JBVectorPush(&cmd, "ld");
        JBVectorPush(&cmd, "-o");
        JBVectorPush(&cmd, jb_concat(exec->build_folder, jb_concat("/", exec->name))); // Leak

        JBArrayForEach(&object_files) {
            JBVectorPush(&cmd, *it);
        }

        JBVectorPush(&cmd, "-syslibroot");
        JBVectorPush(&cmd, "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk");

        JBVectorPush(&cmd, "-L/usr/local/lib");
        JBVectorPush(&cmd, "-lSystem");

        JBVectorPush(&cmd, NULL);
        jb_run(cmd.data, __FILE__, __LINE__);
    }

    JBArrayForEach(&object_files) {
        free(*it);
    }
    free(object_files.data);
    free(object_folder);
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

#endif // JOSH_BUILD_IMPL

#endif // JOSH_BUILD_H
