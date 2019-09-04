APPNAME:=watchtogether
CC:=gcc
OUTPUT_DIR:=bin
OUTPUT_FILE:=$(OUTPUT_DIR)/$(APPNAME)

LIB_FLAGS := $(LIB_DIR) -lSDL2 -lavcodec -lavutil -lavformat -lswscale

src := $(wildcard ./src/*.c)
obj = $(src:.c=.o)
inc := $(wildcard src/*.h)

ifeq ($(OS),Windows_NT)
	RM := del
#	LIB_DIR := -LD:\TDM-GCC-64\lib
#	INCLUDE_DIR := -ID:\TDM-GCC-64\include
	LIB_FLAGS += -lKernel32
	DELETE_COMMAND := $(RM) "src/*.o"
else
	src := $(wildcard src/*.c)
	obj := $(src:.c=.o)

	LIB_DIR := -L/usr/local/lib
	INCLUDE_DIR := -I/usr/local/include
	LIB_FLAGS += -lXv -lX11 -lXext
	DELETE_COMMAND = $(RM) $(obj)
endif

DBGFLAGS := -DDEBUG -fno-omit-frame-pointer
CFLAGS := -Wall $(INCLUDE_DIR) -g -O3

watchtogether: $(obj)
	$(CC) -MD -MF $(CFLAGS) $^ -o $(OUTPUT_FILE) $(LIB_FLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:
debug: CFLAGS+= $(DBGFLAGS)
debug: watchtogether

clean:
	$(DELETE_COMMAND)