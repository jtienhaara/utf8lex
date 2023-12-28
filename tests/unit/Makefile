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
	gcc $(CC_ARGS) -c test_utf8lex_cat.c -o test_utf8lex_cat.o
	gcc $(LINK_ARGS) \
	    ../../src/utf8lex.o \
	    test_utf8lex_cat.o \
	    -L/usr/lib/x86_64-linux-gnu \
	    -lpcre2-8 \
	    -lutf8proc \
	    -o test_utf8lex_cat

.PHONY: clean
clean:
	rm -f *.o
	rm -f test_utf8lex_cat

.PHONY: run
run: build
	./test_utf8lex_cat
