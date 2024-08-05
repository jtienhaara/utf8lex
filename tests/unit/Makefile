SRC_DIR ?= ../../src
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
	test_utf8lex_cat.c \
	test_utf8lex_definition.c \
	test_utf8lex_definition_multi.c \
	test_utf8lex_printable_str.c \
	test_utf8lex_rule.c \
	test_utf8lex_string.c

OBJECT_FILES = \
	$(patsubst %.c,$(TEST_BUILD_DIR)/%.o,$(SOURCE_FILES))

PROGRAMS = \
	$(patsubst %.c,$(TEST_BUILD_DIR)/%,$(SOURCE_FILES))

LC_CTYPE = en_US.UTF-8

.PHONY: all
all: build run

# Rule for $(OBJECT_FILES):
$(TEST_BUILD_DIR)/%.o: %.c
	mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for $(PROGRAMS):
$(TEST_BUILD_DIR)/%: $(TEST_BUILD_DIR)/%.o
	$(LD) $(LDFLAGS) $< -o $@

.PHONY: build
build: $(PROGRAMS)

.PHONY: clean
clean:
	rm -f core
	rm -f $(OBJECT_FILES) $(PROGRAMS)

.PHONY: run
run:
	@for UNIT_TEST in $(PROGRAMS); \
	do \
	    echo "$$UNIT_TEST:"; \
	    "$$UNIT_TEST" || exit 1; \
	done
