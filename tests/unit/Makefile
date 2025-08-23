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

#
# By default, assume valgrind is supported and installed.
# To override and skip valgrind checks, set VALGRIND_PLATFORM="UNSUPPORTED".
#
VALGRIND_PLATFORM ?= "supported"
GENERATE_SUPPRESSIONS_OPTIONS = -s
RUNTIME_LEAK_OPTIONS = --leak-check=yes --error-exitcode=66 --suppressions=valgrind_suppressions.txt

TEST_BUILD_DIR ?= ../build

SOURCE_FILES ?= \
	test_utf8lex_cat.c \
	test_utf8lex_definition.c \
	test_utf8lex_definition_multi.c \
	test_utf8lex_printable_str.c \
	test_utf8lex_read.c \
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
	@echo "ulimit main stack size: hard=`ulimit -H -s` soft=`ulimit -S -s`"
	@echo "VALGRIND_PLATFORM=$(VALGRIND_PLATFORM)"
	for UNIT_TEST in $(PROGRAMS); \
	do \
	    echo "$$UNIT_TEST:"; \
	    if test "$(VALGRIND_PLATFORM)" = "UNSUPPORTED"; \
	    then \
	        "$$UNIT_TEST" || exit 1; \
	    else \
	        valgrind $(RUNTIME_LEAK_OPTIONS) "$$UNIT_TEST" || exit 1; \
	    fi; \
	done
