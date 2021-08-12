CC=/usr/bin/g++
CFLAGS= -Wall -Iinclude
SOURCE=$(shell find source/ -name *.cpp)

all:
	$(CC) $(CFLAGS) $(SOURCE) -o build/zserver

clean :
	rm -rf build/*
