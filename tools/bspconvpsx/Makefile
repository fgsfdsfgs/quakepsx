TARGET = bspconvpsx
SRC = $(wildcard *.c) ../common/pak.c ../common/util.c ../common/libpsxav/adpcm.c ../common/libpsxav/cdrom.c
CC ?= gcc

../bin/$(TARGET).exe: $(SRC)
	$(CC) -Og -g -Werror -Wno-unused-result -o $@ $^ -lm

clean:
	rm -f ../bin/$(TARGET).exe

.PHONY: clean
