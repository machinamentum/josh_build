
#define JOSH_BUILD_IMPL
#include "josh_build.h"

void usage() {
    printf("Usage: josh [option]\n");
    printf("    build   : build josh.build file\n");
    printf("    library : dump josh build header library\n");
    printf("\n");
}

const char josh_build_src[] = {
#include "josh_build_embed.h"
    , 0
};

const char josh_build_init_josh_build[] = {
#include "init_josh_build_embed.h"
    , 0
};

const char josh_build_init_src_main[] = {
#include "init_src_main_embed.h"
    , 0
};


char *read_file(const char *path) {
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

    return out;
}

void write_file(const char *path, const char *text) {
     FILE *f = fopen(path, "wb");

    if (!f)
        return;

    fwrite(text, 1, strlen(text), f);
    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 0;
    }

    if (strcmp(argv[1], "build") == 0) {

        // Look for josh.build in current directory

        char *build_source = read_file("josh.build");

        if (!build_source) {
            printf("Could not read josh.build in current directory\n");
            return -1;
        }

        const char *josh_builder_file = "josh.build.c";

        FILE *out = fopen(josh_builder_file, "wb");
        const char *preamble = "#define JOSH_BUILD_IMPL\n#line 1 \"josh_build.h\"\n";
        fwrite(preamble, 1, strlen(preamble), out);
        fwrite(josh_build_src, 1, strlen(josh_build_src), out);
        fputc('\n', out);
        fputs("#line 1 \"josh.build.c\"\n", out);
        fwrite(build_source, 1, strlen(build_source), out);
        fclose(out);

        JBExecutable josh = {"josh_builder"};
        josh.sources = (const char *[]){josh_builder_file, NULL};
        josh.cflags = (const char *[]){"-Wno-unknown-escape-sequence", NULL};
        josh.build_folder = "build";
        jb_build(&josh);

        JB_RUN_CMD("./build/josh_builder");

        remove(josh_builder_file);
        // remove("./build/josh_builder");
        return 0;
    }

    if (strcmp(argv[1], "init") == 0) {
        JB_RUN(mkdir -p src);

        if (jb_file_exists("josh.build")) {
            printf("josh.build already exists. Aborting\n");
            exit(1);
        }

        write_file("josh.build", josh_build_init_josh_build);

        if (jb_file_exists("src/main.c")) {
            printf("src/main.c already exists. Aborting\n");
            exit(1);
        }

        write_file("src/main.c", josh_build_init_src_main);
        return 0;
    }

    if (strcmp(argv[1], "library") == 0) {
        puts(josh_build_src);
        return 0;
    }

    usage();
    return 0;
}
