#
# utf8lex
# Copyright 2023 Johann Tienhaara
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
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC_ARGS = -Werror
LINK_ARGS =

LC_CTYPE=en_US.UTF-8

.PHONY: all
all: build test

.PHONY: container
container:
	docker build . \
	    --file build.Dockerfile \
	    --tag utf8lex:latest
	docker run \
	    --rm \
	    --volume `pwd`:/utf8lex:rw \
	    utf8lex:latest \
	    make all

.PHONY: build
build:
	gcc $(CC_ARGS) -c utf8lex.c -o utf8lex.o

.PHONY: test
test: build unit_tests integration_tests

# !!! -L/usr/lib/x86_64-linux-gnu --> no good architecture-dependent
.PHONY: unit_tests
unit_tests: build
	gcc $(CC_ARGS) -c test_utf8lex_cat.c -o test_utf8lex_cat.o
	gcc $(LINK_ARGS) \
	    utf8lex.o \
	    test_utf8lex_cat.o \
	    -L/usr/lib/x86_64-linux-gnu \
	    -lpcre2-8 \
	    -lutf8proc \
	    -o test_utf8lex_cat
	./test_utf8lex_cat

# !!! -L/usr/lib/x86_64-linux-gnu --> no good architecture-dependent
.PHONY: integration_tests
integration_tests: build
	gcc $(CC_ARGS) -c test_utf8lex.c -o test_utf8lex.o
	gcc $(LINK_ARGS) \
	    utf8lex.o \
	    test_utf8lex.o \
	    -L/usr/lib/x86_64-linux-gnu \
	    -lpcre2-8 \
	    -lutf8proc \
	    -o test_utf8lex
	@for TEST_FILE in test_utf8lex_*.txt; \
	do \
	    echo ""; \
	    echo "$$TEST_FILE:"; \
	    cat "$$TEST_FILE" \
	        | ./test_utf8lex \
	        || exit 1; \
	done
