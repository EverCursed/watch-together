APPNAME=watchtogether
CC=gcc

OUTPUT_DIR=bin

ifeq ($(OS),Windows_NT)
	LIB_FLAGS=
else
	INCLUDE_DIR=/usr/local/include
	LIB_DIR=.lib
	LIB_FLAGS=-lSDL2 `pkg-config --cflags --libs libavcodec libavutil libavformat libswscale` #-l:libavformat.so.58 -lm -lz -l:libavcodec.so -pthread -l:libswscale.so -l:libavutil.so -l:libswresample.so -lXv -lX11 -lXext 
endif

CFLAGS=-I$(INCLUDE_DIR) -O3 -no-pie

DBGFLAGS=-DDEBUG -fno-omit-frame-pointer

src = $(wildcard src/*.c)
obj = $(src:.c=.o)
inc = $(wildcard src/*.h)

watchtogether:
	$(CC) -o $(OUTPUT_DIR)/$(APPNAME) src/SDL_platform.c $(CFLAGS) $(LIB_FLAGS)

.PHONY: debug
debug:
	$(CC) -o $(OUTPUT_DIR)/$(APPNAME) src/SDL_platform.c $(CFLAGS) $(LIB_FLAGS) $(DBGFLAGS)

.PHONY: clean
clean:
	rm -f watchtogether $(obj)
