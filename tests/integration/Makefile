#
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC_ARGS = -Werror -I../../src
LINK_ARGS =

LC_CTYPE=en_US.UTF-8

.PHONY: all
all: build run

# !!! -L/usr/lib/x86_64-linux-gnu --> no good architecture-dependent
.PHONY: build
build:
	gcc $(CC_ARGS) -c test_utf8lex.c -o test_utf8lex.o
	gcc $(LINK_ARGS) \
	    ../../src/utf8lex.o \
	    test_utf8lex.o \
	    -L/usr/lib/x86_64-linux-gnu \
	    -lpcre2-8 \
	    -lutf8proc \
	    -o test_utf8lex

.PHONY: clean
clean:
	rm -f *.o
	rm -f test_utf8lex

.PHONY: run
run: build
	@for TEST_FILE in test_utf8lex_*.txt; \
	do \
	    echo ""; \
	    echo "$$TEST_FILE:"; \
	    cat "$$TEST_FILE" \
	        | ./test_utf8lex \
	        || exit 1; \
	done
