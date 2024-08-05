#
# SRC_DIR is required for header files:
#
SRC_DIR ?= ../../src

#
# BUILD_DIR is required for libraries and executables:
#
BUILD_DIR ?= ../../build

#
# Make sure the path we use to load shared libraries is absolute, not relative:
#
UTF8LEX_LIBRARY_DIR = $(subst $(PWD)//,/,$(PWD)/$(BUILD_DIR))
UTF8LEX_LIBRARY_PATH = $(if $(LD_LIBRARY_PATH),$(UTF8LEX_LIBRARY_DIR):$(LD_LIBRARY_PATH),$(UTF8LEX_LIBRARY_DIR))

#
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC = gcc
CFLAGS = -Werror -fPIC -I$(SRC_DIR) --debug

LD = gcc
LDFLAGS = -Wl,--no-as-needed -lpcre2-8 -lutf8proc -L$(UTF8LEX_LIBRARY_DIR) -Wl,-rpath,$(UTF8LEX_LIBRARY_DIR) -lutf8lex

TEST_BUILD_DIR ?= ../build

SOURCE_FILES ?= \
	test_utf8lex.c \
	test_utf8lex_generate.c

OBJECT_FILES = \
	$(patsubst %.c,$(TEST_BUILD_DIR)/%.o,$(SOURCE_FILES))

PROGRAMS = \
	$(patsubst %.c,$(TEST_BUILD_DIR)/%,$(SOURCE_FILES))

LC_CTYPE = en_US.UTF-8

# Rule for $(OBJECT_FILES):
$(TEST_BUILD_DIR)/%.o: %.c
	mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for $(PROGRAMS):
$(TEST_BUILD_DIR)/%: $(TEST_BUILD_DIR)/%.o
	$(LD) $(LDFLAGS) $< -o $@

.PHONY: build
build: $(PROGRAMS)
	$(CC) $(CFLAGS) -c test_l_file.c -o $(TEST_BUILD_DIR)/test_l_file.o

.PHONY: clean
clean:
	rm -f core
	rm -f $(OBJECT_FILES) $(TEST_BUILD_DIR)/test_l_file.o $(PROGRAMS)

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

#
# Turn test_utf8lex_generate_001.l into test_utf8lex_generate_001.c
# Compile test_utf8lex_generate_001.c into an executable
# Use test_utf8lex_generate_001 to parse test_l_file_001_input.txt
# Make sure the output is the same as expected.
#
.PHONY: run-test_utf8lex_generate
run-test_utf8lex_generate:
	$(TEST_BUILD_DIR)/test_utf8lex_generate \
	    . \
	    ../../templates/c/mmap \
	    $(TEST_BUILD_DIR) \
	    test_utf8lex_generate_001
	$(CC) $(CFLAGS) \
	    -c $(TEST_BUILD_DIR)/test_utf8lex_generate_001.c \
	    -o $(TEST_BUILD_DIR)/test_utf8lex_generate_001.o
	$(LD) $(LDFLAGS) \
	    $(TEST_BUILD_DIR)/test_utf8lex_generate_001.o \
	    $(TEST_BUILD_DIR)/test_l_file.o \
	    -o $(TEST_BUILD_DIR)/test_utf8lex_generate_001
	$(TEST_BUILD_DIR)/test_utf8lex_generate_001 \
	    test_l_file_001_input.txt \
	    test_l_file_001_expected_output.txt \
	    $(TEST_BUILD_DIR)/test_l_file_001_actual_output.txt

.PHONY: debug
debug:
	ls ./core > /dev/null \
	    && gdb $(TEST_BUILD_DIR)/test_utf8lex_generate ./core
