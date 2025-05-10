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

#include <stdio.h>

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                            utf8lex_state_t
// ---------------------------------------------------------------------

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
      "(bytes@%d[%d], chars@%d[%d], graphemes@%d[%d], lines@%d[%d])",
      state->loc[UTF8LEX_UNIT_BYTE].start,
      state->loc[UTF8LEX_UNIT_BYTE].length,
      state->loc[UTF8LEX_UNIT_CHAR].start,
      state->loc[UTF8LEX_UNIT_CHAR].length,
      state->loc[UTF8LEX_UNIT_GRAPHEME].start,
      state->loc[UTF8LEX_UNIT_GRAPHEME].length,
      state->loc[UTF8LEX_UNIT_LINE].start,
      state->loc[UTF8LEX_UNIT_LINE].length);

  if (num_bytes_written >= str->max_length_bytes)
  {
    // The error string was truncated.
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  return UTF8LEX_OK;
}


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
    self->loc[unit].after = -2;
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
    self->loc[unit].after = -2;
  }

  return UTF8LEX_OK;
}
