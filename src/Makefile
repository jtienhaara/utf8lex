#
# utf8lex
# Copyright © 2023-2025 Johann Tienhaara
# All rights reserved
#
# SPDX-License-Identifier: Apache-2.0
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
LD_PROGRAM_FLAGS = -Wl,--no-as-needed -lpcre2-8 -lutf8proc

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
# (We decided not to statically link utf8lex executable after all,
# since the standard c library causes problems for valgrind.  Abort mission!
# Now we dynamically link all libraries for the ut8lex executable,
# but statically link the .o files for utf8lex itself, since
# no distribution in the universe today has libutf8lex.so.
# Happy compromise.)
#
LDSTATIC_EXTRACT = ar
LDSTATIC_EXTRACT_FLAGS = -x -o
LDSTATIC_LIBRARY = ar
LDSTATIC_LIBRARY_FLAGS = -c -r -s

#
# The template C files get processed into constants for use in runtime
# environments where the templates themselves do not exist.
# See ../templates/Makefile.
#
GENERATED_SOURCE_FILES ?= \
	$(BUILD_DIR)/templates/utf8lex_template_head.c \
	$(BUILD_DIR)/templates/utf8lex_template_tail.c

#
# Do NOT include utf8lex.c (with main()) in SOURCE_FILES:
#
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
	utf8lex_token.c

GENERATED_OBJECT_FILES = \
	$(patsubst $(BUILD_DIR)/templates/%.c,$(BUILD_DIR)/libutf8lex/%.o,$(GENERATED_SOURCE_FILES))

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

# Rule for $(GENERATED_OBJECT_FILES):
$(BUILD_DIR)/libutf8lex/%.o: $(BUILD_DIR)/templates/%.c
	mkdir -p $(BUILD_DIR)/libutf8lex
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for $(OBJECT_FILES):
$(BUILD_DIR)/libutf8lex/%.o: %.c
	mkdir -p $(BUILD_DIR)/libutf8lex
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: check_generated_source_files
check_generated_source_files:
	@for GENERATED_SOURCE_FILE in $(GENERATED_SOURCE_FILES); \
	do \
	    if test ! -f "$$GENERATED_SOURCE_FILE"; \
	    then \
	        echo "ERROR Missing generated source file: $$GENERATED_SOURCE_FILE" >&2; \
	        echo "ERROR You probably need to 'make' from top-level templates directory." >&2; \
	        exit 1; \
	    fi; \
	done

.PHONY: check_source_files_list
check_source_files_list:
	@for SOURCE_FILE in *.c; \
	do \
	    if test "$$SOURCE_FILE" = "utf8lex.c"; \
	    then \
	        continue; \
	    fi; \
	    OBJECT_FILE=`echo "$$SOURCE_FILE" | sed 's|\\.[a-zA-Z0-9]$$|\\.o|'`; \
	    if test ! -f "$(BUILD_DIR)/libutf8lex/$$OBJECT_FILE"; \
	    then \
	        echo "ERROR source file $$SOURCE_FILE did not generate an object file $(BUILD_DIR)/libutf8lex/$$OBJECT_FILE.  Is it included in the Makefile?" >&2; \
	        exit 1; \
	    fi; \
	done

# Rule for .so library files:
$(LIBRARY_DYNAMIC): $(GENERATED_OBJECT_FILES) $(OBJECT_FILES) check_source_files_list
	@echo "Building $(LIBRARY_DYNAMIC)..."
	$(LD_LIBRARY) $(LD_LIBRARY_FLAGS)  \
	    -Wl,-soname,libutf8lex.so.$(UTF8LEX_MAJOR) \
	    $(GENERATED_OBJECT_FILES) \
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
	@echo "SUCCESS Building $(LIBRARY_DYNAMIC)."

# Rule for .a library files:
$(LIBRARY_STATIC): $(GENERATED_OBJECT_FILES) $(OBJECT_FILES) check_source_files_list
	@echo "Building $(LIBRARY_STATIC)..."
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
	    $(GENERATED_OBJECT_FILES) \
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
	@echo "SUCCESS Building $(LIBRARY_STATIC)."

# Rules for $(PROGRAM):
$(BUILD_DIR)/libutf8lex/utf8lex.o: utf8lex.c
	mkdir -p $(BUILD_DIR)/libutf8lex
	$(CC) $(CFLAGS) -c $< -o $@

#
# We statically link the library object files, along with utf8lex.o,
# into the executable (but use dynamically linked libpcre2-8.so
# and libutf8proc.so), so that most operating system distributions
# will run utf8lex out of the box, no dynamic library install required.
#
$(PROGRAM): $(GENERATED_OBJECT_FILES) $(OBJECT_FILES) $(BUILD_DIR)/libutf8lex/utf8lex.o
	@echo "Building $(PROGRAM)..."
	$(LD_PROGRAM) $(LD_PROGRAM_FLAGS) $^ -o $@
	cd $(BUILD_DIR) \
	    && if test "$(UTF8LEX_MAJOR)" != "$(UTF8LEX_VERSION)" ; \
                then \
	            ln -s -f utf8lex.$(UTF8LEX_VERSION) \
	                utf8lex.$(UTF8LEX_MAJOR) ; \
	        fi \
	    && ln -s -f utf8lex.$(UTF8LEX_VERSION) \
	        utf8lex
	@echo "SUCCESS Building $(PROGRAM)."

.PHONY: build
build: check_generated_source_files \
	$(LIBRARY_DYNAMIC) $(LIBRARY_STATIC) $(PROGRAM)

#
# E.g.: make re-version UTF8LEX_VERSION=1.2.3
#
# To make the release process (Docker within GitHub Actions)
# less excruciatingly painful (but still excruciatingly painful),
# we re-use the .a file generated during the last commit to the main
# branch, but then explode it, and re-build the .o files into
# a newly versioned .so dynamic library.
#
# GitHub Actions has to copy the previously-built static library
# (build/libutf8lex.a.(githuba_hash).(debian_architecture)),
# and rename it to the new version (build/libutf8lex.a.1.2.3
# with no architecture in the name), to produce the .so being built
# (build/libutf8lex.so.1.2.3).
#
.PHONY: re-version-explode-archive
re-version-explode-archive:
	@echo "Exploding archive to re-version $(UTF8LEX_VERSION)..."
	@if test ! -f "$(LIBRARY_STATIC)"; \
	then \
	    echo "ERROR $(LIBRARY_STATIC) does not exist" >&2; \
	    echo "ERROR Copy previously built .a to $(LIBRARY_STATIC) and try again" >&2; \
	    exit 1; \
	fi
	rm -rf $(BUILD_DIR)/libutf8lex
	mkdir -p $(BUILD_DIR)/libutf8lex
	cd $(BUILD_DIR)/libutf8lex \
	    && $(LDSTATIC_EXTRACT) $(LDSTATIC_EXTRACT_FLAGS) \
	        ../libutf8lex.a.$(UTF8LEX_VERSION)
	cd "$(ORIGINAL_DIR)"
	touch $(BUILD_DIR)/libutf8lex/*.o
	@echo "SUCCESS Exploding archive to re-version $(UTF8LEX_VERSION)."

.PHONY: re-version
re-version: re-version-explode-archive $(LIBRARY_DYNAMIC) $(PROGRAM)
	@echo "Making exploded ar files writable by all"
	chmod -R a+w $(BUILD_DIR)

.PHONY: clean
clean:
	rm -f core
	rm -f $(GENERATED_OBJECT_FILES)
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
