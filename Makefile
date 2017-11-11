#!/usr/bin/make -f

SRCDIR = src
INSTALLDIR= ~/emu80

CC = g++
CFLAGS = -c -Wall -std=c++11 `wx-config --cflags` `sdl2-config --cflags` -DPAL_SDL -DPAL_WX
LDFLAGS = `sdl2-config --libs` `wx-config --libs`

SRC = $(SRCDIR)/*.cpp
SRCSDL = $(SRCDIR)/sdl/*.cpp
SRCWX = $(SRCDIR)/wx/*.cpp
SRCLITE = $(SRCDIR)/lite/*.cpp

SOURCES = $(shell echo $(SRC)) $(shell echo $(SRCSDL)) $(shell echo $(SRCWX))
SOURCES_LITE = $(shell echo $(SRC)) $(shell echo $(SRCSDL)) $(shell echo $(SRCLITE))

OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS_LITE = $(SOURCES:.cpp=.o)

all: Emu80 Emu80lite

Emu80: $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

Emu80lite: $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS_LITE) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(OBJECTS_LITE)
	rm -f Emu80
	rm -f Emu80lite

install: Emu80 Emu80lite
	mkdir -p $(INSTALLDIR)
	cp Emu80 $(INSTALLDIR)
	cp Emu80lite $(INSTALLDIR)
	cp -r dist/* $(INSTALLDIR)
	cp COPYING.txt $(INSTALLDIR)
	cp whatsnew.txt $(INSTALLDIR)
	cp doc/* $(INSTALLDIR)
