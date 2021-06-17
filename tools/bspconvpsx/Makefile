TARGET = bspconvpsx
SRC = $(wildcard *.c) ../common/pak.c
CC ?= gcc

../bin/$(TARGET).exe: $(SRC)
	$(CC) -Og -g -o $@ $^

clean:
	rm -f ../bin/$(TARGET).exe

.PHONY: clean
