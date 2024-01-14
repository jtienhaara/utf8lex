UTF8LEX_LIBRARY_VERSION = 1.0.0
UTF8LEX_MAJOR = $(word 1, $(subst ., ,$(UTF8LEX_LIBRARY_VERSION)))

#
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC = gcc
CFLAGS = -Werror -fPIC -O2

LD = gcc
LDFLAGS = -shared

BUILD_DIR = ../build

SOURCE_FILES = \
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
	utf8lex_string.c \
	utf8lex_target_language_c.c \
	utf8lex_token.c

OBJECT_FILES = \
	$(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCE_FILES))

LC_CTYPE=en_US.UTF-8

.PHONY: all
all: build

# Rule for $(OBJECT_FILES):
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_LIBRARY_VERSION): $(OBJECT_FILES)
	$(LD) $(LDFLAGS)  \
	    -Wl,-soname,$(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_MAJOR) \
	    $(OBJECT_FILES) \
	    -o $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_LIBRARY_VERSION)
	chmod a+x $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_LIBRARY_VERSION)
	cd $(BUILD_DIR) \
	    && ln -s -f libutf8lex.so.$(UTF8LEX_LIBRARY_VERSION) \
	        libutf8lex.so.$(UTF8LEX_MAJOR) \
	    && ln -s -f libutf8lex.so.$(UTF8LEX_LIBRARY_VERSION) \
	        libutf8lex.so
	rm -f $(OBJECT_FILES)

.PHONY: build
build: $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_LIBRARY_VERSION)

.PHONY: clean
clean:
	rm -f $(OBJECT_FILES)
	rm -f \
	    $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_LIBRARY_VERSION) \
	    $(BUILD_DIR)/libutf8lex.so.$(UTF8LEX_MAJOR) \
	    $(BUILD_DIR)/libutf8lex.so
