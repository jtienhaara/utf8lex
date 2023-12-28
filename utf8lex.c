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

#include <stdio.h>
#include <inttypes.h>  // For uint32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strlen(), memcpy().
#include <utf8proc.h>

// 8-bit character units for pcre2:
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "utf8lex.h"

//
// Base categories are equivalent to (but not equal to) those
// in the utf8proc library (UTF8PROC_CATEGORY_LU, etc):
//
const utf8lex_cat_t UTF8LEX_CAT_NONE = 0x00000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_NA = 0x00000001;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_UPPER = 0x00000002;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_LOWER = 0x00000004;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_TITLE = 0x00000008;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_MODIFIER = 0x00000010;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_OTHER = 0x00000020;
const utf8lex_cat_t UTF8LEX_CAT_MARK_NON_SPACING = 0x00000040;
const utf8lex_cat_t UTF8LEX_CAT_MARK_SPACING_COMBINING = 0x00000080;
const utf8lex_cat_t UTF8LEX_CAT_MARK_ENCLOSING = 0x00000100;
const utf8lex_cat_t UTF8LEX_CAT_NUM_DECIMAL = 0x00000200;
const utf8lex_cat_t UTF8LEX_CAT_NUM_LETTER = 0x00000400;
const utf8lex_cat_t UTF8LEX_CAT_NUM_OTHER = 0x00000800;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_CONNECTOR = 0x00001000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_DASH = 0x00002000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_OPEN = 0x00004000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_CLOSE = 0x00008000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_QUOTE_OPEN = 0x00010000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_QUOTE_CLOSE = 0x00020000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_OTHER = 0x00040000;
const utf8lex_cat_t UTF8LEX_CAT_SYM_MATH = 0x00080000;
const utf8lex_cat_t UTF8LEX_CAT_SYM_CURRENCY = 0x00100000;
const utf8lex_cat_t UTF8LEX_CAT_SYM_MODIFIER = 0x00200000;
const utf8lex_cat_t UTF8LEX_CAT_SYM_OTHER = 0x00400000;
const utf8lex_cat_t UTF8LEX_CAT_SEP_SPACE = 0x00800000;
const utf8lex_cat_t UTF8LEX_CAT_SEP_LINE = 0x01000000;
const utf8lex_cat_t UTF8LEX_CAT_SEP_PARAGRAPH = 0x02000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_CONTROL = 0x04000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_FORMAT = 0x08000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_SURROGATE = 0x10000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_PRIVATE = 0x20000000;
const utf8lex_cat_t UTF8LEX_CAT_MAX = 0x40000000;

//
// Combined categories, OR'ed together base categories e.g. letter
// can be upper, lower or title case, etc.:
//
const utf8lex_cat_t UTF8LEX_GROUP_OTHER =
  UTF8LEX_CAT_OTHER_NA
  | UTF8LEX_CAT_OTHER_CONTROL
  | UTF8LEX_CAT_OTHER_FORMAT
  | UTF8LEX_CAT_OTHER_SURROGATE
  | UTF8LEX_CAT_OTHER_PRIVATE;
const utf8lex_cat_t UTF8LEX_GROUP_LETTER =
  UTF8LEX_CAT_LETTER_UPPER
  | UTF8LEX_CAT_LETTER_LOWER
  | UTF8LEX_CAT_LETTER_TITLE
  | UTF8LEX_CAT_LETTER_MODIFIER
  | UTF8LEX_CAT_LETTER_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_MARK =
  UTF8LEX_CAT_MARK_NON_SPACING
  | UTF8LEX_CAT_MARK_SPACING_COMBINING
  | UTF8LEX_CAT_MARK_ENCLOSING;
const utf8lex_cat_t UTF8LEX_GROUP_NUM =
  UTF8LEX_CAT_NUM_DECIMAL
  | UTF8LEX_CAT_NUM_LETTER
  | UTF8LEX_CAT_NUM_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_PUNCT =
  UTF8LEX_CAT_PUNCT_CONNECTOR
  | UTF8LEX_CAT_PUNCT_DASH
  | UTF8LEX_CAT_PUNCT_OPEN
  | UTF8LEX_CAT_PUNCT_CLOSE
  | UTF8LEX_CAT_PUNCT_QUOTE_OPEN
  | UTF8LEX_CAT_PUNCT_QUOTE_CLOSE
  | UTF8LEX_CAT_PUNCT_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_SYM =
  UTF8LEX_CAT_SYM_MATH
  | UTF8LEX_CAT_SYM_CURRENCY
  | UTF8LEX_CAT_SYM_MODIFIER
  | UTF8LEX_CAT_SYM_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_WHITESPACE =
  UTF8LEX_CAT_SEP_SPACE
  | UTF8LEX_CAT_SEP_LINE
  | UTF8LEX_CAT_SEP_PARAGRAPH;


// ---------------------------------------------------------------------
//                           utf8lex_string_t
// ---------------------------------------------------------------------
utf8lex_error_t utf8lex_string_init(
        utf8lex_string_t *self,
        size_t max_length_bytes,  // How many bytes have been allocated.
        size_t length_bytes,  // How many bytes have been written.
        unsigned char *bytes
        )
{
  if (self == NULL
      || bytes == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (length_bytes < (size_t) 0
           || max_length_bytes < length_bytes)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  self->max_length_bytes = max_length_bytes;
  self->length_bytes = length_bytes;
  self->bytes = bytes;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_string_clear(
        utf8lex_string_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->max_length_bytes = (size_t) 0;
  self->length_bytes = (size_t) 0;
  self->bytes = NULL;

  return UTF8LEX_OK;
}


// Print state (location) to the specified string:
utf8lex_error_t utf8lex_state_string(
        utf8lex_string_t *str,
        utf8lex_state_t *state
        )
{
  if (str == NULL
      || str->bytes == NULL
      || state == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  size_t num_bytes_written = snprintf(
      str->bytes,
      str->max_length_bytes,
      "(bytes@%d[%d], chars@%d[%d], graphemes@%d[%d], words@%d[%d], lines@%d[%d], paragraphs@%d[%d], sections@%d[%d])",
      state->loc[UTF8LEX_UNIT_BYTE].start,
      state->loc[UTF8LEX_UNIT_BYTE].length,
      state->loc[UTF8LEX_UNIT_CHAR].start,
      state->loc[UTF8LEX_UNIT_CHAR].length,
      state->loc[UTF8LEX_UNIT_GRAPHEME].start,
      state->loc[UTF8LEX_UNIT_GRAPHEME].length,
      state->loc[UTF8LEX_UNIT_WORD].start,
      state->loc[UTF8LEX_UNIT_WORD].length,
      state->loc[UTF8LEX_UNIT_LINE].start,
      state->loc[UTF8LEX_UNIT_LINE].length,
      state->loc[UTF8LEX_UNIT_PARAGRAPH].start,
      state->loc[UTF8LEX_UNIT_PARAGRAPH].length,
      state->loc[UTF8LEX_UNIT_SECTION].start,
      state->loc[UTF8LEX_UNIT_SECTION].length);

  if (num_bytes_written >= str->max_length_bytes)
  {
    // The error string was truncated.
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  return UTF8LEX_OK;
}

// Print error message to the specified string:
utf8lex_error_t utf8lex_error_string(
        utf8lex_string_t *str,
        utf8lex_error_t error
        )
{
  if (str == NULL
      || str->bytes == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (error < UTF8LEX_OK
           || error > UTF8LEX_ERROR_MAX)
  {
    return UTF8LEX_ERROR_BAD_ERROR;
  }

  size_t num_bytes_written;
  switch (error)
  {
  case UTF8LEX_OK:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_OK");
    break;

  case UTF8LEX_MORE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_MORE");
    break;
  case UTF8LEX_NO_MATCH:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_NO_MATCH");
    break;

  case UTF8LEX_ERROR_NULL_POINTER:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_NULL_POINTER");
    break;
  case UTF8LEX_ERROR_CHAIN_INSERT:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_CHAIN_INSERT");
    break;
  case UTF8LEX_ERROR_CAT:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_CAT");
    break;
  case UTF8LEX_ERROR_PATTERN_TYPE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_PATTERN_TYPE");
    break;
  case UTF8LEX_ERROR_INFINITE_LOOP:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_INFINITE_LOOP");
    break;
  case UTF8LEX_ERROR_REGEX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_REGEX");
    break;
  case UTF8LEX_ERROR_BAD_LENGTH:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_LENGTH");
    break;
  case UTF8LEX_ERROR_BAD_OFFSET:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_OFFSET");
    break;
  case UTF8LEX_ERROR_BAD_START:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_START");
    break;
  case UTF8LEX_ERROR_BAD_REGEX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_REGEX");
    break;
  case UTF8LEX_ERROR_BAD_ERROR:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_ERROR");
    break;

  case UTF8LEX_ERROR_MAX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_MAX");
    break;

  default:
    return UTF8LEX_ERROR_BAD_ERROR;
  }

  if (num_bytes_written >= str->max_length_bytes)
  {
    // The error string was truncated.
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                          utf8lex_category_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_category_init(
        utf8lex_category_t *self,
        utf8lex_category_t *prev,
        utf8lex_cat_t cat,
        unsigned char *name
        )
{
  if (self == NULL
      || name == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }
  else if (cat <= UTF8LEX_CAT_NONE
           || cat >= UTF8LEX_CAT_MAX)
  {
    return UTF8LEX_ERROR_CAT;
  }

  self->next = NULL;
  self->prev = prev;
  self->cat = cat;
  self->name = name;

  if (self->prev != NULL)
  {
    self->prev->next = self;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_category_clear(
        utf8lex_category_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self->next;
  }

  if (self->next != NULL)
  {
    self->next->prev = self->prev;
  }

  self->next = NULL;
  self->prev = NULL;
  self->cat = UTF8LEX_CAT_NONE;
  self->name = NULL;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_find_category(
        utf8lex_category_t *category_chain,
        utf8lex_cat_t cat,
        utf8lex_category_t **found
        )
{
  if (category_chain == NULL
      || found == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (cat <= UTF8LEX_CAT_NONE
           || cat >= UTF8LEX_CAT_MAX)
  {
    return UTF8LEX_ERROR_CAT;
  }

  utf8lex_category_t *category = category_chain;
  int infinite_loop = (int) UTF8LEX_CAT_MAX;
  for (int c = 0; c < infinite_loop; c ++)
  {
    if (category->cat == cat)
    {
      *found = category;
      return UTF8LEX_OK;
    }

    category = category->next;
    if (category == NULL)
    {
      break;
    }
  }

  if (category != NULL)
  {
    *found = NULL;
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  *found = NULL;
  return UTF8LEX_ERROR_CAT;
}

// ---------------------------------------------------------------------
//                       utf8lex_class_pattern_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_class_pattern_init(
        utf8lex_class_pattern_t *self,
        utf8lex_cat_t cat,  // The category, such as UTF8LEX_GROUP_LETTER.
        int min,  // Minimum consecutive occurrences of the class (1 or more).
        int max  // Maximum consecutive occurrences (-1 = no limit).
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (cat <= UTF8LEX_CAT_NONE
           || cat >= UTF8LEX_CAT_MAX)
  {
    return UTF8LEX_ERROR_CAT;
  }
  else if (min <= 0)
  {
    return UTF8LEX_ERROR_BAD_MIN;
  }
  else if (max != -1
           && max < min)
  {
    return UTF8LEX_ERROR_BAD_MAX;
  }

  self->cat = cat;
  self->min = min;
  self->max = max;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_class_pattern_clear(
        utf8lex_class_pattern_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->cat = UTF8LEX_CAT_NONE;
  self->min = 0;
  self->max = 0;

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                      utf8lex_literal_pattern_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_literal_pattern_init(
        utf8lex_literal_pattern_t *self,
        unsigned char *str
        )
{
  if (self == NULL
      || str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  size_t num_bytes = strlen(str);
  if (num_bytes == (size_t) 0)
  {
    return UTF8LEX_ERROR_EMPTY_LITERAL;
  }

  self->str = str;
  self->length[UTF8LEX_UNIT_BYTE] = (int) num_bytes;

  // We know how many bytes the ltieral is.
  // Now we'll now use utf8proc to count how many characters,
  // graphemes, lines, etc. are in the matching region.

  utf8lex_string_t utf8lex_str;
  utf8lex_string_init(&utf8lex_str,  // self
                      num_bytes,     // max_length_bytes
                      num_bytes,     // length_bytes
                      self->str);    // bytes
  utf8lex_buffer_t buffer;
  utf8lex_buffer_init(&buffer,       // self
                      NULL,          // prev
                      &utf8lex_str,  // str
                      true);         // is_eof
  utf8lex_state_t state;
  utf8lex_state_init(&state,         // self
                     &buffer);       // buffer
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state.loc[unit].start = 0;
    state.loc[unit].length = 0;
  }

  off_t offset = (off_t) 0;
  size_t length[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    length[unit] = (size_t) 0;
  }

  // Keep reading graphemes until we've reached the end of the literal string:
  for (int ug = 0;
       length[UTF8LEX_UNIT_BYTE] < self->length[UTF8LEX_UNIT_BYTE];
       ug ++)
  {
    // Read in one UTF-8 grapheme cluster per loop iteration:
    size_t num_bytes_read = (size_t) 0;
    size_t num_chars_read = (size_t) 0;
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        &state,  // state, including absolute locations.
        &offset,  // start byte, relative to start of the buffer string.
        length,  // size_t[] # bytes, chars, etc read.
        &codepoint,  // codepoint
        &cat  //cat
        );

    if (error != UTF8LEX_OK)
    {
      // The literal is a string that utf8proc either needs MORE bytes for,
      // or rejected outright.  We can't accept it.
      return error;
    }

    // We found another grapheme inside the literal string.
    // Keep looking for more graphemes inside the literal string.
  }

  // Check to make sure strlen and utf8proc agree on # bytes.  (They should.)
  if (length[UTF8LEX_UNIT_BYTE] != self->length[UTF8LEX_UNIT_BYTE])
  {
    printf("!!! strlen and utf8proc disagree: strlen = %d vs utf8proc = %d\n",
           self->length[UTF8LEX_UNIT_BYTE],
           length[UTF8LEX_UNIT_BYTE]);
  }

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->length[unit] = (int) length[unit];
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_literal_pattern_clear(
        utf8lex_literal_pattern_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->str = NULL;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->length[unit] = -1;
  }

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                       utf8lex_regex_pattern_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_regex_pattern_init(
        utf8lex_regex_pattern_t *self,
        unsigned char *pattern
        )
{
  if (self == NULL
      || pattern == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Compile the regex_pattern string into a pcre2code
  // regular expression:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#SEC20
  int pcre2_error;  // We don't actually use this for now.
  PCRE2_SIZE pcre2_offset;  // We don't actually use this for now.
  self->regex = pcre2_compile(
                              pattern,  // the regular expression pattern
                              PCRE2_ZERO_TERMINATED,  // '\0' at end.
                              0,  // default options.
                              &pcre2_error,  // for error code.
                              &pcre2_offset,  // for error offset.
                              NULL);  // no compile context.
  if (pcre2_error < 0)
  {
    PCRE2_UCHAR pcre2_error_string[256];
    pcre2_get_error_message(pcre2_error,
                            pcre2_error_string,
                            (PCRE2_SIZE) 256);
    printf("!!! pcre2 error: %s\n", pcre2_error_string);
    utf8lex_regex_pattern_clear(self);

    return UTF8LEX_ERROR_BAD_REGEX;
  }

  self->pattern = pattern;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_regex_pattern_clear(
        utf8lex_regex_pattern_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (self->regex != NULL)
  {
    pcre2_code_free(self->regex);
  }

  self->pattern = NULL;
  self->regex = NULL;

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                        utf8lex_token_type_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_token_type_init(
        utf8lex_token_type_t *self,
        utf8lex_token_type_t *prev,
        unsigned char *name,
        utf8lex_pattern_type_t pattern_type,
        utf8lex_class_pattern_t *class_pattern,
        utf8lex_literal_pattern_t *literal_pattern,
        utf8lex_regex_pattern_t *regex_pattern,
        unsigned char *code
        )
{
  if (self == NULL
      || name == NULL
      || code == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }
  else if (pattern_type <= UTF8LEX_PATTERN_TYPE_NONE
           || pattern_type >= UTF8LEX_PATTERN_TYPE_MAX)
  {
    return UTF8LEX_ERROR_PATTERN_TYPE;
  }

  switch (pattern_type)
  {
  case UTF8LEX_PATTERN_TYPE_CLASS:
    if (class_pattern == NULL)
    {
      return UTF8LEX_ERROR_NULL_POINTER;
    }
    self->pattern.cls = class_pattern;
    break;

  case UTF8LEX_PATTERN_TYPE_LITERAL:
    if (literal_pattern == NULL)
    {
      return UTF8LEX_ERROR_NULL_POINTER;
    }
    self->pattern.literal = literal_pattern;
    break;

  case UTF8LEX_PATTERN_TYPE_REGEX:
    if (regex_pattern == NULL)
    {
      return UTF8LEX_ERROR_NULL_POINTER;
    }
    self->pattern.regex = regex_pattern;
    break;

  default:
    return UTF8LEX_ERROR_PATTERN_TYPE;
  }

  self->next = NULL;
  self->prev = prev;
  // id is set below, in the if/else statements.
  self->name = name;
  self->pattern_type = pattern_type;
  self->code = code;
  if (self->prev != NULL)
  {
    self->id = self->prev->id + 1;
    self->prev->next = self;
  }
  else
  {
    self->id = (uint32_t) 0;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_token_type_clear(
        utf8lex_token_type_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self->next;
  }

  if (self->next != NULL)
  {
    self->next->prev = self->prev;
  }

  if (self->pattern_type == UTF8LEX_PATTERN_TYPE_CLASS
      && self->pattern.cls != NULL)
  {
    utf8lex_class_pattern_clear(self->pattern.cls);
  }
  else if (self->pattern_type == UTF8LEX_PATTERN_TYPE_REGEX
      && self->pattern.regex != NULL)
  {
    utf8lex_regex_pattern_clear(self->pattern.regex);
  }

  self->next = NULL;
  self->prev = NULL;
  self->id = (uint32_t) 0;
  self->name = NULL;
  self->pattern_type = UTF8LEX_PATTERN_TYPE_NONE;
  self->pattern.cls = NULL;
  self->pattern.literal = NULL;
  self->pattern.regex = NULL;
  self->code = NULL;

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                          utf8lex_location_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_location_init(
        utf8lex_location_t *self,
        int start,
        int length
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (start < 0)
  {
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length < 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  self->start = start;
  self->length = length;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_location_clear(
        utf8lex_location_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->start = -1;
  self->length = -1;

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                           utf8lex_token_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_token_init(
        utf8lex_token_t *self,
        utf8lex_token_type_t *token_type,
        utf8lex_state_t *state  // For buffer and absolute location.
        )
{
  if (self == NULL
      || token_type == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Check valid buffer-relative starts, lengths:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (state->buffer->loc[unit].start < 0)
    {
      return UTF8LEX_ERROR_BAD_START;
    }
    else if (state->buffer->loc[unit].length < 0)
    {
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
  }

  // Check valid bytes range within buffer:
  int start_byte = state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  int length_bytes = state->buffer->loc[UTF8LEX_UNIT_BYTE].length;
  if ((size_t) start_byte >= state->buffer->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length_bytes < 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  else if ((start_byte + length_bytes) > state->buffer->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  // Check valid absolute starts, lengths:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (state->loc[unit].start < 0)
    {
      return UTF8LEX_ERROR_BAD_START;
    }
    else if (state->loc[unit].length < 0)
    {
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
  }

  self->token_type = token_type;
  self->start_byte = start_byte;  // Bytes offset into str where token starts.
  self->length_bytes = length_bytes;  // # bytes in token.
  self->str = state->buffer->str;  // The buffer's string.
  // Absolute locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    // Absolute location from the absolute state:
    self->loc[unit].start = state->loc[unit].start;
    // Absolute length from the length of the token within the buffer:
    self->loc[unit].length = state->buffer->loc[unit].length;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_token_clear(
        utf8lex_token_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->token_type = NULL;
  self->start_byte = -1;
  self->length_bytes = -1;
  self->str = NULL;

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  return UTF8LEX_OK;
}


// Returns UTF8LEX_MORE if the destination string truncates the token:
extern utf8lex_error_t utf8lex_token_copy_string(
        utf8lex_token_t *self,
        unsigned char *str,
        size_t max_bytes)
{
  if (self == NULL
      || str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  int start_byte = self->start_byte;
  int length_bytes = self->length_bytes;
  if (start_byte < 0)
  {
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length_bytes < 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  else if ((size_t) (start_byte + length_bytes) > self->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  size_t num_bytes = length_bytes;
  if ((num_bytes + 1) > max_bytes)
  {
    num_bytes = max_bytes - 1;
  }

  void *source = &self->str->bytes[start_byte];
  memcpy(str, source, num_bytes);
  str[num_bytes] = 0;

  if (num_bytes != length_bytes)
  {
    return UTF8LEX_MORE;
  }

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                           utf8lex_buffer_t
// ---------------------------------------------------------------------

const uint32_t UTF8LEX_BUFFER_STRINGS_MAX = 16384;

utf8lex_error_t utf8lex_buffer_init(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *prev,
        utf8lex_string_t *str,
        bool is_eof
        )
{
  if (self == NULL
      || str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->next = NULL;
  self->prev = prev;
  self->str = str;
  self->is_eof = is_eof;

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = 0;
    self->loc[unit].length = 0;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_buffer_clear(
        utf8lex_buffer_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self->next;
  }

  if (self->next != NULL)
  {
    self->next->prev = self->prev;
  }

  self->prev = NULL;
  self->next = NULL;
  self->str = NULL;


  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_buffer_add(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *tail
        )
{
  if (self == NULL
      || tail == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (tail->prev != NULL
           || tail->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  // Find the end of the buffer, in case this (self) buffer is not at the end:
  utf8lex_buffer_t *buffer = self;
  int infinite_loop = (int) UTF8LEX_BUFFER_STRINGS_MAX;
  for (int b = 0; b < infinite_loop; b ++)
  {
    if (buffer->next == NULL)
    {
      buffer->next = tail;
      tail->prev = buffer;
      break;
    }

    buffer = buffer->next;
  }

  if (tail->prev != buffer)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                            ut8lex_state_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_state_init(
        utf8lex_state_t *self,
        utf8lex_buffer_t *buffer
        )
{
  if (self == NULL
      || buffer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->buffer = buffer;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_state_clear(
        utf8lex_state_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->buffer = NULL;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                            utf8lex_lex()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_cat_from_utf8proc_category(
        utf8proc_category_t utf8proc_cat,
        utf8lex_cat_t *cat
        )
{
  utf8lex_error_t error = UTF8LEX_NO_MATCH;
  switch (utf8proc_cat)
  {
  case UTF8PROC_CATEGORY_CN:
    *cat = UTF8LEX_CAT_OTHER_NA;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LU:
    *cat = UTF8LEX_CAT_LETTER_UPPER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LL:
    *cat = UTF8LEX_CAT_LETTER_LOWER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LT:
    *cat = UTF8LEX_CAT_LETTER_TITLE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LM:
    *cat = UTF8LEX_CAT_LETTER_MODIFIER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LO:
    *cat = UTF8LEX_CAT_LETTER_OTHER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_MN:
    *cat = UTF8LEX_CAT_MARK_NON_SPACING;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_MC:
    *cat = UTF8LEX_CAT_MARK_SPACING_COMBINING;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ME:
    *cat = UTF8LEX_CAT_MARK_ENCLOSING;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ND:
    *cat = UTF8LEX_CAT_NUM_DECIMAL;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_NL:
    *cat = UTF8LEX_CAT_NUM_LETTER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_NO:
    *cat = UTF8LEX_CAT_NUM_OTHER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PC:
    *cat = UTF8LEX_CAT_PUNCT_CONNECTOR;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PD:
    *cat = UTF8LEX_CAT_PUNCT_DASH;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PS:
    *cat = UTF8LEX_CAT_PUNCT_OPEN;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PE:
    *cat = UTF8LEX_CAT_PUNCT_CLOSE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PI:
    *cat = UTF8LEX_CAT_PUNCT_QUOTE_OPEN;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PF:
    *cat = UTF8LEX_CAT_PUNCT_QUOTE_CLOSE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PO:
    *cat = UTF8LEX_CAT_PUNCT_OTHER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_SM:
    *cat = UTF8LEX_CAT_SYM_MATH;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_SC:
    *cat = UTF8LEX_CAT_SYM_CURRENCY;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_SK:
    *cat = UTF8LEX_CAT_SYM_MODIFIER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_SO:
    *cat = UTF8LEX_CAT_SYM_OTHER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ZS:
    *cat = UTF8LEX_CAT_SEP_SPACE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ZL:
    *cat = UTF8LEX_CAT_SEP_LINE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ZP:
    *cat = UTF8LEX_CAT_SEP_PARAGRAPH;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_CC:
    *cat = UTF8LEX_CAT_OTHER_CONTROL;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_CF:
    *cat = UTF8LEX_CAT_OTHER_FORMAT;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_CS:
    *cat = UTF8LEX_CAT_OTHER_SURROGATE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_CO:
    *cat = UTF8LEX_CAT_OTHER_PRIVATE;
    error = UTF8LEX_OK;
    break;

  default:
    error = UTF8LEX_ERROR_CAT;  // No idea what category this is!
    *cat = UTF8LEX_CAT_NONE;
    break;
  }

  return error;
}

// Reads to the end of a grapheme, sets the first codepoint of the
// grapheme, and the number of bytes read.
// The state is used only for the string buffer to read from,
// not for its location info.
// The offset, lengths, codepoint and cat are all updated upon
// successfully reading one complete grapheme cluster.
utf8lex_error_t utf8lex_read_grapheme(
        utf8lex_state_t *state,
        off_t *offset,  // Mutable.
        size_t length[UTF8LEX_UNIT_MAX],  // Mutable.
        int32_t *codepoint,  // Mutable.
        utf8lex_cat_t *cat  // Mutable.
        )
{
  if (state == NULL
      || offset == NULL
      || length == NULL
      || codepoint == NULL
      || cat == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (*offset < (off_t) 0
           || *offset >= state->buffer->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_START;
  }

  utf8lex_error_t error = UTF8LEX_ERROR_BAD_ERROR;
  off_t curr_offset = *offset;
  utf8proc_int32_t first_codepoint = (utf8proc_int32_t) -1;
  utf8proc_int32_t prev_codepoint = (utf8proc_int32_t) -1;
  utf8proc_int32_t utf8proc_state = (utf8proc_int32_t) 0;;
  size_t total_bytes_read = (size_t) 0;
  size_t total_chars_read = (size_t) 0;
  for (int u8c = 0; ; u8c ++)
  {
    unsigned char *str_pointer = (unsigned char *)
      (state->buffer->str->bytes + curr_offset);
    size_t max_bytes = (size_t)
      (state->buffer->str->length_bytes - curr_offset);

    if (max_bytes <= (size_t) 0)
    {
      if (state->buffer->next != NULL)
      {
        // Continue reading from the next buffer in the chain.
        state->buffer = state->buffer->next;
        curr_offset = (off_t) 0;
        u8c --;
        continue;
      }
      else if (state->buffer->is_eof == true)
      {
        // No more bytes can be read in, we're at EOF.
        if (u8c == 0)
        {
          // We haven't read any bytes so far.  How did we get here...?
          error = UTF8LEX_NO_MATCH;  // Should we also return ..._EOF from here?
          break;
        }
        else
        {
          // Finished reading a grapheme with at least 1 valid codepoint.  Done.
          error = UTF8LEX_OK;
          break;
        }
      }
      else
      {
        // Ask for more bytes to be read in, so that we can find
        // a full grapheme, even if it straddles buffer boundaries.
        error = UTF8LEX_MORE;
      }
      break;
    }

    utf8proc_int32_t utf8proc_codepoint;
    utf8proc_ssize_t utf8proc_num_bytes_read = utf8proc_iterate(
        (utf8proc_uint8_t *) str_pointer,
        (utf8proc_ssize_t) max_bytes,
        &utf8proc_codepoint);

    if (utf8proc_num_bytes_read == UTF8PROC_ERROR_INVALIDUTF8
        && (state->buffer->str->length_bytes - (size_t) curr_offset)
           < UTF8LEX_MAX_BYTES_PER_CHAR)
    {
      if (state->buffer->is_eof == true)
      {
        // No more bytes can be read in, we're at EOF.
        // Bad UTF-8 character at the end of the buffer.
        error = UTF8LEX_ERROR_BAD_UTF8;
      }
      else
      {
        // Need to read more bytes for the full grapheme.
        error = UTF8LEX_MORE;
      }
      break;
    }
    else if (utf8proc_num_bytes_read <= (utf8proc_ssize_t) 0)
    {
      // Possible errors:
      //     - UTF8PROC_ERROR_NOMEM
      //       Memory could not be allocated.
      //     - UTF8PROC_ERROR_OVERFLOW
      //       The given string is too long to be processed.
      //     - UTF8PROC_ERROR_INVALIDUTF8
      //       The given string is not a legal UTF-8 string.
      //     - UTF8PROC_ERROR_NOTASSIGNED
      //       The UTF8PROC_REJECTNA flag was set and an unassigned codepoint
      //       was found.
      //     - UTF8PROC_ERROR_INVALIDOPTS
      //       Invalid options have been used.
      if (u8c == 0)
      {
        error = UTF8LEX_ERROR_BAD_UTF8;
        break;
      }
      else
      {
        // Finished reading at least 1 codepoint.  Done.
        // We'll return to this bad character the next time we lex.
        error = UTF8LEX_OK;
        break;
      }
    }

    if (u8c == 0)  // First UTF-8 character we've read in the grapheme.
    {
      first_codepoint = utf8proc_codepoint;
    }
    else  // Not the first UTF-8 character we've read in the grapheme.
    {
      // Check for grapheme break.  Keep reading until we find one.
      utf8proc_bool is_grapheme_break = utf8proc_grapheme_break_stateful(
          prev_codepoint,
          utf8proc_codepoint,
          &utf8proc_state);
      if (is_grapheme_break == (utf8proc_bool) true)
      {
        error = UTF8LEX_OK;
        break;
      }
    }

    curr_offset += (off_t) utf8proc_num_bytes_read;
    total_bytes_read += (size_t) utf8proc_num_bytes_read;
    total_chars_read += (size_t) 1;

    // Keep reading more bytes until we find a grapheme boundary
    // (or run out of bytes).
  }

  if (error != UTF8LEX_OK)
  {
    // Could be bad UTF-8, or need more bytes, etc.
    return error;
  }

  // We read in a full grapheme cluster.
  // Figure out the utf8lex_cat_t equivalent of the utf8proc_category_t:
  utf8proc_category_t utf8proc_cat = utf8proc_category(first_codepoint);
  utf8lex_cat_t utf8lex_cat = UTF8LEX_CAT_NONE;
  error = utf8lex_cat_from_utf8proc_category(utf8proc_cat, &utf8lex_cat);
  if (error != UTF8LEX_OK)
  {
    // Couldn't figure out the first UTF-8 character's category.
    // Maybe a utf8lex bug?
    return error;
  }

  // Success.
  *offset += (off_t) total_bytes_read;
  length[UTF8LEX_UNIT_BYTE] += (size_t) total_bytes_read;
  length[UTF8LEX_UNIT_CHAR] += (size_t) total_chars_read;
  length[UTF8LEX_UNIT_GRAPHEME] += (size_t) 1;
  *codepoint = first_codepoint;
  *cat = utf8lex_cat;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_lex_class(
        utf8lex_token_type_t *token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token
        )
{
  if (token_type == NULL
      || token_type->pattern.cls == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (token_type->pattern_type != UTF8LEX_PATTERN_TYPE_CLASS)
  {
    return UTF8LEX_ERROR_PATTERN_TYPE;
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  size_t length[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    length[unit] = (size_t) 0;
  }

  for (int ug = 0;
       token_type->pattern.cls->max == -1 || ug < token_type->pattern.cls->max;
       ug ++)
  {
    // Read in one UTF-8 grapheme cluster:
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        state,  // state, including absolute locations.
        &offset,  // start byte, relative to start of the buffer string.
        length,  // size_t[] # bytes, chars, etc read.
        &codepoint,  // codepoint
        &cat  //cat
        );

    if (error == UTF8LEX_MORE)
    {
      return error;
    }
    else if (error != UTF8LEX_OK)
    {
      if (ug < token_type->pattern.cls->min)
      {
        return error;
      }
      else
      {
        // Finished reading at least (min) graphemes.  Done.
        // We'll return to this bad UTF-8 grapheme the next time we lex.
        break;
      }
    }

    if (token_type->pattern.cls->cat & cat)
    {
      // A/the category we're looking for.
      error = UTF8LEX_OK;
    }
    else if (ug < token_type->pattern.cls->min)
    {
      // Not the category we're looking for, and we haven't found
      // at least (min) graphemes matching this category, so fail
      // with no match.
      return UTF8LEX_NO_MATCH;
    }
    else
    {
      // Not a/the category we're looking for, but we already
      // finished reading at least (min) graphemes, so we're done.
      break;
    }

    // We found another grapheme of the expected class.
    // Keep looking for more matches for this token,
    // until we hit the max.
  }

  // Found what we're looking for.
  // Update buffer locations amd the absolute locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state->buffer->loc[unit].length = (int) length[unit];
    state->loc[unit].length = (int) length[unit];
  }

  utf8lex_error_t error = utf8lex_token_init(
      token,
      token_type,
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_lex_literal(
        utf8lex_token_type_t *token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token
        )
{
  if (token_type == NULL
      || token_type->pattern.literal == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (token_type->pattern_type != UTF8LEX_PATTERN_TYPE_LITERAL)
  {
    return UTF8LEX_ERROR_PATTERN_TYPE;
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  size_t remaining_bytes = state->buffer->str->length_bytes - (size_t) offset;

  utf8lex_literal_pattern_t *literal = token_type->pattern.literal;
  size_t token_length_bytes = literal->length[UTF8LEX_UNIT_BYTE];

  for (off_t c = (off_t) 0;
       (size_t) c < remaining_bytes && (size_t) c < token_length_bytes;
       c ++)
  {
    if (state->buffer->str->bytes[offset + c] != literal->str[c])
    {
      return UTF8LEX_NO_MATCH;
    }
  }

  if (remaining_bytes < token_length_bytes)
  {
    // Not enough bytes to read the string.
    // (It was matching for maybe a few bytes, anyway.)
    if (state->buffer->is_eof)
    {
      // No more bytes can be read in, we're at EOF.
      return UTF8LEX_NO_MATCH;
    }
    else
    {
      // Need to read more bytes for the full grapheme.
      return UTF8LEX_MORE;
    }
  }

  // Matched the literal exactly.

  // Update buffer locations and absolute locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state->buffer->loc[unit].length = literal->length[unit];
    state->loc[unit].length = literal->length[unit];
  }

  utf8lex_error_t error = utf8lex_token_init(
      token,
      token_type,
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_lex_regex(
        utf8lex_token_type_t *token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token
        )
{
  if (token_type == NULL
      || token_type->pattern.regex == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (token_type->pattern_type != UTF8LEX_PATTERN_TYPE_REGEX)
  {
    return UTF8LEX_ERROR_PATTERN_TYPE;
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  size_t remaining_bytes = state->buffer->str->length_bytes - (size_t) offset;

  // For now we use the traditional (Perl-compatible, "NFA") algorithm:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#SEC28
  //
  // For differences between the traditional and "DFA" algorithms, see:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2matching.html
  pcre2_match_data *match = pcre2_match_data_create(
      1,  // ovecsize.  We only care about the whole match, not sub-groups.
      NULL);  // gcontext.
  if (match == NULL)
  {
    return UTF8LEX_ERROR_REGEX;
  }

  // Match options:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#matchoptions
  //     PCRE2_ANCHORED
  //     The PCRE2_ANCHORED option limits pcre2_match() to matching
  //     at the first matching position. If a pattern was compiled
  //     with PCRE2_ANCHORED, or turned out to be anchored by virtue
  //     of its contents, it cannot be made unachored at matching time.
  //     Note that setting the option at match time disables JIT matching.
  //     PCRE2_NOTBOL
  //     This option specifies that first character of the subject string
  //     is not the beginning of a line, so the circumflex metacharacter
  //     should not match before it. Setting this without having
  //     set PCRE2_MULTILINE at compile time causes circumflex never to match.
  //     This option affects only the behaviour of the circumflex
  //     metacharacter. It does not affect \A.
  //     PCRE2_NOTEOL
  //     This option specifies that the end of the subject string
  //     is not the end of a line, so the dollar metacharacter
  //     should not match it nor (except in multiline mode) a newline
  //     immediately before it. Setting this without having
  //     set PCRE2_MULTILINE at compile time causes dollar never to match.
  //     This option affects only the behaviour of the dollar metacharacter.
  //     It does not affect \Z or \z.
  //     PCRE2_NOTEMPTY
  //     An empty string is not considered to be a valid match if this option
  //     is set. If there are alternatives in the pattern, they are tried.
  //     If all the alternatives match the empty string, the entire match
  //     fails. For example, if the pattern
  //         a?b?
  //     is applied to a string not beginning with "a" or "b", it matches
  //     an empty string at the start of the subject. With PCRE2_NOTEMPTY set,
  //     this match is not valid, so pcre2_match() searches further
  //     into the string for occurrences of "a" or "b". 
  int pcre2_error = pcre2_match(
      token_type->pattern.regex->regex,  // The pcre2_code (compiled regex).
      (PCRE2_SPTR) state->buffer->str->bytes,  // subject
      (PCRE2_SIZE) state->buffer->str->length_bytes,  // length
      (PCRE2_SIZE) offset,  // startoffset
      (uint32_t) PCRE2_ANCHORED,  // options (see above)
      match,  // match_data
      (pcre2_match_context *) NULL);  // NULL means use defaults.

  if (pcre2_error == PCRE2_ERROR_NOMATCH)
  {
    pcre2_match_data_free(match);
    return UTF8LEX_NO_MATCH;
  }
  else if (pcre2_error == PCRE2_ERROR_PARTIAL)
  {
    pcre2_match_data_free(match);
    if (state->buffer->is_eof)
    {
      // No more bytes can be read in, we're at EOF.
      // Bad UTF-8 character at the end of the buffer.
      return UTF8LEX_ERROR_BAD_UTF8;
    }
    else
    {
      // Need to read more bytes for the full grapheme.
      return UTF8LEX_MORE;
    }
  }
  // Negative number indicates error:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#SEC32
  else if (pcre2_error < 0)
  {
    pcre2_match_data_free(match);
    return UTF8LEX_ERROR_REGEX;
  }

  uint32_t num_ovectors = pcre2_get_ovector_count(match);
  if (num_ovectors == (uint32_t) 0)
  {
    pcre2_match_data_free(match);
    return UTF8LEX_ERROR_REGEX;
  }

  // Extract the whole match from the ovectors:
  // pcre2_match_data *match_data);
  PCRE2_SIZE pcre2_match_length_bytes;
  pcre2_substring_length_bynumber(
      match, // match_data
      (uint32_t) 0,  // number
      &pcre2_match_length_bytes);
  size_t match_length_bytes = (size_t) pcre2_match_length_bytes;

  pcre2_match_data_free(match);

  if (match_length_bytes == (size_t) 0)
  {
    return UTF8LEX_NO_MATCH;
  }

  PCRE2_UCHAR match_substring[256];
  PCRE2_SIZE match_substring_length = 256;
  pcre2_substring_copy_bynumber(match, (uint32_t) 0, match_substring, &match_substring_length);

  // Matched.
  //
  // We know how many bytes matched the regular expression.
  // Now we'll now use utf8proc to count how many characters,
  // graphemes, lines, etc. are in the matching region.
  size_t length[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    length[unit] = (size_t) 0;
  }

  // Keep reading graphemes until we've reached the end of the regex match:
  for (int ug = 0;
       length[UTF8LEX_UNIT_BYTE] < match_length_bytes;
       ug ++)
  {
    // Read in one UTF-8 grapheme cluster per loop iteration:
    size_t num_bytes_read = (size_t) 0;
    size_t num_chars_read = (size_t) 0;
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        state,  // state, including absolute locations.
        &offset,  // start byte, relative to start of the buffer string.
        length,  // size_t[] # bytes, chars, etc read.
        &codepoint,  // codepoint
        &cat  //cat
        );

    if (error != UTF8LEX_OK)
    {
      // pcre2 matched something that utf8proc either needs MORE bytes for,
      // or rejected outright.
      return error;
    }

    // We found another grapheme inside the regex match.
    // Keep looking for more graphemes inside the regex match.
  }

  // Check to make sure pcre2 and utf8proc agree on # bytes.  (They should.)
  if (length[UTF8LEX_UNIT_BYTE] != match_length_bytes)
  {
    printf("!!! pcre2 and utf8proc disagree: pcre2 match_length_bytes = %d vs utf8proc = %d\n",
           match_length_bytes,
           length[UTF8LEX_UNIT_BYTE]);
  }

  // Update buffer locations amd the absolute locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state->buffer->loc[unit].length = (int) length[unit];
    state->loc[unit].length = (int) length[unit];
  }

  utf8lex_error_t error = utf8lex_token_init(
      token,
      token_type,
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_lex(
        utf8lex_token_type_t *first_token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token
        )
{
  if (first_token_type == NULL
      || state == NULL
      || token == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (state->loc[UTF8LEX_UNIT_BYTE].start < 0)
  {
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      state->loc[unit].start = 0;
      state->loc[unit].length = 0;
    }
  }
  // EOF check:
  else if (state->buffer->loc[UTF8LEX_UNIT_BYTE].start
           >= state->buffer->str->length_bytes)
  {
    // We've lexed to the end of the buffer.
    if (state->buffer->next == NULL)
    {
      if (state->buffer->is_eof == true)
      {
        // Done lexing.
        return UTF8LEX_EOF;
      }
      else
      {
        // Please, sir, may I have some more?
        return UTF8LEX_MORE;
      }
    }

    // Move on to the next buffer in the chain.
    state->buffer = state->buffer->next;
  }

  utf8lex_token_type_t *matched = NULL;
  for (utf8lex_token_type_t *token_type = first_token_type;
       token_type != NULL;
       token_type = token_type->next)
  {
    utf8lex_error_t error;
    switch (token_type->pattern_type)
    {
    case UTF8LEX_PATTERN_TYPE_CLASS:
      error = utf8lex_lex_class(token_type,
                                state,
                                token);
      break;

    case UTF8LEX_PATTERN_TYPE_LITERAL:
      error = utf8lex_lex_literal(token_type,
                                 state,
                                 token);
      break;

    case UTF8LEX_PATTERN_TYPE_REGEX:
      error = utf8lex_lex_regex(token_type,
                                state,
                                token);
      break;

    default:
    }

    if (error == UTF8LEX_NO_MATCH)
    {
      // Did not match this one token type.  Carry on with the loop.
      continue;
    }
    else if (error == UTF8LEX_MORE)
    {
      // Need to read more bytes before trying again.
      return error;
    }
    else if (error == UTF8LEX_OK)
    {
      // Matched the token type.  Break out of the loop.
      matched = token_type;
      break;
    }
    else
    {
      // Some other error.  Return the error to the caller.
      return error;
    }
  }

  // If we get this far, we've either 1) matched a token type,
  // or 2) not matched any token type.
  if (matched == NULL)
  {
    return UTF8LEX_NO_MATCH;
  }

  // We have a match.
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    int length_units = token->loc[unit].length;

    // Update buffer locations past end of this token:
    state->buffer->loc[unit].start += length_units;
    state->buffer->loc[unit].length = 0;
    // Update absolute locations past end of this token:
    state->loc[unit].start += length_units;
    state->loc[unit].length = 0;
  }

  return UTF8LEX_OK;
}
