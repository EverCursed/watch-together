mkdir -p ../bin

make clean
make debug


#gcc -g -o bin/watchtogether src/SDL_platform.c `pkg-config --cflags --libs libavcodec libavutil libavformat libswscale` -O3 -Llib/ -Wl,--no-as-needed -l:libprofiler.so -Wl,--as-needed -fstack-protector-all -lSDL2 -no-pie -fno-omit-frame-pointer -DDEBUG #-DWITHGPERFTOOLS #-DSUPPRESS_WARN
