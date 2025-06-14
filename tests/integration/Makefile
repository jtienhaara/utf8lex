#
# utf8lex
# Copyright © 2023-2025 Johann Tienhaara
# All rights reserved
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

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

RUNTIME_LEAK_OPTIONS = --leak-check=yes --error-exitcode=66 --suppressions=valgrind_suppressions.txt

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
	        valgrind $(RUNTIME_LEAK_OPTIONS) $(TEST_BUILD_DIR)/test_utf8lex "$$TEST_FILE" \
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
	echo "================================================================"
	echo "test_utf8lex_generate 001:"
	valgrind $(RUNTIME_LEAK_OPTIONS) $(TEST_BUILD_DIR)/test_utf8lex_generate \
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
	echo "----------------------------------------------------------------"
	echo "================================================================"
	echo "test_utf8lex_generate 002:"
	valgrind $(RUNTIME_LEAK_OPTIONS) $(TEST_BUILD_DIR)/test_utf8lex_generate \
	    . \
	    ../../templates/c/mmap \
	    $(TEST_BUILD_DIR) \
	    test_utf8lex_generate_002
	$(CC) $(CFLAGS) \
	    -c $(TEST_BUILD_DIR)/test_utf8lex_generate_002.c \
	    -o $(TEST_BUILD_DIR)/test_utf8lex_generate_002.o
	$(LD) $(LDFLAGS) \
	    $(TEST_BUILD_DIR)/test_utf8lex_generate_002.o \
	    $(TEST_BUILD_DIR)/test_l_file.o \
	    -o $(TEST_BUILD_DIR)/test_utf8lex_generate_002
	$(TEST_BUILD_DIR)/test_utf8lex_generate_002 \
	    test_l_file_002_input.txt \
	    test_l_file_002_expected_output.txt \
	    $(TEST_BUILD_DIR)/test_l_file_002_actual_output.txt
	echo "----------------------------------------------------------------"

.PHONY: debug
debug:
	ls ./core > /dev/null \
	    && readelf -a core \
	    && objdump --syms $(UTF8LEX_LIBRARY_DIR)/libutf8lex.so | grep -v '\*UND\*' \
	    && gdb $(TEST_BUILD_DIR)/test_utf8lex_generate ./core
