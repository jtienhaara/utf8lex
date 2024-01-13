/*
 * utf8lex
 * Copyright 2023 Johann Tienhaara
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UTF8LEX_H_INCLUDED
#define UTF8LEX_H_INCLUDED

#include <inttypes.h>  // For uint32_t.
#include <stdbool.h>  // For bool, true, false.

// 8-bit character units for pcre2:
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

// Maximum number of bytes in one UTF-8 character:
#define UTF8LEX_MAX_BYTES_PER_CHAR 6

typedef struct _STRUCT_utf8lex_buffer           utf8lex_buffer_t;
typedef uint32_t                                utf8lex_cat_t;
typedef struct _STRUCT_utf8lex_cat_definition   utf8lex_cat_definition_t;
typedef struct _STRUCT_utf8lex_definition       utf8lex_definition_t;
typedef enum _ENUM_utf8lex_error                utf8lex_error_t;
typedef struct _STRUCT_utf8lex_literal_definition utf8lex_literal_definition_t;
typedef struct _STRUCT_utf8lex_location         utf8lex_location_t;
typedef struct _STRUCT_utf8lex_definition_type  utf8lex_definition_type_t;
typedef struct _STRUCT_utf8lex_regex_definition utf8lex_regex_definition_t;
typedef struct _STRUCT_utf8lex_rule             utf8lex_rule_t;
typedef struct _STRUCT_ut8lex_state             utf8lex_state_t;
typedef struct _STRUCT_utf8lex_string           utf8lex_string_t;
typedef struct _STRUCT_utf8lex_target_language  utf8lex_target_language_t;
typedef struct _STRUCT_utf8lex_token            utf8lex_token_t;
typedef enum _ENUM_utf8lex_unit                 utf8lex_unit_t;


//
// More about the regular expression patterns (pcre2) used by utx8lex:
//
//     https://pcre2project.github.io/pcre2/doc/html/pcre2pattern.html
//

//
// utf8lex_lex():
//
// Start / continue lexing from the specified table and state.
//
extern utf8lex_error_t utf8lex_lex(
        utf8lex_rule_t * first_rule,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        );


enum _ENUM_utf8lex_error
{
  UTF8LEX_OK = 0,
  UTF8LEX_EOF,  // Lexing completed successfully.

  UTF8LEX_MORE,  // Need to read in more bytes from the source.
  UTF8LEX_NO_MATCH,  // Could not match bytes against any definition(s).

  UTF8LEX_ERROR_NULL_POINTER,

  UTF8LEX_ERROR_FILE_OPEN,  // File doesn't exist, or permissions, etc.
  UTF8LEX_ERROR_FILE_DESCRIPTOR,  // Invalid file descriptor (int fd) or FILE *.
  UTF8LEX_ERROR_FILE_EMPTY,  // 0-byte file.
  UTF8LEX_ERROR_FILE_MMAP,  // Could not mmap() a file.
  UTF8LEX_ERROR_FILE_READ,  // Could not read() from an open file descriptor.
  UTF8LEX_ERROR_FILE_SIZE,  // Could not use fstat for file size.
  UTF8LEX_ERROR_FILE_WRITE,  // Could not write() to an open file descriptor.

  UTF8LEX_ERROR_BUFFER_INITIALIZED,  // Buffer has non-NULL str->bytes.
  UTF8LEX_ERROR_CHAIN_INSERT,  // Can't insert links into the chain, only append
  UTF8LEX_ERROR_CAT,  // Invalid cat (category id) NONE < cat < MAX / not found.
  UTF8LEX_ERROR_DEFINITION_TYPE,  // definition_type mismatch (eg cat / regex).
  UTF8LEX_ERROR_EMPTY_LITERAL,  // Literals cannot be "".
  UTF8LEX_ERROR_MAX_LENGTH,  // Too many (rules, definitions, ...) in database.
  UTF8LEX_ERROR_NOT_FOUND,  // ..._find() did not match any objects.
  UTF8LEX_ERROR_REGEX,  // Matching against a regular expression failed.
  UTF8LEX_ERROR_UNIT,  // Invalid unit must be NONE < unit < MAX.
  UTF8LEX_ERROR_INFINITE_LOOP,  // Aborted, possible infinite loop detected.

  UTF8LEX_ERROR_BAD_LENGTH,  // Negative length, < start, too close to end.
  UTF8LEX_ERROR_BAD_OFFSET,  // Negative offset, or too close to end of string.
  UTF8LEX_ERROR_BAD_START,  // Negative start, or too close to end of string.
  UTF8LEX_ERROR_BAD_AFTER,  // After is neither reset (-1) nor valid new start.
  UTF8LEX_ERROR_BAD_MIN,  // Min must be 1 or greater.
  UTF8LEX_ERROR_BAD_MAX,  // Max must be >= min, or -1 for no limit.
  UTF8LEX_ERROR_BAD_REGEX,  // Could not compile regex definition.
  UTF8LEX_ERROR_BAD_UTF8,  // Could not process the UTF-8 text.
  UTF8LEX_ERROR_BAD_ERROR,  // Invalid error NONE <= e <= MAX.

  UTF8LEX_ERROR_TOKEN,  // Unexpected token, while in some lexer state or other.
  UTF8LEX_ERROR_STATE,  // Some other bad state that is not captured above.

  UTF8LEX_ERROR_MAX
};


enum _ENUM_utf8lex_unit
{
  UTF8LEX_UNIT_NONE = -1,

  UTF8LEX_UNIT_BYTE = 0,
  UTF8LEX_UNIT_CHAR,
  UTF8LEX_UNIT_GRAPHEME,
  UTF8LEX_UNIT_LINE,

  UTF8LEX_UNIT_MAX
};

struct _STRUCT_utf8lex_location
{
  int start;  // First byte / char / grapheme / and so on of a token.
  int length;  // # of bytes / chars / graphemes / and so on of a token.
  int after;  // Either -1, or reset the start location to this, if >= 0.
};


struct _STRUCT_utf8lex_string
{
  size_t max_length_bytes;  // How many bytes have been allocated.
  size_t length_bytes;  // How many bytes have been written.
  unsigned char *bytes;
};

extern utf8lex_error_t utf8lex_string_init(
        utf8lex_string_t *self,
        size_t max_length_bytes,  // How many bytes have been allocated.
        size_t length_bytes,  // How many bytes have been written.
        unsigned char *bytes
        );
extern utf8lex_error_t utf8lex_string_clear(
        utf8lex_string_t *self
        );

// Print state (location) to the specified string:
utf8lex_error_t utf8lex_state_string(
        utf8lex_string_t *str,
        utf8lex_state_t *state
        );

// Print error message to the specified string:
utf8lex_error_t utf8lex_error_string(
        utf8lex_string_t *str,
        utf8lex_error_t error
        );

// No more than (this many) buffers can be chained together:
extern const uint32_t UTF8LEX_BUFFER_STRINGS_MAX;

struct _STRUCT_utf8lex_buffer
{
  utf8lex_buffer_t *next;
  utf8lex_buffer_t *prev;

  int fd;  // File descriptor, or -1 if no open file backs this buffer.
  FILE *fp;  // File descriptor for fopen(), fread(), or NULL.

  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];
  utf8lex_string_t *str;
  bool is_eof;  // No more bytes to read?  (If so, do not return UTF8LEX_MORE).
};

extern utf8lex_error_t utf8lex_buffer_init(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *prev,
        utf8lex_string_t *str,
        bool is_eof
        );
extern utf8lex_error_t utf8lex_buffer_clear(
        utf8lex_buffer_t *self
        );

extern utf8lex_error_t utf8lex_buffer_add(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *tail
        );

// buffer functions in utf8lex_file.c:

// Do NOT call utf8lex_buffer_init() before calling utf8lex_buffer_mmap().
extern utf8lex_error_t utf8lex_buffer_mmap(
        utf8lex_buffer_t *self,
        unsigned char *path
        );
extern utf8lex_error_t utf8lex_buffer_munmap(
        utf8lex_buffer_t *self
        );

// Call utf8lex_buffer_init() first, then utf8lex_buffer_read().
extern utf8lex_error_t utf8lex_buffer_read(
        utf8lex_buffer_t *self,
        int fd
        );

// Call utf8lex_buffer_init() first, then utf8lex_buffer_readf().
extern utf8lex_error_t utf8lex_buffer_readf(
        utf8lex_buffer_t *self,
        FILE *fp
        );


//
// Base categories are equivalent to (but not equal to) those
// in the utf8proc library (UTF8PROC_CATEGORY_LU, etc):
//
extern const utf8lex_cat_t UTF8LEX_CAT_NONE;
extern const utf8lex_cat_t UTF8LEX_CAT_OTHER_NA;  // UTF8PROC_CATEGORY_CN
extern const utf8lex_cat_t UTF8LEX_CAT_LETTER_UPPER;  // UTF8PROC_CATEGORY_LU
extern const utf8lex_cat_t UTF8LEX_CAT_LETTER_LOWER;  // UTF8PROC_CATEGORY_LL
extern const utf8lex_cat_t UTF8LEX_CAT_LETTER_TITLE;  // UTF8PROC_CATEGORY_LT
extern const utf8lex_cat_t UTF8LEX_CAT_LETTER_MODIFIER;  // UTF8PROC_CATEGORY_LM
extern const utf8lex_cat_t UTF8LEX_CAT_LETTER_OTHER;  // UTF8PROC_CATEGORY_LO
extern const utf8lex_cat_t UTF8LEX_CAT_MARK_NON_SPACING;  // UTF8PROC_CATEGORY_MN
extern const utf8lex_cat_t UTF8LEX_CAT_MARK_SPACING_COMBINING;  // UTF8PROC_CATEGORY_MC
extern const utf8lex_cat_t UTF8LEX_CAT_MARK_ENCLOSING;  // UTF8PROC_CATEGORY_ME
extern const utf8lex_cat_t UTF8LEX_CAT_NUM_DECIMAL;  // UTF8PROC_CATEGORY_ND
extern const utf8lex_cat_t UTF8LEX_CAT_NUM_LETTER;  // UTF8PROC_CATEGORY_NL
extern const utf8lex_cat_t UTF8LEX_CAT_NUM_OTHER;  // UTF8PROC_CATEGORY_NO
extern const utf8lex_cat_t UTF8LEX_CAT_PUNCT_CONNECTOR;  // UTF8PROC_CATEGORY_PC
extern const utf8lex_cat_t UTF8LEX_CAT_PUNCT_DASH;  // UTF8PROC_CATEGORY_PD
extern const utf8lex_cat_t UTF8LEX_CAT_PUNCT_OPEN;  // UTF8PROC_CATEGORY_PS
extern const utf8lex_cat_t UTF8LEX_CAT_PUNCT_CLOSE;  // UTF8PROC_CATEGORY_PE
extern const utf8lex_cat_t UTF8LEX_CAT_PUNCT_QUOTE_OPEN;  // UTF8PROC_CATEGORY_PI
extern const utf8lex_cat_t UTF8LEX_CAT_PUNCT_QUOTE_CLOSE;  // UTF8PROC_CATEGORY_PF
extern const utf8lex_cat_t UTF8LEX_CAT_PUNCT_OTHER;  // UTF8PROC_CATEGORY_PO
extern const utf8lex_cat_t UTF8LEX_CAT_SYM_MATH;  // UTF8PROC_CATEGORY_SM
extern const utf8lex_cat_t UTF8LEX_CAT_SYM_CURRENCY;  // UTF8PROC_CATEGORY_SC
extern const utf8lex_cat_t UTF8LEX_CAT_SYM_MODIFIER;  // UTF8PROC_CATEGORY_SK
extern const utf8lex_cat_t UTF8LEX_CAT_SYM_OTHER;  // UTF8PROC_CATEGORY_SO
extern const utf8lex_cat_t UTF8LEX_CAT_SEP_SPACE;  // UTF8PROC_CATEGORY_ZS
extern const utf8lex_cat_t UTF8LEX_CAT_SEP_LINE;  // UTF8PROC_CATEGORY_ZL
extern const utf8lex_cat_t UTF8LEX_CAT_SEP_PARAGRAPH;  // UTF8PROC_CATEGORY_ZP
extern const utf8lex_cat_t UTF8LEX_CAT_OTHER_CONTROL;  // UTF8PROC_CATEGORY_CC
extern const utf8lex_cat_t UTF8LEX_CAT_OTHER_FORMAT;  // UTF8PROC_CATEGORY_CF
extern const utf8lex_cat_t UTF8LEX_CAT_OTHER_SURROGATE;  // UTF8PROC_CATEGORY_CS
extern const utf8lex_cat_t UTF8LEX_CAT_OTHER_PRIVATE;  // UTF8PROC_CATEGORY_CO

//
// Separator rules specified by Unicode that are NOT obeyed
// by utf8proc (except when using its malloc()-based functions,
// and specifying options such as UTF8PROC_NLF2LS).
//
// From:
//     Unicode 15.1.0
//     https://www.unicode.org/reports/tr14/tr14-51.html#BK
//     https://www.unicode.org/reports/tr14/tr14-51.html#CR
//     https://www.unicode.org/reports/tr14/tr14-51.html#LF
//     https://www.unicode.org/reports/tr14/tr14-51.html#NL
//   Also for more details on history (such as VT / vertical tab), see:
//     https://www.unicode.org/standard/reports/tr13/tr13-5.html#Definitions
//
// 000A    LINE FEED (LF)
// 000B    LINE TABULATION (VT)
// 000C    FORM FEED (FF)
// 000D    CARRIAGE RETURN (CR) (unless followed by 000A LF)
// 0085    NEXT LINE (NEL)
// 2028    LINE SEPARATOR
// 2029    PARAGRAPH SEPARATOR
//
extern const utf8lex_cat_t UTF8LEX_EXT_SEP_LINE;  // Unicode line separators

//
// Combined categories, OR'ed together base categories e.g. letter
// can be upper, lower or title case, etc.:
//
extern const utf8lex_cat_t UTF8LEX_GROUP_OTHER;  // Other (control, format, etc)
extern const utf8lex_cat_t UTF8LEX_GROUP_NOT_OTHER;  // All but OTHER
extern const utf8lex_cat_t UTF8LEX_GROUP_LETTER;  // Letters
extern const utf8lex_cat_t UTF8LEX_GROUP_NOT_LETTER;  // All but LETTER
extern const utf8lex_cat_t UTF8LEX_GROUP_MARK;  // Marks
extern const utf8lex_cat_t UTF8LEX_GROUP_NOT_MARK;  // All but MARK
extern const utf8lex_cat_t UTF8LEX_GROUP_NUM;  // Numbers
extern const utf8lex_cat_t UTF8LEX_GROUP_NOT_NUM;  // All but NUM
extern const utf8lex_cat_t UTF8LEX_GROUP_PUNCT;  // Punctuation
extern const utf8lex_cat_t UTF8LEX_GROUP_NOT_PUNCT;  // All but PUNCT
extern const utf8lex_cat_t UTF8LEX_GROUP_SYM;  // Symbols
extern const utf8lex_cat_t UTF8LEX_GROUP_NOT_SYM;  // All but SYM
// Warning: Unicode considers \n, \r, \t, etc to be control characters!
//     https://www.unicode.org/charts/PDF/U0000.pdf
extern const utf8lex_cat_t UTF8LEX_GROUP_WHITESPACE;  // CAT_SEPs + EXT_SEPs
extern const utf8lex_cat_t UTF8LEX_GROUP_NOT_WHITESPACE;  // All but WHITESPACE
extern const utf8lex_cat_t UTF8LEX_GROUP_HSPACE;  // space, tab, ...
extern const utf8lex_cat_t UTF8LEX_GROUP_NOT_HSPACE;  // All but HSPACE
extern const utf8lex_cat_t UTF8LEX_GROUP_VSPACE;  // LF, CR, paragraph sep, ...
extern const utf8lex_cat_t UTF8LEX_GROUP_NOT_VSPACE;  // All but VSPACE
extern const utf8lex_cat_t UTF8LEX_GROUP_ALL;  // All categories
extern const utf8lex_cat_t UTF8LEX_CAT_MAX;

// Formats the specified OR'ed category/ies as a string,
// overwriting the specified str_pointer.
extern utf8lex_error_t utf8lex_format_cat(
        utf8lex_cat_t cat,
        unsigned char *str_pointer
        );
// Parses the specified string into OR'ed category/ies,
// overwriting the specified cat_pointer.
extern utf8lex_error_t utf8lex_parse_cat(
        utf8lex_cat_t *cat_pointer,
        unsigned char *str
        );

struct _STRUCT_utf8lex_definition_type
{
  char *name;  // Such as "CATEGORY", "LITERAL" or "REGEX".

  // During lexing, utf8lex_lex() will invoke this definition_type's
  // lex() function to determine 1) whether the text in the lexing
  // buffer matches this definition_type and, if so, then also
  // 2) initialize the specified token_pointer
  // (optionally updating the buffer and state lengths in the process).
  //
  // The rule contains the definition to use for lexing;
  // rule->definition is of type utf8lex_definition_t *,
  // so utf8lex_lex() calls rule->definition->definition_type->lex(...).
  //
  // The builtin cat, literal and regex definition types
  // provide this functionality; other definition types can be added,
  // if desired.
  utf8lex_error_t (*lex)(
          utf8lex_rule_t *rule,
          utf8lex_state_t *state,
          utf8lex_token_t *token_pointer
          );

  // When a rule is cleared, its definition_type will free any
  // memory used by its definition (such as a compiled regular expression).
  utf8lex_error_t (*clear)(
          utf8lex_definition_t *definition
          );
};

// A token definition that matches a sequence of N characters
// of a specific utf8lex_cat_t categpru, such as UTF8LEX_GROUP_WHITESPACE:
extern utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_CAT;

// A token definition that matches a literal string,
// such as "int" or "==" or "proc" and so on:
extern utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_LITERAL;

// A token definition that matches a regular expression,
// such as "^[0-9]+" or "[\\p{N}]+" or "[_\\p{L}][_\\p{L}\\p{N}]*" or "[\\s]+"
// and so on:
extern utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_REGEX;

// No more than (this many) utf8lex_definition_t's can be in a database.
extern const uint32_t UTF8LEX_DEFINITIONS_DB_LENGTH_MAX;

struct _STRUCT_utf8lex_definition
{
  // The definition_type must always be the first field in every
  // utf8lex_definition_t implementation (e.g. cat, literal, regex).
  utf8lex_definition_type_t *definition_type;

  // Unique id in the definitions database (0, 1, 2, ...).
  uint32_t id;

  // A name for this definition, typically all uppercase.
  unsigned char *name;

  // Next and previous definitions in the database (if any).  Can be NULL.
  utf8lex_definition_t *next;
  utf8lex_definition_t *prev;
};

extern utf8lex_error_t utf8lex_definition_find(
        utf8lex_definition_t *first_definition,  // Database to search.
        unsigned char *name,  // Name of definition to search for.
        utf8lex_definition_t ** found_pointer  // Gets set when found.
        );
extern utf8lex_error_t utf8lex_definition_find_by_id(
        utf8lex_definition_t *first_definition,  // Database to search.
        uint32_t id,  // The id of the definition to search for.
        utf8lex_definition_t ** found_pointer  // Gets set when found.
        );


#define UTF8LEX_CAT_FORMAT_MAX_LENGTH 512

struct _STRUCT_utf8lex_cat_definition
{
  utf8lex_definition_t base;

  utf8lex_cat_t cat;  // The category, such as UTF8LEX_GROUP_LETTER.
  char str[UTF8LEX_CAT_FORMAT_MAX_LENGTH];  // e.g. "LETTER_UPPER|LETTER_LOWER".
  int min;  // Minimum consecutive occurrences of the cat (1 or more).
  int max;  // Maximum consecutive occurrences of the cat (-1 for no limit).
};

extern utf8lex_error_t utf8lex_cat_definition_init(
        utf8lex_cat_definition_t *self,
        utf8lex_definition_t *prev,  // Previous definition in DB, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        utf8lex_cat_t cat,  // The category, such as UTF8LEX_GROUP_LETTER.
        int min,  // Minimum consecutive occurrences of the cat (1 or more).
        int max  // Maximum consecutive occurrences (-1 = no limit).
        );
extern utf8lex_error_t utf8lex_cat_definition_clear(
        // self must be utf8lex_cat_definition_t *:
        utf8lex_definition_t *self
        );

struct _STRUCT_utf8lex_literal_definition
{
  utf8lex_definition_t base;

  unsigned char *str;
  // # bytes, chars, graphemes and lines in this literal,
  // plus char and grapheme resets to account for newlines
  // (after == -1 -> no reset; after >= 0 -> reset state, buffer positions
  // to the specified char, grapheme locations):
  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];
};

extern utf8lex_error_t utf8lex_literal_definition_init(
        utf8lex_literal_definition_t *self,
        utf8lex_definition_t *prev,  // Previous definition in DB, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        unsigned char *str
        );
extern utf8lex_error_t utf8lex_literal_definition_clear(
        // self must be utf8lex_literal_definition_t *:
        utf8lex_definition_t *self
        );

struct _STRUCT_utf8lex_regex_definition
{
  utf8lex_definition_t base;

  unsigned char *pattern;
  pcre2_code *regex;
};

// PCRE2 regex definition language:
//     https://pcre2project.github.io/pcre2/doc/html/pcre2definition.html
extern utf8lex_error_t utf8lex_regex_definition_init(
        utf8lex_regex_definition_t *self,
        utf8lex_definition_t *prev,  // Previous definition in DB, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        unsigned char *pattern
        );
extern utf8lex_error_t utf8lex_regex_definition_clear(
        // self must be utf8lex_regex_definition_t *:
        utf8lex_definition_t *self
        );

// No more than (this many) utf8lex_rule_t's can be in a database.
extern const uint32_t UTF8LEX_RULES_DB_LENGTH_MAX;

struct _STRUCT_utf8lex_rule
{
  utf8lex_rule_t *prev;
  utf8lex_rule_t *next;

  uint32_t id;
  unsigned char *name;
  utf8lex_definition_t *definition;  // Such as cat, literal, regex.
  unsigned char *code;
  size_t code_length_bytes;
};

extern utf8lex_error_t utf8lex_rule_init(
        utf8lex_rule_t *self,
        utf8lex_rule_t *prev,
        unsigned char *name,
        utf8lex_definition_t *definition,  // Such as cat, literal, regex.
        unsigned char *code,
        size_t code_length_bytes
        );
extern utf8lex_error_t utf8lex_rule_clear(
        utf8lex_rule_t *self
        );

extern utf8lex_error_t utf8lex_rule_find(
        utf8lex_rule_t *first_rule,  // Database to search.
        unsigned char *name,  // Name of rule to search for.
        utf8lex_rule_t ** found_pointer  // Gets set when found.
        );
extern utf8lex_error_t utf8lex_rule_find_by_id(
        utf8lex_rule_t *first_rule,  // Database to search.
        uint32_t id,  // The id of the rule to search for.
        utf8lex_rule_t ** found_pointer  // Gets set when found.
        );

struct _STRUCT_utf8lex_token
{
  utf8lex_rule_t *rule;

  int start_byte;  // Bytes offset into str where token starts.
  int length_bytes;  // # bytes in token.
  utf8lex_string_t *str;

  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];  // Absolute location of token.
};

extern utf8lex_error_t utf8lex_token_init(
        utf8lex_token_t *self,
        utf8lex_rule_t *rule,
        utf8lex_location_t token_loc[UTF8LEX_UNIT_MAX],  // Resets, lengths.
        utf8lex_state_t *state  // For buffer and absolute location.
        );
extern utf8lex_error_t utf8lex_token_clear(
        utf8lex_token_t *self
        );

// Returns UTF8LEX_MORE if the destination string truncates the token:
extern utf8lex_error_t utf8lex_token_copy_string(
        utf8lex_token_t *self,
        unsigned char *str,
        size_t max_bytes);

struct _STRUCT_ut8lex_state
{
  utf8lex_buffer_t *buffer;
  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];
};

extern utf8lex_error_t utf8lex_state_init(
        utf8lex_state_t *self,
        utf8lex_buffer_t *buffer
        );
extern utf8lex_error_t utf8lex_state_clear(
        utf8lex_state_t *self
        );

// Determines the category/ies of the specified Unicode 32 bit codepoint.
// Pass in a reference to the utf8lex_cat_t; on success, the specified
// utf8lex_cat_t pointer will be overwritten.
extern utf8lex_error_t utf8lex_cat_codepoint(
        int32_t codepoint,
        utf8lex_cat_t *cat_pointer  // Mutable.
        );

// Reads to the end of a grapheme, sets the first codepoint of the
// grapheme, and the number of bytes read.
// Note that CR, LF (U+000D, U+000A), often equivalent to '\r\n')
// is treated as a special grapheme cluster (however LF, CR is NOT for now),
// since the 2 characters, combined in sequence, usually represent
// one single line separator.
// The state is used only for the string buffer to read from,
// not for its location info.
// The offset, lengths, codepoint and cat are all set upon
// successfully reading one complete grapheme cluster.
// loc[*].after will be -1 if no newlines were encountered, or 0
// or more if the character / grapheme positions were reset to 0 at newline.
// (Bytes and lines will never have their after locations reset, always -1.)
extern utf8lex_error_t utf8lex_read_grapheme(
        utf8lex_state_t *state,
        off_t *offset_pointer,  // Mutable.
        utf8lex_location_t loc_pointer[UTF8LEX_UNIT_MAX], // Mutable
        int32_t *codepoint_pointer,  // Mutable.
        utf8lex_cat_t *cat_pointer  // Mutable.
        );


struct _STRUCT_utf8lex_target_language
{
  unsigned char *name;
  unsigned char *extension;
};

extern const utf8lex_target_language_t *TARGET_LANGUAGE_C;


extern utf8lex_error_t utf8lex_generate(
        const utf8lex_target_language_t *target_language,
        unsigned char *lex_file_path,
        unsigned char *template_dir_path,
        unsigned char *generated_file_path,
        utf8lex_state_t *state_pointer  // Will be initialized.
        );

#endif  // UTF8LEX_H_INCLUDED
