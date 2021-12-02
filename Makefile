TARGET		= quakepsx
TYPE			= ps-exe

LTOFLAGS	?= -flto

CFLAGS		:= -Inugget/psyq/include -Isrc
ASFLAGS		:= -Inugget/psyq/include -Isrc
CFLAGS		+= -Wall -Wno-missing-braces -mno-check-zero-division

LDFLAGS		:= -Lnugget/psyq/lib -Wl,--start-group -lcard -lapi -lc2 -lcd -letc -lgte -lgpu -lspu -Wl,--end-group
LDFLAGS		+= $(LTOFLAGS)

SRCDIR		= src
SRCSUB		= game

SRCDIRS		= $(foreach dir,$(SRCSUB),$(SRCDIR)/$(dir)) $(SRCDIR)
CFILES		= $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
CPPFILES 	= $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.cpp))
AFILES		= $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.s))
SRCS			= $(CFILES) $(CPPFILES) $(AFILES)
SRCS			+= nugget/common/crt0/crt0.s

default: all

iso: $(TARGET).iso

$(TARGET).iso: all
	mkpsxiso -y -q iso.xml

.PHONY: iso

include nugget/common.mk

CPPFLAGS_Release	+= -O3 $(LTOFLAGS)
LDFLAGS_Release		+= -O3 $(LTOFLAGS)
