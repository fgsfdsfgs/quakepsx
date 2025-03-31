TARGET = bspconvpsx
SRC = $(wildcard *.c) ../common/pak.c libpsxav/adpcm.c libpsxav/cdrom.c
CC ?= gcc

../bin/$(TARGET).exe: $(SRC)
	$(CC) -Og -g -Werror -Wno-unused-result -o $@ $^ -lm

clean:
	rm -f ../bin/$(TARGET).exe

.PHONY: clean
