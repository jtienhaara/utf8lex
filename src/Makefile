# !!! AR?=ar
# !!! CC?=gcc
# !!! INSTALL=install
# !!! 
# !!! CFLAGS ?= -O2
# !!! PICFLAG = -fPIC
# !!! C99FLAG = -std=c99
# !!! WCFLAGS = -Wsign-conversion -Wall -Wextra -pedantic
# !!! UCFLAGS = $(CPPFLAGS) $(CFLAGS) $(PICFLAG) $(C99FLAG) $(WCFLAGS) -DUTF8PROC_EXPORTS $(UTF8PROC_DEFINES)
# !!! LDFLAG_SHARED = -shared
# !!! SOFLAG = -Wl,-soname
# !!! $(CC) $(LDFLAGS) $(LDFLAG_SHARED) -o $@ $(SOFLAG) -Wl,libutf8proc.so.$(MAJOR) utf8proc.o
# !!! 	chmod a-x $@

#
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC_ARGS = -Werror -fPIC -O2
LINK_ARGS = -shared

LC_CTYPE=en_US.UTF-8

.PHONY: all
all: build

.PHONY: build
build:
	gcc $(CC_ARGS) -c utf8lex_cat.c -o utf8lex_cat.o
	gcc $(CC_ARGS) -c utf8lex_file.c -o utf8lex_file.o
	gcc $(CC_ARGS) -c utf8lex_lex.c -o utf8lex_lex.o
	gcc $(CC_ARGS) -c utf8lex_location.c -o utf8lex_location.o
	gcc $(CC_ARGS) -c utf8lex_pattern_cat.c -o utf8lex_pattern_cat.o
	gcc $(CC_ARGS) -c utf8lex_pattern_literal.c -o utf8lex_pattern_literal.o
	gcc $(CC_ARGS) -c utf8lex_pattern_regex.c -o utf8lex_pattern_regex.o
	gcc $(CC_ARGS) -c utf8lex_read.c -o utf8lex_read.o
	gcc $(CC_ARGS) -c utf8lex_string.c -o utf8lex_string.o
	gcc $(CC_ARGS) -c utf8lex_token.c -o utf8lex_token.o
	gcc $(LINK_ARGS)  -Wl,-soname,libutf8lex.so.1 \
	    utf8lex_cat.o \
	    utf8lex_file.o \
	    utf8lex_lex.o \
	    utf8lex_location.o \
	    utf8lex_pattern_cat.o \
	    utf8lex_pattern_literal.o \
	    utf8lex_pattern_regex.o \
	    utf8lex_read.o \
	    utf8lex_string.o \
	    utf8lex_token.o \
	    -o libutf8lex.so.1.0.0
	chmod a+x libutf8lex.so.1.0.0
	ln -s libutf8lex.so.1.0.0 libutf8lex.so.1
	ln -s libutf8lex.so.1.0.0 libutf8lex.so

.PHONY: clean
clean:
	rm -f *.o
	rm -f libutf8lex.so.1.0.0 libutf8lex.so.1 libutf8lex.so
