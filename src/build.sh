mkdir -p ../bin

gcc -ggdb -o bin/watchtogether src/SDL_platform.c `pkg-config --cflags --libs libavcodec libavutil libavformat` -Llib/ -lm -lswscale -lSDL2 -no-pie -O3 -DDEBUG