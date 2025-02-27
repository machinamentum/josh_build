
int main(int argc, char *argv[]) {
	{
        JBExecutable josh = {"josh"};
        josh.sources = (const char *[]){"src/main.c", NULL};
        josh.build_folder = "build";
        jb_build_exe(&josh);
    }

    JB_RUN(./build/josh);
	return 0;
}
