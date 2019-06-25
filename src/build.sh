mkdir -p ../bin

gcc -g -o bin/watchtogether src/SDL_platform.c `pkg-config --cflags --libs libavcodec libavutil libavformat` -Llib/ -lm -lswscale -O3 -fstack-protector-all -lSDL2 -no-pie -fno-omit-frame-pointer -DDEBUG
