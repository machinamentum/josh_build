
#define JOSH_BUILD_IMPL
#include "josh_build.h"

int main(int argc, char **argv) {
	if (argc < 3) {
		printf("Usage: embed <input> <output>");
		return 1;
	}

	jb_generate_embed(argv[1], argv[2]);
	return 0;
}
