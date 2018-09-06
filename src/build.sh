mkdir ../bin
gcc `pkg-config --cflags gtk+-3.0` -o ../bin/watchtogether deb-watchtogether.c `pkg-config --libs gtk+-3.0`
