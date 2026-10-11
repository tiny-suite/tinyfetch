CC = gcc
BIN = tinyfetch
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

CFLAGS ?= -O2
CFLAGS += -std=c11 -Wall -Wextra -D_DEFAULT_SOURCE

all: $(BIN)

$(BIN): tinyfetch.c
	$(CC) $(CFLAGS) -o $@ $<

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 $(BIN) $(DESTDIR)$(BINDIR)/$(BIN)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)

clean:
	rm -f $(BIN)

.PHONY: all install uninstall clean
