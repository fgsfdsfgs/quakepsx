TARGET = mkresmap
SRC = $(wildcard *.c) ../common/pak.c ../common/util.c
CC ?= gcc

../bin/$(TARGET).exe: $(SRC)
	$(CC) -o $@ $^

clean:
	rm -f ../bin/$(TARGET).exe

.PHONY: clean
