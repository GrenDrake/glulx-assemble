OBJS=assemble.o assem_tokens.o assem_build.o assem_labels.o assem_opcodes.o utility.o
TARGET=glulx-assemble

CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -g -DDEBUG

all: $(TARGET) demos

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

demos: basic.ulx #complex.ulx

basic.ulx: demos/basic.ga $(TARGET)
	cd demos && ../$(TARGET) basic.ga ../basic.ulx

complex.ulx: demos/complex.ga demos/glk.ga $(TARGET)
	cd demos && ../$(TARGET) complex.ga ../complex.ulx

clean:
	$(RM) *.o $(TARGET)

.PHONY: all demos clean
