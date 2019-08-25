APPNAME:=watchtogether
CC:=gcc
OUTPUT_DIR:=bin
OUTPUT_FILE:=$(OUTPUT_DIR)/$(APPNAME)

src := $(wildcard src/*.c)
obj := $(src:.c=.o)
#inc := $(wildcard src/*.h)

LIB_DIR := -L/usr/local/lib
INCLUDE_DIR := -I/usr/local/include

DBGFLAGS := -DDEBUG -fno-omit-frame-pointer
LIB_FLAGS := $(LIB_DIR) -lSDL2 -lavcodec -lavutil -lavformat -lswscale -lXv -lX11 -lXext
#`pkg-config --cflags --libs libavcodec libavutil libavformat libswscale` #-l:libavformat.so.58 -lm -lz -l:libavcodec.so -pthread -l:libswscale.so -l:libavutil.so -l:libswresample.so 
CFLAGS := -Wall $(INCLUDE_DIR) -O3 -no-pie

all: clean watchtogether

watchtogether: $(obj)
	$(CC) $(CFLAGS) $^ -o $(OUTPUT_FILE) $(LIB_FLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:
debug: CFLAGS+= $(DBGFLAGS)
debug: watchtogether

clean:
	$(RM) $(obj)