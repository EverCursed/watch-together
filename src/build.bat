@echo off

SET main_file=watchtogether.c

IF NOT EXIST "..\bin" (
	mkdir "..\bin"
)

gcc -ggdb -o ../bin/watchtogether.exe %main_file% -L../bin/lib/ -lm -lswscale -lSDL2 -lSDL2_net -llibavcodec -llibavutil -llibavformat -O3 -DDEBUG
