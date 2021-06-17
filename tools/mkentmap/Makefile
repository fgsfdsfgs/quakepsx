TARGET = mkentmap
SRC = $(wildcard *.c) ../common/pak.c
CC ?= gcc

../bin/$(TARGET).exe: $(SRC)
	$(CC) -o $@ $^

clean:
	rm -f ../bin/$(TARGET).exe

.PHONY: clean
