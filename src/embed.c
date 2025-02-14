
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

int main(int argc, char **argv) {
	char *text = read_file(argv[1]);

	FILE *out = fopen(argv[2], "wb");

	while (*text) {
		fprintf(out, "0x%X", *text);
		text++;

		if (*text != 0)
			fprintf(out, ", ");
	}

	fputc('\n', out);

	fclose(out);

	return 0;
}
