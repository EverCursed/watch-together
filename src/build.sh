#mkdir -p ../bin

gcc -ggdb -o bin/watchtogether src/deb-watchtogether-v2.c `pkg-config --cflags --libs libavcodec libavutil libavformat` -Llib/ -lm -lswscale -lSDL2 -no-pie -O3 -DDEBUG