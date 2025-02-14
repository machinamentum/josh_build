#ifndef JOSH_BUILD_H
#define JOSH_BUILD_H

#include <unistd.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


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


void jb_run(char *argv[]);

#define JB_RUN_CMD(...) jb_run((char **)(const char *const []){__VA_ARGS__, NULL, })

typedef struct {
    const char *name;
    const char **sources;


    const char *build_folder;
} JBExecutable;

char *jb_concat(const char *lhs, const char *rhs);

const char *jb_filename(const char *path);

void jb_build(JBExecutable *exec);

#ifdef JOSH_BUILD_IMPL

void jb_run(char *argv[]) {

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
    }
    else {
        // child
        execvp(argv[0], argv);
        printf("Could not run %s\n", argv[0]);
        exit(-1);
    }
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

    JBVector(char *) object_files = {};

    for (int i = 0; exec->sources[i]; i++) {
        const char *filename = jb_filename(exec->sources[i]);
        char *object = jb_concat(object_folder, filename);
        size_t len = strlen(object);

        object[len-1] = 'o'; // .c -> .o

        JB_RUN_CMD("cc", "-o", object, "-c", exec->sources[i]);

        JBVectorPush(&object_files, object);
    }

    {
        JBVector(char *) cmd = {};

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

        jb_run(cmd.data);
    }

    JBArrayForEach(&object_files) {
        free(*it);
    }
    free(object_files.data);
    free(object_folder);
}

#endif // JOSH_BUILD_IMPL

#endif // JOSH_BUILD_H
