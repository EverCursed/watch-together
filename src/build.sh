mkdir -p ../bin

gcc -g -o bin/watchtogether src/deb-watchtogether-v2.c -lSDL2 -no-pie 
