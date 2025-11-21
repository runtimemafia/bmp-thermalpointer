CC = gcc
CFLAGS = -Wall -Wextra -O2 `pkg-config --cflags pangocairo cairo libqrencode`
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
SOURCES = src/poc_printer.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

clean:
	rm -f $(TARGET)

install-deps:
	sudo apt update
	sudo apt install -y build-essential pkg-config \
		libcairo2-dev libpango1.0-dev libpangocairo-1.0-0 \
		libqrencode-dev libcjson-dev fonts-manjari

.PHONY: all clean install-deps