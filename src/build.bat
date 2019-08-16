mkdir "..\bin"
gcc -ggdb -o bin/watchtogether src/SDL_platform.c -L../bin/lib/ -lm -lswscale -lSDL2 -llibavcodec -llibavutil -llibavformat -O3 -DDEBUG
