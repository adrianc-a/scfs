CC = gcc

CFLAGS_COMMON = -Wall -std=c99 `pkg-config libcurl fuse --cflags`

LIBS = `pkg-config libcurl fuse --libs` -lm

CFLAGS_NDBG = $(CFLAGS_COMMON) -DNDEBUG

CFLAGS_DBG = $(CFLAGS_COMMON) -g

MAIN_EXECUTABLE = scfs
TEST_EXECUTABLE = test

SOURCES = arr.c \
		  buf.c \
		  cJSON.c \
		  http.c \
		  sc.c
HEADERS = $(SOURCES:.c=.h) utils.h
OBJECTS = $(SOURCES:.c=.o)

MAIN_SOURCES = $(SOURCES) scfs.c
TEST_SOURCES = $(SOURCES) test.c

MAIN_OBJECTS = $(OBJECTS) scfs.o
TEST_OBJECTS = $(OBJECTS) test.o

.phony: clean tags


all: $(MAIN_OBJECTS) $(MAIN_SOURCES) $(HEADERS)
	echo "making release build"
	$(CC) -o $(MAIN_EXECUTABLE) $(MAIN_OBJECTS) $(LIBS) $(CFLAGS_NDBG)

test: $(TEST_OBJECTS) $(TEST_SOURCES) $(HEADERS)
	echo "making test build"
	$(CC) -o $(TEST_EXECUTABLE) $(TEST_OBJECTS) $(LIBS) $(CFLAGS_DBG)

debug: $(OBJECTS) $(SOURCES) $(HEADERS)
	echo "making debug build"
	$(CC) -o $(MAIN_EXECUTABLE) $(MAIN_OBJECTS) $(LIBS) $(CLFAGS_DBG)

.c.o:
	$(CC) $(CFLAGS_DBG) $(LIBS) -c $< -o $@

clean:
	$(RM) *.o scfs test

tags:
	ctags --c-kinds=+defgmpstux -R

