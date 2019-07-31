IDIR=./src
LDIR=../lib
BDIR=bin
APPNAME=watchtogether
CC=gcc
CFLAGS=-I$(IDIR) -O0 -L$(LDIR) -no-pie -fno-omit-frame-pointer 
LDFLAGS=-lSDL2 `pkg-config --cflags --libs libavcodec libavutil libavformat libswscale`
DBGFLAGS=-DDEBUG

DEPS = $(wildcard src/*.h)

src = $(wildcard src/*.c)
obj = $(src:.c=.o)
inc = $(wildcard src/*.h)

watchtogether:
	$(CC) -o $(BDIR)/$(APPNAME) src/SDL_platform.c $(CFLAGS) $(LDFLAGS)

.PHONY: debug
debug:
	$(CC) -o $(BDIR)/$(APPNAME) src/SDL_platform.c $(CFLAGS) $(LDFLAGS) $(DBGFLAGS)

.PHONY: clean
clean:
	rm -f watchtogether