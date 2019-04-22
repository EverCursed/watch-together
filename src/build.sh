mkdir -p ../bin

gcc -g -o bin/watchtogether src/deb-watchtogether-v2.c `pkg-config --cflags --libs libavcodec libavutil libavformat` -lm -lSDL2 -no-pie
