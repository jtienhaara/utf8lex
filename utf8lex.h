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

#include <inttypes.h>

// 8-bit character units for pcre2:
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

typedef struct _STRUCT_utf8lex_buffer           utf8lex_buffer_t;
typedef uint32_t                                utf8lex_cat_t;
typedef struct _STRUCT_utf8lex_category         utf8lex_category_t;
typedef enum _ENUM_utf8lex_error                utf8lex_error_t;
typedef struct _STRUCT_utf8lex_grapheme         utf8lex_grapheme_t;
typedef struct _STRUCT_utf8lex_location         utf8lex_location_t;
typedef enum _ENUM_utf8lex_pattern_type         utf8lex_pattern_type_t;
typedef struct _STRUCT_ut8lex_state             utf8lex_state_t;
typedef struct _STRUCT_utf8lex_string           utf8lex_string_t;
typedef struct _STRUCT_utf8lex_token            utf8lex_token_t;
typedef struct _STRUCT_utf8lex_token_type       utf8lex_token_type_t;
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
        utf8lex_token_type_t * first_token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token
        );


enum _ENUM_utf8lex_error
{
  UTF8LEX_OK = 0,
  UTF8LEX_EOF,  // Lexing completed successfully.

  UTF8LEX_MORE,  // Need to read in more bytes from the source.
  UTF8LEX_NO_MATCH,  // Could not match bytes against any pattern(s).

  UTF8LEX_ERROR_NULL_POINTER,
  UTF8LEX_ERROR_CHAIN_INSERT,  // Can't insert links into the chain, only append
  UTF8LEX_ERROR_CAT,  // Invalid cat (category id) NONE < cat < MAX / not found.
  UTF8LEX_ERROR_PATTERN_TYPE,  // Invalid pattern type NONE < p.t. < MAX.
  UTF8LEX_ERROR_REGEX,  // Matching against a regular expression failed.
  UTF8LEX_ERROR_INFINITE_LOOP,  // Aborted, possible infinite loop detected.
  UTF8LEX_ERROR_BAD_LENGTH,  // Negative length.
  UTF8LEX_ERROR_BAD_OFFSET,  // Negative offset, or too close to end of string.
  UTF8LEX_ERROR_BAD_START,  // Negative start, or too close to end of string.
  UTF8LEX_ERROR_BAD_END,  // Negative end, or < start, or too close to end.
  UTF8LEX_ERROR_BAD_REGEX,  // Could not compile regex pattern.
  UTF8LEX_ERROR_BAD_ERROR,  // Invalid error NONE <= e <= MAX.

  UTF8LEX_ERROR_MAX
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
// Combined categories, OR'ed together base categories e.g. letter
// can be upper, lower or title case, etc.:
//
extern const utf8lex_cat_t UTF8LEX_GROUP_OTHER;  // Other (control, format, etc)
extern const utf8lex_cat_t UTF8LEX_GROUP_LETTER;  // Letters
extern const utf8lex_cat_t UTF8LEX_GROUP_MARK;  // Marks
extern const utf8lex_cat_t UTF8LEX_GROUP_NUM;  // Numbers
extern const utf8lex_cat_t UTF8LEX_GROUP_PUNCT;  // Punctuation
extern const utf8lex_cat_t UTF8LEX_GROUP_SYM;  // Symbols
// Warning: Unicode considers \n, \r, \t, etc to be control characters!
//     https://www.unicode.org/charts/PDF/U0000.pdf
extern const utf8lex_cat_t UTF8LEX_GROUP_WHITESPACE; // Separators
extern const utf8lex_cat_t UTF8LEX_CAT_MAX;

struct _STRUCT_utf8lex_category
{
  utf8lex_category_t *next;
  utf8lex_category_t *prev;

  utf8lex_cat_t cat;  // Bits ORed together to get multiple categories
  unsigned char *name;
};

extern utf8lex_error_t utf8lex_category_init(
        utf8lex_category_t *self,
        utf8lex_category_t *prev,
        utf8lex_cat_t cat,
        unsigned char *name
        );
extern utf8lex_error_t utf8lex_category_clear(
        utf8lex_category_t *self
        );

extern utf8lex_error_t utf8lex_find_category(
        utf8lex_category_t *category_chain,
        utf8lex_cat_t cat,
        utf8lex_category_t **found
        );

struct _STRUCT_utf8lex_grapheme
{
  utf8lex_string_t *str;
  off_t offset;
  size_t length_bytes;
  utf8lex_cat_t cat;  // Must be 1 category for a grapheme (no A | B).
};

extern utf8lex_error_t utf8lex_grapheme_init(
        utf8lex_grapheme_t *self,
        utf8lex_string_t *str,
        off_t offset,
        size_t length_bytes,
        utf8lex_cat_t cat
        );
extern utf8lex_error_t utf8lex_grapheme_clear(
        utf8lex_grapheme_t *self
        );

enum _ENUM_utf8lex_pattern_type
{
  UTF8LEX_PATTERN_TYPE_NONE = 0,

  UTF8LEX_PATTERN_TYPE_CLASS,
  UTF8LEX_PATTERN_TYPE_REGEX,
  UTF8LEX_PATTERN_TYPE_STRING,

  UTF8LEX_PATTERN_TYPE_MAX
};

struct _STRUCT_utf8lex_token_type
{
  utf8lex_token_type_t *prev;
  utf8lex_token_type_t *next;

  uint32_t id;
  unsigned char *name;
  utf8lex_pattern_type_t pattern_type;
  union
  {
    utf8lex_cat_t cat;
    pcre2_code *regex;
    unsigned char *str;
  } pattern;
  unsigned char *code;
};

// PCRE2 regex pattern language:
//     https://pcre2project.github.io/pcre2/doc/html/pcre2pattern.html
extern utf8lex_error_t utf8lex_token_type_init(
        utf8lex_token_type_t *self,
        utf8lex_token_type_t *prev,
        unsigned char *name,
        utf8lex_pattern_type_t pattern_type,
        utf8lex_cat_t cat_pattern,
        unsigned char *regex_pattern,
        unsigned char *str_pattern,
        unsigned char *code
        );
extern utf8lex_error_t utf8lex_token_type_clear(
        utf8lex_token_type_t *self
        );

enum _ENUM_utf8lex_unit
{
  UTF8LEX_UNIT_NONE = -1,

  UTF8LEX_UNIT_BYTE = 0,
  UTF8LEX_UNIT_CHAR,
  UTF8LEX_UNIT_GRAPHEME,
  UTF8LEX_UNIT_WORD,
  UTF8LEX_UNIT_LINE,
  UTF8LEX_UNIT_PARAGRAPH,
  UTF8LEX_UNIT_SECTION,

  UTF8LEX_UNIT_MAX
};

struct _STRUCT_utf8lex_location
{
  int start;  // First byte / char / grapheme / and so on of a token.
  int end;  // Last byte / char / grapheme / and so on of a token.
};

extern utf8lex_error_t utf8lex_location_init(
        utf8lex_location_t *self,
        int start,
        int end
        );
extern utf8lex_error_t utf8lex_location_clear(
        utf8lex_location_t *self
        );

struct _STRUCT_utf8lex_token
{
  utf8lex_token_type_t *token_type;

  utf8lex_string_t *str;
  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];
};

extern utf8lex_error_t utf8lex_token_init(
        utf8lex_token_t *self,
        utf8lex_token_type_t *token_type,
        utf8lex_string_t *str,
        utf8lex_state_t *state  // For location in bytes, chars, etc.
        );
extern utf8lex_error_t utf8lex_token_clear(
        utf8lex_token_t *self
        );

// Returns UTF8LEX_MORE if the destination string truncates the token:
extern utf8lex_error_t utf8lex_token_copy_string(
        utf8lex_token_t *self,
        unsigned char *str,
        size_t max_bytes);

// No more than (this many) buffers can be chained together:
extern const uint32_t UTF8LEX_BUFFER_STRINGS_MAX;

struct _STRUCT_utf8lex_buffer
{
  utf8lex_buffer_t *next;
  utf8lex_buffer_t *prev;

  utf8lex_string_t *str;
};

extern utf8lex_error_t utf8lex_buffer_init(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *prev,
        utf8lex_string_t *str
        );
extern utf8lex_error_t utf8lex_buffer_clear(
        utf8lex_buffer_t *self
        );

extern utf8lex_error_t utf8lex_buffer_add(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *tail
        );

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

#endif  // UTF8LEX_H_INCLUDED
