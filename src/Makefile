#
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC_ARGS = -Werror
LINK_ARGS =

LC_CTYPE=en_US.UTF-8

.PHONY: all
all: build

.PHONY: build
build:
	gcc $(CC_ARGS) -c utf8lex.c -o utf8lex.o

.PHONY: clean
clean:
	rm -f *.o
