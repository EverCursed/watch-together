APPNAME:=watchtogether
CC:=gcc
OUTPUT_DIR:=bin
OUTPUT_FILE:=$(OUTPUT_DIR)/$(APPNAME)

LIB_DIR := -Lbin
LIB_FLAGS := $(LIB_DIR) -lSDL2 -lSDL2_net -lavcodec -lavutil -lavformat -lswscale

src := $(wildcard ./src/*.c) $(wildcard ./src/*/*.c)
obj = $(src:.c=.o)
inc := $(wildcard src/*.h) $(wildcard ./src/*/*.h)

ifeq ($(OS),Windows_NT)
	RM := del
#	INCLUDE_DIR := -ID:\TDM-GCC-64\include
	LIB_FLAGS += -lKernel32
else
	src := $(wildcard src/*.c)
	obj := $(src:.c=.o)

	INCLUDE_DIR := -I/usr/local/include
	LIB_FLAGS += -lXv -lX11 -lXext
endif

DBGFLAGS := -DDEBUG -fno-omit-frame-pointer
CFLAGS := -Wall $(INCLUDE_DIR) -ggdb -O3

watchtogether: $(obj) $(inc)
	$(CC) -MD -MF $(CFLAGS) $(obj) -o $(OUTPUT_FILE) $(LIB_FLAGS)

%.o: %.c $(inc)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:
debug: CFLAGS+= $(DBGFLAGS)
debug: watchtogether

errors: CFLAGS+= -DERRORS_ONLY
errors: watchtogether

clean:
	$(DELETE_COMMAND)