
#define JOSH_BUILD_IMPL
#include "josh_build.h"

void usage() {
    printf("Usage: josh [option]\n");
    printf("    build   : build josh.build file\n");
    printf("    cc      : invoke C compiler; use -target <triple> to use cross compiler\n");
    printf("    init    : generate project template in current working directory\n");
    printf("    library : dump josh build header library\n");
    printf("\n");
}

const char _jb_josh_build_src[] = {
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
        if (!jb_file_exists("josh.build")) {
            printf("josh.build file not found\n");
            exit(1);
        }

        josh_build("josh.build");
        
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
        puts(_jb_josh_build_src);
        puts("const char _jb_josh_build_src[] = {\n");

        const char *text = _jb_josh_build_src;
        while (*text) {
            printf("0x%X", *text);

            text += 1;
            if (*text)
                printf(", ");
        }
        puts("\n    , 0\n");

        puts("};\n");
        return 0;
    }

    if (strcmp(argv[1], "cc") == 0) {
        JBToolchain *tc = jb_native_toolchain();
        const char *target_opt = "-target";
        for (int i = 2; i < (argc - 1); i++) {
            if (strcmp(argv[i], target_opt) == 0) {
                char *triple = argv[i+1];
                tc = jb_find_toolchain_by_triple(triple);

                if (!tc) {
                    JB_FAIL("Could not find toolchain for %s", triple);
                }
                i += 1;
            }
        }

        if (!tc->cc)
            JB_FAIL("Toolchain missing C compiler");

        JBVector(char *) cmd = {0};

        JBVectorPush(&cmd, tc->cc);

        // TODO check if gcc is our compiler and filter out -target <triple>
        for (int i = 2; i < argc; i++) {
            JBVectorPush(&cmd, argv[i]);
        }

        jb_run(cmd.data, __FILE__, __LINE__);

        return 0;
    }

    usage();
    return 0;
}
