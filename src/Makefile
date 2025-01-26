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

UTF8LEX_VERSION ?= 1.0.0
UTF8LEX_MAJOR ?= $(word 1, $(subst ., ,$(UTF8LEX_VERSION)))

#
# Make sure the path we use to load shared libraries is absolute, not relative:
#
UTF8LEX_LIBRARY_DIR = $(subst $(PWD)//,/,$(PWD)/$(BUILD_DIR))
UTF8LEX_LIBRARY_PATH = $(if $(LD_LIBRARY_PATH),$(UTF8LEX_LIBRARY_DIR):$(LD_LIBRARY_PATH),$(UTF8LEX_LIBRARY_DIR))

#
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC = gcc
CFLAGS = -Werror -fPIC -O2
# CFLAGS = -Werror -fPIC -O2 --debug

LD = gcc
LDFLAGS_LIBRARY = -shared
LDFLAGS_PROGRAM = -Wl,--no-as-needed -lpcre2-8 -lutf8proc -L$(UTF8LEX_LIBRARY_DIR) -Wl,-rpath,$(UTF8LEX_LIBRARY_DIR) -lutf8lex

BUILD_DIR ?= ../build

SOURCE_FILES ?= \
	utf8lex_buffer.c \
	utf8lex_cat.c \
	utf8lex_definition.c \
	utf8lex_definition_cat.c \
	utf8lex_definition_literal.c \
	utf8lex_definition_multi.c \
	utf8lex_definition_regex.c \
	utf8lex_error.c \
	utf8lex_file.c \
	utf8lex_generate.c \
	utf8lex_lex.c \
	utf8lex_read.c \
	utf8lex_rule.c \
	utf8lex_state.c \
	utf8lex_string.c \
	utf8lex_target_language_c.c \
	utf8lex_token.c

OBJECT_FILES = \
	$(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCE_FILES))

LIBRARY = \
	$(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_VERSION)

PROGRAM = \
	$(BUILD_DIR)/utf8lex.$(UTF8LEX_VERSION)

LC_CTYPE = en_US.UTF-8

.PHONY: all
all: build

# Rule for $(OBJECT_FILES):
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for .so library files:
$(LIBRARY): $(OBJECT_FILES)
	$(LD) $(LDFLAGS_LIBRARY)  \
	    -Wl,-soname,libutf8lex.so.$(UTF8LEX_MAJOR) \
	    $(OBJECT_FILES) \
	    -o $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_VERSION)
	chmod a+x $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_VERSION)
	cd $(BUILD_DIR) \
	    && if test "$(UTF8LEX_MAJOR)" != "$(UTF8LEX_VERSION)" ; \
                then \
	            ln -s -f libutf8lex.so.$(UTF8LEX_VERSION) \
	                libutf8lex.so.$(UTF8LEX_MAJOR) ; \
	        fi \
	    && ln -s -f libutf8lex.so.$(UTF8LEX_VERSION) \
	        libutf8lex.so

# Rules for $(PROGRAM):
$(BUILD_DIR)/utf8lex.o: utf8lex.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(PROGRAM): $(BUILD_DIR)/utf8lex.o
	$(LD) $(LDFLAGS_PROGRAM) $< -o $@
	cd $(BUILD_DIR) \
	    && if test "$(UTF8LEX_MAJOR)" != "$(UTF8LEX_VERSION)" ; \
                then \
	            ln -s -f utf8lex.$(UTF8LEX_VERSION) \
	                utf8lex.$(UTF8LEX_MAJOR) ; \
	        fi \
	    && ln -s -f utf8lex.$(UTF8LEX_VERSION) \
	        utf8lex

.PHONY: build
build: $(LIBRARY) $(PROGRAM)

.PHONY: clean
clean:
	rm -f core
	rm -f $(OBJECT_FILES)
	rm -f \
	    $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_VERSION) \
	    $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_MAJOR) \
	    $(BUILD_DIR)/libutf8lex.so \
	    $(BUILD_DIR)/utf8lex.$(UTF8LEX_VERSION) \
	    $(BUILD_DIR)/utf8lex.$(UTF8LEX_MAJOR) \
	    $(BUILD_DIR)/utf8lex
