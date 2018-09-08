OBJS=assemble.o lexer.o parse_core.o parse_main.o parse_preprocess.o tokens.o labels.o opcodes.o utility.o
TARGET=glulx-assemble

CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -g -DDEBUG

all: $(TARGET) tests demos

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)


tests: test_utility test_parse_core

test_parse_core: tests/test.o tests/parse_core.o parse_core.o tokens.o utility.o
	$(CC) tests/test.o tests/parse_core.o parse_core.o tokens.o utility.o -o test_parse_core
	./test_parse_core
test_utility: tests/test.o tests/utility.o utility.o
	$(CC) tests/test.o tests/utility.o utility.o -o test_utility
	./test_utility


demos: minimal.ulx basic.ulx complex.ulx

minimal.ulx: demos/minimal.ga $(TARGET)
	cd demos && ../$(TARGET) minimal.ga ../minimal.ulx

basic.ulx: demos/basic.ga $(TARGET)
	cd demos && ../$(TARGET) basic.ga ../basic.ulx

complex.ulx: demos/complex.ga demos/glk.ga $(TARGET)
	cd demos && ../$(TARGET) complex.ga ../complex.ulx

clean:
	$(RM) *.o $(TARGET) *.ulx

.PHONY: all demos clean tests run_tests
