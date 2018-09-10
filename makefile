OBJS=assemble.o lexer.o parse_core.o parse_main.o parse_preprocess.o tokens.o labels.o opcodes.o utility.o
TARGET=glulx-assemble

CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -g -DDEBUG

all: $(TARGET) tests demos

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

demos:
	cd demos && $(MAKE)

clean:
	$(RM) *.o tests/*.o $(TARGET) test_parse_core test_utility test_tokens
	cd demos && $(MAKE) clean

tests: test_utility test_parse_core test_tokens

test_parse_core: tests/test.o tests/parse_core.o parse_core.o tokens.o utility.o
	$(CC) tests/test.o tests/parse_core.o parse_core.o tokens.o utility.o -o test_parse_core
	./test_parse_core
test_utility: tests/test.o tests/utility.o utility.o
	$(CC) tests/test.o tests/utility.o utility.o -o test_utility
	./test_utility
test_tokens: tests/test.o tests/tokens.o tokens.o utility.o
	$(CC) tests/test.o tests/tokens.o tokens.o utility.o -o test_tokens
	./test_tokens

.PHONY: all demos clean tests run_tests
