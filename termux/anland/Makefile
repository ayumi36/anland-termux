CC ?= cc
CFLAGS ?= -O2
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

PREFIX ?= /data/data/com.termux/files/usr
BINDIR ?= $(PREFIX)/bin
COMPATIBLE_BRIDGE ?= anland-compatible

SOURCES = anland.c common/socket_utils.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean install

all: anland

anland: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -Wextra -Wpedantic -std=c11 -c -o $@ $<

install: anland
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 0755 anland "$(DESTDIR)$(BINDIR)/anland"
	install -m 0755 "$(COMPATIBLE_BRIDGE)" "$(DESTDIR)$(BINDIR)/$(COMPATIBLE_BRIDGE)"

clean:
	rm -f anland $(OBJECTS)
