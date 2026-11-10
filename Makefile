CC      = cc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -O2 -Iinclude -MMD -MP
BUILD   = build
BIN     = bin

LIB_SRC = $(filter-out src/main.c,$(wildcard src/*.c))
LIB_OBJ = $(LIB_SRC:src/%.c=$(BUILD)/%.o)
TESTS   = $(patsubst tests/%.c,$(BIN)/%,$(wildcard tests/*.c))

all: $(BIN)/minidb

$(BIN)/minidb: $(BUILD)/main.o $(LIB_OBJ) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN)/test_%: tests/test_%.c $(LIB_OBJ) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TESTS)
	@for t in $(TESTS); do ./$$t || exit 1; done

run: $(BIN)/minidb
	./$(BIN)/minidb

$(BUILD) $(BIN):
	mkdir -p $@

clean:
	rm -rf $(BUILD) $(BIN)

-include $(wildcard $(BUILD)/*.d)

.PHONY: all test run clean
