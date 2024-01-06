SRC_DIR = ../../src
BUILD_DIR = ../../build

#
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC = gcc
CFLAGS = -Werror -fPIC -I$(SRC_DIR)

LD = gcc
LDFLAGS = -Wl,--no-as-needed -lpcre2-8 -lutf8proc -L$(BUILD_DIR) -Wl,-rpath,$(BUILD_DIR) -lutf8lex

TEST_BUILD_DIR = ../build

SOURCE_FILES = \
	test_utf8lex.c \
	test_utf8lex_generate.c

OBJECT_FILES = \
	$(patsubst %.c,$(TEST_BUILD_DIR)/%.o,$(SOURCE_FILES))

PROGRAMS = \
	$(patsubst %.c,$(TEST_BUILD_DIR)/%,$(SOURCE_FILES))

LC_CTYPE=en_US.UTF-8

# Rule for $(OBJECT_FILES):
$(TEST_BUILD_DIR)/%.o: %.c
	mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for $(PROGRAMS):
$(TEST_BUILD_DIR)/%: $(TEST_BUILD_DIR)/%.o
	$(LD) $(LDFLAGS) $< -o $@
	rm -f $<

#
# Kludgy McKludgerson for running the unit tests with the builtin
# "../build" library path for libutf8lex.so.$(MAJOR_VERSION).
# (-Wl,-soname,../build/libutf8.so.1 leads to "ld test_utf8lex_blabla"
# returning "../build/libutf8lex.so.1 => not found".)
# We should properly install the library in an absolute path.
# Meanwhile, this is a temporary kludge.
#
.PHONY: libraries-for-runtime
libraries-for-runtime:
	for LIBRARY in $(BUILD_DIR)/libutf8lex.so*; \
	do \
	    ln -s -f "$$LIBRARY" $(TEST_BUILD_DIR) || exit 1; \
	done

.PHONY: build
build: $(PROGRAMS) libraries-for-runtime

.PHONY: clean
clean:
	rm -f $(OBJECT_FILES) $(PROGRAMS)

.PHONY: run
run: run-test_utf8lex run-test_utf8lex_generate

.PHONY: run-test_utf8lex
run-test_utf8lex:
	@for TEST_FILE in test_utf8lex_*.txt; \
	do \
	    echo ""; \
	    echo "$$TEST_FILE:"; \
	        $(TEST_BUILD_DIR)/test_utf8lex "$$TEST_FILE" \
	        || exit 1; \
	done

.PHONY: run-test_utf8lex_generate
run-test_utf8lex_generate:
	$(TEST_BUILD_DIR)/test_utf8lex_generate \
	    . \
	    ../../templates/c/mmap \
	    $(TEST_BUILD_DIR) \
	    test_utf8lex_generate_001
