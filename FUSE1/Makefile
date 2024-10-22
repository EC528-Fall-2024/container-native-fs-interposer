CC := gcc
CFLAGS := -g -Wall -Werror
LDFLAGS := -lfuse3

TARGET := FUSE_ll

SRC := FUSE_ll.c
OBJ := $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

%.o: %.c passthrough_helper.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) $(TARGET)
