#
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC_ARGS = -Werror -fPIC -I../../src
LINK_ARGS = -Wl,--no-as-needed -lpcre2-8 -lutf8proc -L../../src -Wl,-rpath,../../src -lutf8lex

LC_CTYPE=en_US.UTF-8

.PHONY: all
all: build run

.PHONY: build
build: build-test-cat build-test-string

.PHONY: build-test-cat
build-test-cat:
	gcc $(CC_ARGS) -c test_utf8lex_cat.c -o test_utf8lex_cat.o
	gcc $(LINK_ARGS) \
	    test_utf8lex_cat.o \
	    -o test_utf8lex_cat

.PHONY: build-test-string
build-test-string:
	gcc $(CC_ARGS) -c test_utf8lex_string.c -o test_utf8lex_string.o
	gcc $(LINK_ARGS) \
	    test_utf8lex_string.o \
	    -o test_utf8lex_string

.PHONY: clean
clean:
	rm -f *.o
	rm -f test_utf8lex_cat test_utf8lex_string

.PHONY: run
run:
	./test_utf8lex_cat
	./test_utf8lex_string
