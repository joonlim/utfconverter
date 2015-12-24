CC = clang
CFLAGS = -Wall -Werror
DFLAGS = -g -DCSE320
BIN = utfconverter

SRC = $(wildcard *.c)

all: $(BIN)

debug: $(SRC)
	$(CC) $(CFLAGS) $(DFLAGS) $^ -o $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean run

clean:
	rm -f *.o *.out $(BIN)

run: $(BIN)
	./$(BIN)
