set -e
rm -rf build
mkdir -p build
cc -Wall -Isrc -g -o embed src/embed.c
./embed src/josh_build.h src/josh_build_embed.h
./embed src/init_josh_build.c src/init_josh_build_embed.h
./embed src/init_src_main.c src/init_src_main_embed.h
rm embed
cc -Wall -Isrc -Itools -g -o build/josh src/main.c
rm src/josh_build_embed.h
rm src/init_josh_build_embed.h
rm src/init_src_main_embed.h
