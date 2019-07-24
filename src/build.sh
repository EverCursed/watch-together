mkdir -p ../bin

gcc -g -o bin/watchtogether src/SDL_platform.c `pkg-config --cflags --libs libavcodec libavutil libavformat` -O3 -Llib/ -lm -lswscale -fstack-protector-all -lSDL2 -no-pie -fno-omit-frame-pointer -DDEBUG #-DSUPPRESS_WARN
