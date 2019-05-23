mkdir "..\bin"
gcc -ggdb -o bin/watchtogether src/deb-watchtogether-v2.c -L../bin/lib/ -lm -lswscale -lSDL2 -llibavcodec -llibavutil -llibavformat -O3 -DDEBUG
