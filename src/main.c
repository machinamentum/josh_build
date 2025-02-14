
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
        const char *preamble = "#define JOSH_BUILD_IMPL\n";
        fwrite(preamble, 1, strlen(preamble), out);
        fwrite(josh_build_src, 1, strlen(josh_build_src), out);
        fputc('\n', out);
        fwrite(build_source, 1, strlen(build_source), out);
        fclose(out);

        JBExecutable josh = {"josh_builder"};
        josh.sources = (const char *[]){josh_builder_file, NULL};
        josh.build_folder = "build";
        jb_build(&josh);

        JB_RUN_CMD("./build/josh_builder");

        remove(josh_builder_file);
        remove("./build/josh_builder");
        return 0;
    }

    if (strcmp(argv[1], "library") == 0) {
        puts(josh_build_src);
        return 0;
    }

    usage();
    return 0;
}
