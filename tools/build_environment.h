
#ifndef JOSH_TOOLS_BUILD_ENVIRONMENT_H
#define JOSH_TOOLS_BUILD_ENVIRONMENT_H

#define DOWNLOAD_AND_EXTRACT_GENERIC(lib, version, link) \
DOWNLOAD_GENERIC(STR(lib-version.tar.COMPRESSION), STR(link)); \
if (!jb_file_exists(STR(lib-version))) UNTAR(ARCHIVE(lib-version.tar.COMPRESSION))

#define DOWNLOAD_GENERIC(name, link) \
if (!jb_file_exists(ARCHIVE_STRING(name))) CURL(link, ARCHIVE_STRING(name))

#define CURL(link, path) JB_RUN_CMD("curl", link, "--output", path)
#define UNTAR(path) JB_RUN_CMD("tar", "-xf", path)

#define XSTR(x) #x
#define STR(x) XSTR(x)

#define WORKSPACE PATH(context.root, toolchain-build)
#define ROOT context.root
#define TOOLCHAIN(...) PATH(ROOT, toolchains/__VA_ARGS__)
#define TOOLCHAIN_TARGET(target, tokens) FMT("%s%s/%s", TOOLCHAIN(), target, STR(tokens))

#define NATIVE_TOOLS(tokens) PATH(WORKSPACE, native-tools/tokens)
#define NATIVE_TOOLS_STRING(string) FMT("%s/native-tools/%s", WORKSPACE, string)
#define NATIVE_BUILD   PATH(WORKSPACE, native-build)

#define ARCHIVE_STRING(string) FMT("%s/archive/%s", WORKSPACE, string)
#define ARCHIVE(tokens) PATH(WORKSPACE, archive/tokens)

#define TARGET_BUILD(tokens) PATH(WORKSPACE, target-build/tokens)

#define mkdir(dir) JB_RUN(mkdir -p, dir)
#define PATH(var, tokens) jb_concat(var, "/" #tokens)

#define FMT(fmt, ...) jb_format_string(fmt, __VA_ARGS__)

#endif
