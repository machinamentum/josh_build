
#include <stdio.h>
#include <stdlib.h>

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

void generate_embed(const char *input, const char *output) {
	char *text = read_file(input);

	FILE *out = fopen(output, "wb");

	while (*text) {
		fprintf(out, "0x%X", *text);
		text++;

		if (*text != 0)
			fprintf(out, ", ");
	}

	fputc('\n', out);

	fclose(out);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		printf("Usage: embed <input> <output>");
		return 1;
	}

	generate_embed(argv[1], argv[2]);
	return 0;
}
