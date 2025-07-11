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
SRC_DIR ?= ../src

#
# BUILD_DIR is required for libraries and executables:
#
BUILD_DIR ?= ../build

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

EXAMPLES_BUILD_DIR ?= ./build

LEX_FILES = \
	programming_tokens.l \
	programming_kludgy_grammar.l

GENERATED_SOURCE_FILES = \
	$(patsubst %.l,%.c,$(LEX_FILES))

NON_GENERATED_SOURCE_FILES = \
	example_lexer.c

SOURCE_FILES = \
	$(GENERATED_SOURCE_FILES) \
	$(NON_GENERATED_SOURCE_FILES)

OBJECT_FILES = \
	$(patsubst %.c,$(EXAMPLES_BUILD_DIR)/%.o,$(SOURCE_FILES))

PROGRAM = \
	$(EXAMPLES_BUILD_DIR)/example_lexer

LC_CTYPE = en_US.UTF-8

# Rule for $(GENERATED_SOURCE_FILES):
%.c: %.l
	valgrind $(RUNTIME_LEAK_OPTIONS) $(BUILD_DIR)/utf8lex $<

# Rule for $(OBJECT_FILES):
$(EXAMPLES_BUILD_DIR)/%.o: %.c
	mkdir -p $(EXAMPLES_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rules for $(PROGRAM)_*:
$(PROGRAM)_tokens: $(patsubst %.c,$(EXAMPLES_BUILD_DIR)/%.o,$(NON_GENERATED_SOURCE_FILES)) $(EXAMPLES_BUILD_DIR)/programming_tokens.o
	$(LD) $(LDFLAGS) $^ -o $@
	mv programming_tokens.c $(EXAMPLES_BUILD_DIR)/

$(PROGRAM)_kludgy_grammar: $(patsubst %.c,$(EXAMPLES_BUILD_DIR)/%.o,$(NON_GENERATED_SOURCE_FILES)) $(EXAMPLES_BUILD_DIR)/programming_kludgy_grammar.o
	$(LD) $(LDFLAGS) $^ -o $@
	mv programming_kludgy_grammar.c  $(EXAMPLES_BUILD_DIR)/

.PHONY: build
build: $(GENERATED_SOURCE_FILES) $(OBJECT_FILES) $(PROGRAM)_tokens $(PROGRAM)_kludgy_grammar

.PHONY: clean
clean:
	rm -f core
	rm -f $(GENERATED_SOURCE_FILES) $(OBJECT_FILES) $(PROGRAM)_tokens $(PROGRAM)_kludgy_grammar

.PHONY: run
run: run-example_lexer_tokens run-example_lexer_kludgy_grammar

#
# Use the various example_lexer_*s to parse program_001.language.
# Depending on which programming_X.l file we build example_lexer_X with,
# we'll get different outputs.
#
.PHONY: run-example_lexer_tokens
run-example_lexer_tokens:
	@echo "================================================================"
	@echo "example_lexer_tokens:"
	$(PROGRAM)_tokens \
	    program_001.language
	@echo "----------------------------------------------------------------"

.PHONY: run-example_lexer_kludgy_grammar
run-example_lexer_kludgy_grammar:
	@echo "================================================================"
	@echo "example_lexer_kludgy_grammar:"
	$(PROGRAM)_kludgy_grammar \
	    program_001.language
	@echo "----------------------------------------------------------------"
