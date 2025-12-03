CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include `pkg-config --cflags pangocairo cairo libqrencode`
LIBS = `pkg-config --libs pangocairo cairo libqrencode` -ljson-c

# Check if we're cross-compiling for Raspberry Pi or building locally
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    # Check if running on Raspberry Pi
    ifeq ($(shell uname -m),armv7l)
        CFLAGS += -march=armv7-a -mfpu=neon-vfpv4
    endif
endif

TARGET = poc_printer
SOURCES = src/main.c \
          src/block_manager.c \
          src/blocks/text_block.c \
          src/blocks/qr_block.c \
          src/blocks/image_block.c \
          src/blocks/feed_block.c \
          src/blocks/hr_block.c \
          src/blocks/table_block.c \
          src/bitmap_utils.c \
          src/escpos_utils.c \
          src/printer_comm.c \
          src/image_utils.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

clean:
	rm -f $(TARGET) $(OBJECTS)

install-deps:
	sudo apt update
	sudo apt install -y build-essential pkg-config \
		libcairo2-dev libpango1.0-dev libpangocairo-1.0-0 \
		libqrencode-dev libcjson-dev libpng-dev fonts-manjari

.PHONY: all clean install-deps