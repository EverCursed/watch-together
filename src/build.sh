mkdir -p ../bin

gcc `pkg-config --cflags gtk+-3.0` -o ../bin/watchtogether watchtogether.c settings.c `pkg-config --libs gtk+-3.0` 
