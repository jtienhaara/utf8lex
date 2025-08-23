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

ORIGINAL_DIR = $(PWD)

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
CFLAGS = -Werror -fPIC -O2
# CFLAGS = -Werror -fPIC -O2 --debug

#
# Dynamically linked library:
#
LD_LIBRARY = gcc
LD_LIBRARY_FLAGS = -shared
LD_PROGRAM = gcc
LD_PROGRAM_FLAGS = -Wl,--no-as-needed -lpcre2-8 -lutf8proc -L$(UTF8LEX_LIBRARY_DIR) -Wl,-rpath,$(UTF8LEX_LIBRARY_DIR) -lutf8lex

#
# Long but very informative discussion about static linking pitfalls:
#
#     https://stackoverflow.com/questions/6578484/telling-gcc-directly-to-link-a-library-statically/20728782#20728782
#
# TL;DR: "-Wl,-Bstatic -lxyz" works as long as we're using ld.
# We've tied ourselves to ld for the dynamic version, but maybe in future
# we can fall back on linker-independent (gcc-specific) arguments instead.
# Of course, the pitfall with that is that most people probably understand
# ld options better than gcc-specific linking options...
#
# Anyhoo we'll use the "-l:libxyz.a" syntax for now.
#
LDSTATIC_EXTRACT = ar
LDSTATIC_EXTRACT_FLAGS = -x -o
LDSTATIC_LIBRARY = ar
LDSTATIC_LIBRARY_FLAGS = -c -r -s
# !!! LDSTATIC_PROGRAM = gcc
# !!! LDSTATIC_PROGRAM_FLAGS = -static -l:libc.a -l:libutf8proc.a -l:libpcre2-8.a \
# !!! 	-L$(UTF8LEX_LIBRARY_DIR) -l:libutf8lex.a
# !!! LDSTATIC_PROGRAM_FLAGS = -Wl,-Bstatic -lutf8proc -lpcre2-8
# !!! LDSTATIC_PROGRAM_FLAGS = -Wl,-Bstatic -L$(UTF8LEX_LIBRARY_DIR) -lutf8lex
# !!! LDSTATIC_PROGRAM_FLAGS = -lgcc_s -Wl,-Bstatic -L$(UTF8LEX_LIBRARY_DIR) -lutf8lex

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
	utf8lex_generate_1.c \
	utf8lex_generate_2.c \
	utf8lex_generate_3.c \
	utf8lex_lex.c \
	utf8lex_read.c \
	utf8lex_rule.c \
	utf8lex_state.c \
	utf8lex_string.c \
	utf8lex_sub_token.c \
	utf8lex_target_language_c.c \
	utf8lex_token.c \
	utf8lex.c

OBJECT_FILES = \
	$(patsubst %.c,$(BUILD_DIR)/libutf8lex/%.o,$(SOURCE_FILES))

LIBRARY_DYNAMIC = \
	$(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_VERSION)

LIBRARY_STATIC = \
	$(BUILD_DIR)/libutf8lex.a.$(UTF8LEX_VERSION)

PROGRAM = \
	$(BUILD_DIR)/utf8lex.$(UTF8LEX_VERSION)

LC_CTYPE = en_US.UTF-8

.PHONY: all
all: build

# Rule for $(OBJECT_FILES):
$(BUILD_DIR)/libutf8lex/%.o: %.c
	mkdir -p $(BUILD_DIR)/libutf8lex
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: check_source_files_list
check_source_files_list:
	@for SOURCE_FILE in *.c; \
	do \
	    OBJECT_FILE=`echo "$$SOURCE_FILE" | sed 's|\\.[a-zA-Z0-9]$$|\\.o|'`; \
	    if test ! -f "$(BUILD_DIR)/libutf8lex/$$OBJECT_FILE"; \
	    then \
	        echo "ERROR source file $$SOURCE_FILE did not generate an object file $(BUILD_DIR)/libutf8lex/$$OBJECT_FILE.  Is it included in the Makefile?" >&2; \
	        exit 1; \
	    fi; \
	done

# Rule for .so library files:
$(LIBRARY_DYNAMIC): $(OBJECT_FILES) check_source_files_list
	$(LD_LIBRARY) $(LD_LIBRARY_FLAGS)  \
	    -Wl,-soname,libutf8lex.so.$(UTF8LEX_MAJOR) \
	    $(OBJECT_FILES) \
	    -o $(LIBRARY_DYNAMIC)
	chmod a+x $(LIBRARY_DYNAMIC)
	cd $(BUILD_DIR) \
	    && if test "$(UTF8LEX_MAJOR)" != "$(UTF8LEX_VERSION)" ; \
                then \
	            ln -s -f libutf8lex.so.$(UTF8LEX_VERSION) \
	                libutf8lex.so.$(UTF8LEX_MAJOR) ; \
	        fi \
	    && ln -s -f libutf8lex.so.$(UTF8LEX_VERSION) \
	        libutf8lex.so

# Rule for .a library files:
$(LIBRARY_STATIC): $(OBJECT_FILES) check_source_files_list
	for LIBUTF8PROC in `find /usr/lib -type f -name 'libutf8proc.a'` ; \
	    do \
	        echo "Copying libutf8proc to extract from: $$LIBUTF8PROC" \
	        && rm -rf $(BUILD_DIR)/libutf8proc \
	        && mkdir -p $(BUILD_DIR)/libutf8proc \
	        && cp "$$LIBUTF8PROC" $(BUILD_DIR)/libutf8proc/ ; \
	    done
	cd $(BUILD_DIR)/libutf8proc \
	    && $(LDSTATIC_EXTRACT) $(LDSTATIC_EXTRACT_FLAGS) libutf8proc.a \
	    && rm libutf8proc.a
	cd "$(ORIGINAL_DIR)"
	for LIBPCRE2 in `find /usr/lib -type f -name 'libpcre2-8.a'` ; \
	    do \
	        echo "Copying libpcre2-8 to extract from: $$LIBPCRE2" \
	        && rm -rf $(BUILD_DIR)/libpcre2-8 \
	        && mkdir -p $(BUILD_DIR)/libpcre2-8 \
	        && cp "$$LIBPCRE2" $(BUILD_DIR)/libpcre2-8/ ; \
	    done
	cd $(BUILD_DIR)/libpcre2-8 \
	    && $(LDSTATIC_EXTRACT) $(LDSTATIC_EXTRACT_FLAGS) libpcre2-8.a \
	    && rm libpcre2-8.a
	cd "$(ORIGINAL_DIR)"
	$(LDSTATIC_LIBRARY) $(LDSTATIC_LIBRARY_FLAGS)  \
	    $(LIBRARY_STATIC) \
	    $(OBJECT_FILES) \
	    $(BUILD_DIR)/libutf8proc/* \
	    $(BUILD_DIR)/libpcre2-8/*
	chmod a+x $(LIBRARY_STATIC)
	cd $(BUILD_DIR) \
	    && if test "$(UTF8LEX_MAJOR)" != "$(UTF8LEX_VERSION)" ; \
                then \
	            ln -s -f libutf8lex.a.$(UTF8LEX_VERSION) \
	                libutf8lex.a.$(UTF8LEX_MAJOR) ; \
	        fi \
	    && ln -s -f libutf8lex.a.$(UTF8LEX_VERSION) \
	        libutf8lex.a

# Rules for $(PROGRAM):
$(BUILD_DIR)/libutf8lex/utf8lex.o: utf8lex.c
	mkdir -p $(BUILD_DIR)/libutf8lex
	$(CC) $(CFLAGS) -c $< -o $@

$(PROGRAM): $(LIBRARY_DYNAMIC) $(BUILD_DIR)/libutf8lex/utf8lex.o
	$(LD_PROGRAM) $(LD_PROGRAM_FLAGS) $< -o $@
	cd $(BUILD_DIR) \
	    && if test "$(UTF8LEX_MAJOR)" != "$(UTF8LEX_VERSION)" ; \
                then \
	            ln -s -f utf8lex.$(UTF8LEX_VERSION) \
	                utf8lex.$(UTF8LEX_MAJOR) ; \
	        fi \
	    && ln -s -f utf8lex.$(UTF8LEX_VERSION) \
	        utf8lex

# !!! $(PROGRAM): $(LIBRARY_STATIC) $(BUILD_DIR)/libutf8lex/utf8lex.o
# !!! 	$(LDSTATIC_PROGRAM) $(LDSTATIC_PROGRAM_FLAGS) $< -o $@
# !!! 	cd $(BUILD_DIR) \
# !!! 	    && if test "$(UTF8LEX_MAJOR)" != "$(UTF8LEX_VERSION)" ; \
# !!!                 then \
# !!! 	            ln -s -f utf8lex.$(UTF8LEX_VERSION) \
# !!! 	                utf8lex.$(UTF8LEX_MAJOR) ; \
# !!! 	        fi \
# !!! 	    && ln -s -f utf8lex.$(UTF8LEX_VERSION) \
# !!! 	        utf8lex

.PHONY: build
build: $(LIBRARY_DYNAMIC) $(LIBRARY_STATIC) $(PROGRAM)

.PHONY: clean
clean:
	rm -f core
	rm -f $(OBJECT_FILES)
	rm -rf \
	    $(BUILD_DIR)/libpcre2-8/ \
	    $(BUILD_DIR)/libutf8lex/ \
	    $(BUILD_DIR)/libutf8proc/ \

	rm -f \
	    $(LIBRARY_DYNAMIC) \
	    $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_MAJOR) \
	    $(BUILD_DIR)/libutf8lex.so \
	    $(LIBRARY_STATIC) \
	    $(BUILD_DIR)/libutf8lex.a.$(UTF8LEX_MAJOR) \
	    $(BUILD_DIR)/libutf8lex.a \
	    $(BUILD_DIR)/utf8lex.$(UTF8LEX_VERSION) \
	    $(BUILD_DIR)/utf8lex.$(UTF8LEX_MAJOR) \
	    $(BUILD_DIR)/utf8lex
