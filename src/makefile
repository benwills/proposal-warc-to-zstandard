CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=-L/usr/local/lib -lzstd
SOURCES=main.c mmap.c opts.c timer.c warc.c zstd.c
OBJECTS=main.o
BIN=cc_warc
BIN_DIR=../bin

all: $(SOURCES) $(BIN) clean_obj

$(BIN): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	mv $(BIN) $(BIN_DIR)

.c.o:
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean_obj:
	rm -f *.o

clean:
	rm -f *.o
	rm -f $(BIN_DIR)/$(BIN)
