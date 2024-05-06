CC=gcc
CFLAGS=-Wall -std=c2x
.PHONY:all


all: CXXFLAGS+=-O3
all: CFLAGS+=-O3
all: test

debug: CXXFLAGS+=-g -O0
debug: CFLAGS+=-g -O0
debug: clean test

test: test.o vbyte64.o
	$(CC) -o $@ $^  $(CFLAGS) $(EXTFLAGS)


%.o: %.c
	$(CC) -o $@ -c $<  $(CFLAGS) $(EXTFLAGS)

clean:
	rm -rf *.o test
