CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDLIBS = -lm

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

SRCDIR = src
BUILDDIR = build

PROG = ding

TARGET = $(BUILDDIR)/$(PROG)
SRC = $(SRCDIR)/ding.c
MAIN = $(SRCDIR)/main.c
STATISTICS = $(SRCDIR)/statistics.c
STATISTICS_H = include/statistics.h
TWRITER = $(SRCDIR)/twriter.c
TWRITER_H = include/twriter.h

.PHONY: all clean install

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(SRC) $(MAIN) $(STATISTICS) $(STATISTICS_H) $(TWRITER) $(TWRITER_H) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(SRC) $(MAIN) $(STATISTICS) $(TWRITER) -o $(TARGET) $(LDLIBS)

clean:
	rm -rf $(BUILDDIR)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(PROG)


