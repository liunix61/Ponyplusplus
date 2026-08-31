CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-function
LDFLAGS = -lm

SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
TESTS = $(wildcard $(TESTS)/*.c)

TARGET = $(BINDIR)/ponyppc

.PHONY: all clean test run help

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c -o $@ $<

test: all
	@echo "Running tests..."
	@for t in $(tests/*.c); do \
		base=$$(basename $$t .c); \
		echo "=== $$base ==="; \
		$(CC) $(CFLAGS) -I$(INCDIR) -o $(OBJDIR)/$$base $$t $(LDFLAGS) && \
		./$(OBJDIR)/$$base || exit 1; \
	done
	@echo "All tests passed."

clean:
	rm -rf $(OBJDIR) $(BINDIR)

help:
	@echo "Usage: make [target]"
	@echo "  all    - Build ponyppc compiler"
	@echo "  test   - Build and run tests"
	@echo "  clean  - Remove build artifacts"