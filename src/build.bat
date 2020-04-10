@echo off

SET main_file=watchtogether.c

IF NOT EXIST "..\bin" (
	mkdir "..\bin"
)

gcc -m64 -O2 -g -fno-omit-frame-pointer -o ../bin/watchtogether.exe %main_file% -L../lib/ -lswscale -lSDL2 -lSDL2_net -lSDL2_ttf -llibavcodec -llibavutil -llibavformat -DDEBUG
