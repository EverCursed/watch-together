mkdir -p ../bin

gcc -ggdb -o ../bin/watchtogether deb-watchtogether-v2.c -lSDL2 -no-pie #`pkg-config --cflags --libs 
