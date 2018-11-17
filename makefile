OBJS=src/assemble.o src/lexer.o src/parse_core.o src/parse_main.o \
	 src/parse_preprocess.o src/tokens.o src/labels.o src/opcodes.o \
	 src/utility.o src/strings.o
TARGET=glulx-assemble

CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -g -DDEBUG

all: $(TARGET) tests demos

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

demos:
	cd demos && $(MAKE)

clean:
	$(RM) src/*.o tests/*.o $(TARGET) test_parse_core test_utility test_tokens
	cd demos && $(MAKE) clean

tests: test_utility test_parse_core test_tokens test_vbuffer

test_vbuffer: src/vbuffer.o tests/test.o tests/vbuffer.o
	$(CC) src/vbuffer.o tests/test.o tests/vbuffer.o -o test_vbuffer
	./test_vbuffer
test_parse_core: tests/test.o tests/parse_core.o src/parse_core.o src/tokens.o src/utility.o
	$(CC) tests/test.o tests/parse_core.o src/parse_core.o src/tokens.o src/utility.o -o test_parse_core
	./test_parse_core
test_utility: tests/test.o tests/utility.o src/utility.o
	$(CC) tests/test.o tests/utility.o src/utility.o -o test_utility
	./test_utility
test_tokens: tests/test.o tests/tokens.o src/tokens.o src/utility.o
	$(CC) tests/test.o tests/tokens.o src/tokens.o src/utility.o -o test_tokens
	./test_tokens

.PHONY: all demos clean tests run_tests
