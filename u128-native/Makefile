CC     = gcc
TARGET = dev256
SRC    = dev256.c
TEST   = test_dev256
TESTSRC= test_dev256.c

CFLAGS_DEBUG   = -Wall -gdwarf-5 -std=c99 -m64 -O0
CFLAGS_RELEASE = -Wall -std=c99 -m64 -O2

all: debug

.PHONY: all debug release test clean

debug: $(SRC)
	$(CC) $(CFLAGS_DEBUG) -o $(TARGET) $(SRC)

release: $(SRC)
	$(CC) $(CFLAGS_RELEASE) -o $(TARGET) $(SRC)

# -DTESTING compiles out main() in dev256.c so the test can link it.
test: $(TESTSRC) $(SRC)
	$(CC) $(CFLAGS_RELEASE) -Wextra -DTESTING -o $(TEST) $(TESTSRC) $(SRC)
	./$(TEST)

clean:
	rm -f $(TARGET) $(TEST)
