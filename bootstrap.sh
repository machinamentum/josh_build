set -e
cc -Wall -pedantic -Wno-format-security -Wno-fixed-enum-extension -std=c2x -Isrc -g -o embed src/embed.c
./embed src/josh_build.h src/josh_build_embed.h
rm embed
cc -Wall -pedantic -Wno-format-security -Wno-fixed-enum-extension -std=c2x -Isrc -g -o josh src/main.c
rm src/josh_build_embed.h
