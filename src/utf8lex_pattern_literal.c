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
#include <inttypes.h>  // For int32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strlen(), memcpy().

#include "utf8lex.h"


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

  self->pattern_type = UTF8LEX_PATTERN_TYPE_LITERAL;
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
    off_t grapheme_offset = offset;
    size_t grapheme_length[UTF8LEX_UNIT_MAX];  // Uninitialized is fine.
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        &state,  // state, including absolute locations.
        &grapheme_offset,  // start byte, relative to start of buffer string.
        grapheme_length,  // size_t[] # bytes, chars, etc read.
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
    offset = grapheme_offset;
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      length[unit] += grapheme_length[unit];
    }
  }

  // Check to make sure strlen and utf8proc agree on # bytes.  (They should.)
  if (length[UTF8LEX_UNIT_BYTE] != self->length[UTF8LEX_UNIT_BYTE])
  {
    fprintf(stderr,
            "*** strlen and utf8proc disagree: strlen = %d vs utf8proc = %d\n",
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
        utf8lex_abstract_pattern_t *self  // Must be utf8lex_literal_pattern_t *
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_literal_pattern_t *literal_pattern =
    (utf8lex_literal_pattern_t *) self;

  literal_pattern->pattern_type = NULL;
  literal_pattern->str = NULL;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    literal_pattern->length[unit] = -1;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_lex_literal(
        utf8lex_token_type_t *token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  if (token_type == NULL
      || token_type->pattern == NULL
      || token_type->pattern->pattern_type == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (token_type->pattern->pattern_type != UTF8LEX_PATTERN_TYPE_LITERAL)
  {
    return UTF8LEX_ERROR_PATTERN_TYPE;
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  size_t remaining_bytes = state->buffer->str->length_bytes - (size_t) offset;

  utf8lex_literal_pattern_t *literal =
    (utf8lex_literal_pattern_t *) token_type->pattern;
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
      token_pointer,
      token_type,
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


// A token pattern that matches a literal string,
// such as "int" or "==" or "proc" and so on:
static utf8lex_pattern_type_t UTF8LEX_PATTERN_TYPE_LITERAL_INTERNAL =
  {
    .name = "LITERAL",
    .lex = utf8lex_lex_literal,
    .clear = utf8lex_literal_pattern_clear
  };
utf8lex_pattern_type_t *UTF8LEX_PATTERN_TYPE_LITERAL =
  &UTF8LEX_PATTERN_TYPE_LITERAL_INTERNAL;
