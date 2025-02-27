
int main(int argc, char *argv[]) {

    // Optional: use josh_parse_arguments to handle common option switches (such as --verbose to show executed commands)
    argv = josh_parse_arguments(argc, argv);
    argc = jb_string_array_count(argv);

	{
        JBExecutable josh = {"josh"};
        josh.sources = JB_STRING_ARRAY("src/main.c");
        josh.build_folder = "build";
        jb_build_exe(&josh);
    }

    JB_RUN(./build/josh);
	return 0;
}
