/*
 * utf8lex
 * Copyright © 2023-2025 Johann Tienhaara
 * All rights reserved
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
typedef struct _STRUCT_utf8lex_definition_type  utf8lex_definition_type_t;
typedef enum _ENUM_utf8lex_error                utf8lex_error_t;
typedef struct _STRUCT_utf8lex_literal_definition utf8lex_literal_definition_t;
typedef struct _STRUCT_utf8lex_location         utf8lex_location_t;
typedef struct _STRUCT_utf8lex_multi_definition utf8lex_multi_definition_t;
typedef enum _ENUM_utf8lex_multi_type           utf8lex_multi_type_t;
typedef enum _ENUM_utf8lex_printable_flag       utf8lex_printable_flag_t;
typedef struct _STRUCT_utf8lex_reference        utf8lex_reference_t;
typedef struct _STRUCT_utf8lex_regex_definition utf8lex_regex_definition_t;
typedef struct _STRUCT_utf8lex_rule             utf8lex_rule_t;
typedef struct _STRUCT_utf8lex_state            utf8lex_state_t;
typedef struct _STRUCT_utf8lex_string           utf8lex_string_t;
typedef struct _STRUCT_utf8lex_sub_token        utf8lex_sub_token_t;
typedef struct _STRUCT_utf8lex_target_language  utf8lex_target_language_t;
typedef struct _STRUCT_utf8lex_token            utf8lex_token_t;
typedef enum _ENUM_utf8lex_unit                 utf8lex_unit_t;

// Used by yylex() generated by utf8lex_generate(...):
typedef struct STRUCT_utf8lex_lloc              utf8lex_lloc_t;


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
  UTF8LEX_ERROR_EMPTY_DEFINITION,  // Literals cannot be "", multis cannot be []
  UTF8LEX_ERROR_MAX_LENGTH,  // Too many (rules, definitions, ...) in database.
  UTF8LEX_ERROR_NOT_A_RULE,  // Cannot define rule code for e.g. a definition.
  UTF8LEX_ERROR_NOT_FOUND,  // ..._find() did not match any objects.
  UTF8LEX_ERROR_NOT_IMPLEMENTED,  // Feature not implemented in utf8lex, fix me!
  UTF8LEX_ERROR_REGEX,  // Matching against a regular expression failed.
  UTF8LEX_ERROR_UNIT,  // Invalid unit must be NONE < unit < MAX.
  UTF8LEX_ERROR_UNRESOLVED_DEFINITION,  // Multi definitions must be resove()d.
  UTF8LEX_ERROR_INFINITE_LOOP,  // Aborted, possible infinite loop detected.

  UTF8LEX_ERROR_BAD_LENGTH,  // Negative length, < start, too close to end.
  UTF8LEX_ERROR_BAD_OFFSET,  // Negative offset, or too close to end of string.
  UTF8LEX_ERROR_BAD_START,  // Negative start, or too close to end of string.
  UTF8LEX_ERROR_BAD_AFTER,  // After is neither reset (-1) nor valid new start.
  UTF8LEX_ERROR_BAD_HASH,  // Hash is incorrect.
  UTF8LEX_ERROR_BAD_ID,  // Rule, definition and sub-token ids must be > 0.
  UTF8LEX_ERROR_BAD_MIN,  // Min must be 0 or greater.
  UTF8LEX_ERROR_BAD_MAX,  // Max must be >= min, or -1 for no limit.
  UTF8LEX_ERROR_BAD_MULTI_TYPE,  // Multi type must be sequence, OR, etc.
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

  unsigned long hash;  // The sum of bytes / chars / graphemes / and so on.
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
extern utf8lex_error_t utf8lex_state_string(
        utf8lex_string_t *str,
        utf8lex_state_t *state
        );

// Print error message to the specified string:
extern utf8lex_error_t utf8lex_error_string(
        utf8lex_string_t *str,
        utf8lex_error_t error
        );

// Helper to initialize a statically declared utf8lex_string_t.
extern utf8lex_error_t utf8lex_string(
        utf8lex_string_t *self,
        int max_length_bytes,
        unsigned char *content
        );


// No more than (this many) buffers can be chained together:
#define UTF8LEX_BUFFER_STRINGS_MAX 16384

struct _STRUCT_utf8lex_buffer
{
  utf8lex_buffer_t *next;
  utf8lex_buffer_t *prev;

  int fd;  // File descriptor, or -1 if no open file backs this buffer.
  FILE *fp;  // File descriptor for fopen(), fread(), or NULL.

  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];  // Length of str.
  utf8lex_string_t *str;  // One chunk of text from the file / other source.
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

// All explicitly defined CATs and GROUPs:
#define UTF8LEX_NUM_CATEGORIES 50
extern const utf8lex_cat_t UTF8LEX_CATEGORIES[];

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

// No more than (this many) utf8lex_definition_t's can be in a database
// (to prevent infinite loops due to adding the same definition twice etc).
// Warning: setting this too high can cause a program to mysteriously
// segfault at the very start of a static function (as static
// heap space is being initialized, or something like that.)
#define UTF8LEX_DEFINITIONS_DB_LENGTH_MAX 1024

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

enum _ENUM_utf8lex_printable_flag
{
  UTF8LEX_PRINTABLE_NONE = 0,

  UTF8LEX_PRINTABLE_BACKSLASH = 0x0001,        // "\\"
  UTF8LEX_PRINTABLE_ALERT = 0x0002,            // "\a"
  UTF8LEX_PRINTABLE_BACKSPACE = 0x0004,        // "\b"
  UTF8LEX_PRINTABLE_FORM_FEED = 0x0008,        // "\f"
  UTF8LEX_PRINTABLE_NEWLINE = 0x0010,          // "\n"
  UTF8LEX_PRINTABLE_CARRIAGE_RETURN = 0x0020,  // "\r"
  UTF8LEX_PRINTABLE_TAB = 0x0040,              // "\t"
  UTF8LEX_PRINTABLE_VERTICAL_TAB = 0x0080,     // "\v"
  UTF8LEX_PRINTABLE_QUOTE = 0x0100,            // "\""

  UTF8LEX_PRINTABLE_ALL = 0x01FF,

  UTF8LEX_PRINTABLE_MAX
};


// Makes a UTF-8 string printable by converting certain characters
// such as backslash (\) and newline (\n) into C escape sequences
// such as "\\", "\n", and so on.
// Useful, for example, to recreate the C string for a literal
// (literal_definition->str) or regular expression (regex_definition->pattern).
// Returns UTF8LEX_MORE if max_bytes is not big enough to
// contain the printable version of the specified str (in which case
// as many bytes as possible have been written to the target).
// Conversions done:
// \\, \a, \b, \f, \n, \r, \t, \v, \"
// (from https://pubs.opengroup.org/onlinepubs/7908799/xbd/notation.html
// plus quote)
// Each can be turned on/off with the flags, e.g. to only convert
// backslash (\) and quote ("):
//     utf8lex_printable_str(..., UTF8LEX_PRINTABLE_BACKSLASH
//                                | UTF8LEX_PRINTABLE_QUOTE);
extern utf8lex_error_t utf8lex_printable_str(
        unsigned char *printable_str,  // Target printable version of string.
        size_t max_bytes,  // Max bytes to write to printable_str including \0.
        unsigned char *str,  // Source string to convert.  Must be 0-terminated.
        utf8lex_printable_flag_t flags  // Which char(s) to convert.  Default ALL.
        );


// A token definition that matches a sequence of N characters
// of a specific utf8lex_cat_t categpru, such as UTF8LEX_GROUP_WHITESPACE:
extern utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_CAT;

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


// A token definition that matches a literal string,
// such as "int" or "==" or "proc" and so on:
extern utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_LITERAL;

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


// A token definition that matches one or more other definitions,
// in sequence and/or logically grouped.
//
// For example, the following:
//
//     LETTER | UNDERSCORE (LETTER | UNDERSCORE | NUMBER)*
//
// might represent a definition comprising the following sequence:
//
//     1. Either a token matching the LETTER definition,
//        or a token matching the UNDERSCORE definition; followed by
//     2. 0 or more, in sequence, of:
//        Either a token matching the LETTER definition,
//        or a token matching the UNDERSCORE definition,
//        or a token matching the NUMBER definition.
//
// So if the LETTER definition covers a single ASCii character in 'a' - 'z'
// or 'A' - 'Z', and UNDERSCORE matches exactly "_", and NUMBER
// matches a single ASCii character in '0' - '9', then the above
// multi definition would be equivalent to:
//
//     ^[a-zA-Z_][a-zA-Z_0-9]*$
//
// The multi-definition expression is comprised of:
//
//     - Names of other definitions, D1, D2, and so on;
//     - (): Parenthetical expressions, "(x1)";
//     - *: 0 or more of a given expression, "x1*";
//     - +: 1 or more of a given expression, "x1+";
//     - |: Logically ORed expressions, "x1 | x2" (either x1 or x2, not both);
//     - Sequences of expressions, "x1 x2".
//
// Order of operations is above, so:
//
//     D1 D2 | D3    == D1 followed by (D2 or D3)
//     D1 D2* | D3+  == D1 followed by ((or or more D2) or (1 or more D3))
//     D1 (D2 | D3)* == D1 followed by (0 or more D2s and/or D3s)
//
extern utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_MULTI;

// No more than (this many) utf8lex_multi_definition_t's can be
// nested using |, (), and so on
// (to prevent infinite loops due to adding the same definition twice etc).
#define UTF8LEX_MULTI_DEFINITION_DEPTH_MAX 256

// No more than (this many) utf8lex_reference_t's can be in
// a multi-definition's references.
// (to prevent infinite loops due to adding the same reference twice etc).
#define UTF8LEX_REFERENCES_LENGTH_MAX 256

// No more than (this many) utf8lex_sub_token_t's can be in
// a token (to prevent infinite loops).  Note that a sub-token
// can contain sub-tokens, so this is the maximum size of the entire
// tree, not just the immediate children of the token.
#define UTF8LEX_SUB_TOKENS_LENGTH_MAX 256

struct _STRUCT_utf8lex_reference
{
  // References are used for multi-definitions.
  //
  // Each multi-definition references other definitions in a sequence,
  // or ORed together, and so on.
  //
  //    For example, COMMENT as a multi-definition references
  //    COMMENT_OPEN, COMMENT_BODY and COMMENT_CLOSE:
  //
  //        COMMENT COMMENT_OPEN COMMENT_BODY COMMENT_CLOSE
  //
  //    And NUMBER references FLOAT and INT:
  //
  //        NUMBER FLOAT | INT
  //
  // References are NOT thread-safe, because they can be resolved
  // at any time, without locking.  Do NOT share a reference across
  // multiple threads until it has been resolved.

  unsigned char *definition_name;
  utf8lex_definition_t *definition_or_null;  // NULL until resolved.

  int min;  // Minimum number of tokens matching this definition.
  int max;  // Maximum tokens matching, or -1 for no limit.

  utf8lex_reference_t *next;  // Next in a sequence or logical OR of references.
  utf8lex_reference_t *prev;  // Previous in a sequence or logical OR etc.

  utf8lex_multi_definition_t *parent;  // Parent multi-definition, or NULL.
};

extern utf8lex_error_t utf8lex_reference_init(
        utf8lex_reference_t *self,
        utf8lex_reference_t *prev,  // Previous reference, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        int min,  // Minimum number of tokens (0 or more).
        int max,  // Maximum tokens, or -1 for no limit.
        utf8lex_multi_definition_t *parent  // Parent multi-definition.
        );
extern utf8lex_error_t utf8lex_reference_clear(
        utf8lex_reference_t *self
        );

extern utf8lex_error_t utf8lex_reference_resolve(
        utf8lex_reference_t *self,
        utf8lex_definition_t *db  // The main database to resolve references.
        );

enum _ENUM_utf8lex_multi_type
{
  UTF8LEX_MULTI_TYPE_NONE = 0,
  UTF8LEX_MULTI_TYPE_SEQUENCE,
  UTF8LEX_MULTI_TYPE_OR,
  UTF8LEX_MULTI_TYPE_MAX
};

struct _STRUCT_utf8lex_multi_definition
{
  // A multi-definition is NOT thread-safe, since the references and db
  // are built up by adding one reference at a time.  Do not share
  // a multi-definition across multiple threads until after the
  // definition is complete and has been resolved.

  utf8lex_definition_t base;

  utf8lex_multi_type_t multi_type;  // Sequence or ORed references, and so on.
  utf8lex_reference_t *references;  // Sequence / ORed definition names.
  utf8lex_definition_t *db;  // Logical | () etc sub-expression definitions.

  utf8lex_multi_definition_t *parent;  // Parent multi-definition, or NULL.
};

extern utf8lex_error_t utf8lex_multi_definition_init(
        utf8lex_multi_definition_t *self,
        utf8lex_definition_t *prev,  // Previous definition in DB, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        utf8lex_multi_definition_t *parent,  // Parent or NULL.
        utf8lex_multi_type_t multi_type  // Sequence or ORed references, etc.
        );
extern utf8lex_error_t utf8lex_multi_definition_clear(
        // self must be utf8lex_multi_definition_t *:
        utf8lex_definition_t *self
        );

extern utf8lex_error_t utf8lex_multi_definition_resolve(
        utf8lex_multi_definition_t *self,
        utf8lex_definition_t *db  // The main database to resolve references.
        );


// A token definition that matches a regular expression,
// such as "^[0-9]+" or "[\\p{N}]+" or "[_\\p{L}][_\\p{L}\\p{N}]*" or "[\\s]+"
// and so on:
extern utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_REGEX;

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
// (to prevent infinite loops due to adding the same rule twice etc).
// Warning: setting this too high can cause a program to mysteriously
// segfault at the very start of a static function (as static
// heap space is being initialized, or something like that.)
#define UTF8LEX_RULES_DB_LENGTH_MAX 1024

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
        size_t code_length_bytes  // (size_t) -1 to use strlen(code).
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
  utf8lex_rule_t *rule;  // The rule that matched this token.
  utf8lex_definition_t *definition;  // The matching definition.

  int start_byte;  // Bytes offset into str where token starts.
  int length_bytes;  // # bytes in token.
  utf8lex_string_t *str;

  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];  // Absolute location of token.

  // Each token that matches a multi-definition has a sub-token
  // for each reference definition.
  //
  // For example, the token:
  //
  //     /* This is a comment */
  //
  // might match the whole COMMENT multi-definition, with the toplevel
  // token containing sub-tokens:
  //
  //     COMMENT_OPEN  is_match = true str = "/*"
  //     COMMENT_BODY  is_match = true str = " This is a comment "
  //     COMMENT_CLOSE is_match = true str = "*/"
  //
  // Or the token:
  //
  //     1.2345
  //
  // might match the whole NUMBER multi-definition, with the toplevel
  // token containing sub-tokens:
  //
  //     FLOAT is_match = true  str = "1.2345"
  //     INT   is_match = false str = ""
  //
  // Each instance of a variable-length definition is matched.
  // So, for example, a multi-definition referencing component OPTIONAL?
  // will generate 0 or 1 sub-tokens for the OPTIONAL reference.
  // A multi-definition referencing ELEMENT* will generate 0 or more
  // sub-tokens for the ELEMENT reference.
  //
  utf8lex_sub_token_t *sub_tokens;  // Can be NULL.
  utf8lex_token_t *parent_or_null;  // If this is a sub-token.
};

extern utf8lex_error_t utf8lex_token_init(
        utf8lex_token_t *self,
        utf8lex_rule_t *rule,
        utf8lex_definition_t *definition,
        utf8lex_location_t token_loc[UTF8LEX_UNIT_MAX],  // Resets, lengths.
        utf8lex_state_t *state  // For buffer and absolute location.
        );
extern utf8lex_error_t utf8lex_token_copy(
        utf8lex_token_t *from,
        utf8lex_token_t *to,
        utf8lex_state_t *state
        );
extern utf8lex_error_t utf8lex_token_clear(
        utf8lex_token_t *self
        );

// Copies the token text into the specified string, overwriting it.
// Returns UTF8LEX_MORE if the destination string truncates the token:
extern utf8lex_error_t utf8lex_token_copy_string(
        utf8lex_token_t *self,
        unsigned char *str,
        size_t max_bytes);
// Concatenates the token text to the end of the specified string.
// Returns UTF8LEX_MORE if the destination string truncates the token:
extern utf8lex_error_t utf8lex_token_cat_string(
        utf8lex_token_t *self,
        unsigned char *str,  // Text will be concatenated starting at '\0'.
        size_t max_bytes);


struct _STRUCT_utf8lex_sub_token
{
  // Multiple sub-tokens can match a single definition.
  // For example, with a sequence multi-definition:
  //
  //     DIGIT*
  //
  // There can be 0 or more sub-tokens matching the DIGIT definition.
  //
  // Definitions can also be matched at different levels of a
  // multi-definition tree.  For example the FLOAT multi-definition
  // might have multiple instances of the DIGIT reference:
  //
  //     FLOAT DIGIT* (DOT DIGIT+)?
  //
  // NOT thread-safe.  Do not share a utf8lex_sub_token across threads.
  //
  uint32_t id;  // id of this sub-token == id of the matched definition.
  unsigned char *name;  // sub-token name == name of the matched definition.

  utf8lex_token_t token;

  utf8lex_sub_token_t *next;  
  utf8lex_sub_token_t *prev;
};

extern utf8lex_error_t utf8lex_sub_token_init(
        utf8lex_sub_token_t *self,
        utf8lex_sub_token_t *prev,
        utf8lex_token_t *token_to_copy,
        utf8lex_state_t *state
        );
extern utf8lex_error_t utf8lex_sub_token_clear(
        utf8lex_sub_token_t *self
        );

extern utf8lex_error_t utf8lex_sub_token_find(
        utf8lex_sub_token_t *first_sub_token,  // Database to search
        unsigned char *name,
        utf8lex_sub_token_t **found_pointer  // Mutable.
        );
extern utf8lex_error_t utf8lex_sub_token_find_by_id(
        utf8lex_sub_token_t *first_sub_token,  // Database to search
        uint32_t id,
        utf8lex_sub_token_t **found_pointer  // Mutable.
        );

// Copies the sub-token text into the specified string, overwriting it.
// Returns UTF8LEX_MORE if the destination string truncates the sub-token:
extern utf8lex_error_t utf8lex_sub_token_copy_string(
        utf8lex_sub_token_t *self,
        unsigned char *str,
        size_t max_bytes);

// Concatenates the sub-token text to the end of the specified string.
// Returns UTF8LEX_MORE if the destination string truncates the sub-token:
extern utf8lex_error_t utf8lex_sub_token_cat_string(
        utf8lex_sub_token_t *self,
        unsigned char *str,  // Text will be concatenated starting at '\0'.
        size_t max_bytes);


struct _STRUCT_utf8lex_state
{
  utf8lex_buffer_t *buffer;  // Current buffer being lexed.
  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];  // Current location within buffer.

  // Tokens matching multi-definitions have component sub-tokens
  // matching the children of the multi-definition.
  // NOT thread-safe -- do not share a utf8lex_state_t across threads.
  size_t num_used_sub_tokens;
  utf8lex_sub_token_t sub_tokens[UTF8LEX_SUB_TOKENS_LENGTH_MAX];
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

// Used by yylex() generated by utf8lex_generate(...):
struct STRUCT_utf8lex_lloc
{
  int first_line;
  int first_column;

  int last_line;
  int last_column;

  int start_byte;
  int length_bytes;

  int start_char;
  int length_chars;

  int start_grapheme;
  int length_graphemes;

  int start_line;
  int length_lines;
};

// Debug trace logs for obnoxious C bugs:
// #define UTF8LEX_DEBUG(...) fprintf(stdout, "%s %d %s\n", __FILE__, __LINE__, __VA_ARGS__); fflush(stdout)
#define UTF8LEX_DEBUG(...)

// Common token ids used by .c file generated from .l file:
#define YYEOF -1
#define YYerror -2
// (the remainder of the token ids will be >= int 0)

#endif  // UTF8LEX_H_INCLUDED
