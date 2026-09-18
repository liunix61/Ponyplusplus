CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-function
LDFLAGS = -lm

SRCDIR = src
INCDIR = include
TESTDIR = tests
OBJDIR = build
BINDIR = bin

# 顶层 src/*.c -> build/*.o
TOP_SRCS = $(wildcard $(SRCDIR)/*.c)
TOP_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(TOP_SRCS))

# src/ponypp/*.c -> build/*.ponypp.o (展平目录)
SUB_SRCS = $(wildcard $(SRCDIR)/ponypp/*.c)
SUB_OBJS = $(patsubst $(SRCDIR)/ponypp/%.c, $(OBJDIR)/%.ponypp.o, $(SUB_SRCS))

ALL_OBJS = $(TOP_OBJS) $(SUB_OBJS)

# 编译器特有（含 main），不用于测试链接
DRIVER_OBJS = $(OBJDIR)/ponyppc.o

# 库 object 文件（不含 driver），供测试链接
LIB_OBJS = $(filter-out $(DRIVER_OBJS), $(ALL_OBJS))

TARGET = $(BINDIR)/ponyppc

.PHONY: all clean test run help lint regression gtest matrix coverage

all: $(TARGET)

$(TARGET): $(ALL_OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c -o $@ $<

$(OBJDIR)/%.ponypp.o: $(SRCDIR)/ponypp/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c -o $@ $<

lint: all
	@echo "Running linter..."
	@for c in $(TOP_SRCS) $(SUB_SRCS); do \
		$(CC) $(CFLAGS) -I$(INCDIR) -c $$c -o /dev/null 2>&1 || exit 1; \
	done
	@echo "Lint passed. No warnings."

test: all
	@echo "Running tests..."
	@for t in $(wildcard $(TESTDIR)/*.c); do \
		base=$$(basename $$t .c); \
		echo "=== $$base ==="; \
		$(CC) $(CFLAGS) -I$(INCDIR) -o $(OBJDIR)/$$base $$t $(LIB_OBJS) $(LDFLAGS) && \
		./$(OBJDIR)/$$base || exit 1; \
	done
	@echo "All tests passed."

run: all
	@./$(TARGET) --version

# ---- 工程化自动化（nanonode 式）----
regression: all
	@bash scripts/regression.sh

gtest:
	@cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Debug > /dev/null
	@cmake --build build-cmake -j4
	@cd build-cmake && ctest -j4 --timeout 180 --output-on-failure | tail -5

matrix: all
	@bash scripts/regression.sh --san

coverage:
	@cmake -S . -B build-cov -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="--coverage" > /dev/null
	@cmake --build build-cov -j4 > /dev/null
	@cd build-cov && ctest -j4 --timeout 180 > /dev/null 2>&1 || true
	@cd build-cov && gcovr -r .. --html --html-details -o coverage.html ../src 2>/dev/null | tail -3
	@echo "report: build-cov/coverage.html"

clean:
	rm -rf $(OBJDIR) $(BINDIR)

help:
	@echo "Usage: make [target]"
	@echo "  all    - Build ponyppc compiler"
	@echo "  test   - Build and run tests"
	@echo "  clean  - Remove build artifacts"
