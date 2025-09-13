/*
 * utf8lex
 * Copyright © 2023-2025 Johann Tienhaara
 * All rights reserved
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <inttypes.h>  // For uint32_t
#include <stdbool.h>  // For bool, true, false.

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
  UTF8LEX_DEBUG("ENTER utf8lex_state_string()");

  if (str == NULL
      || str->bytes == NULL
      || state == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_state_string()");
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
    UTF8LEX_DEBUG("EXIT utf8lex_state_string()");
    return UTF8LEX_MORE;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_state_string()");
  return UTF8LEX_OK;
}

// Print state location "line-start.char-start" to the specified char array
// (e.g. "1.0" or "17.39" and so on).
utf8lex_error_t utf8lex_state_location_copy_string(
        utf8lex_state_t *state,
        unsigned char *str,
        size_t max_bytes)
{
  UTF8LEX_DEBUG("ENTER utf8lex_state_location_copy_string()");

  if (state == NULL
      || str == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_state_location_copy_string()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  int line_position = state->loc[UTF8LEX_UNIT_LINE].start;
  if (state->loc[UTF8LEX_UNIT_LINE].after >= 0) {
    line_position = state->loc[UTF8LEX_UNIT_LINE].after;
  }
  // 0-indexed; bump up by 1:
  line_position ++;

  int char_position = state->loc[UTF8LEX_UNIT_CHAR].start;
  if (state->loc[UTF8LEX_UNIT_CHAR].after >= 0) {
    char_position = state->loc[UTF8LEX_UNIT_CHAR].after;
  }

  size_t num_bytes_written = snprintf(
      str,
      max_bytes,
      "%d.%d",
      line_position,
      char_position);

  if (num_bytes_written >= max_bytes)
  {
    // The error string was truncated.
    UTF8LEX_DEBUG("EXIT utf8lex_state_location_copy_string()");
    return UTF8LEX_MORE;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_state_location_copy_string()");
  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_state_init(
        utf8lex_state_t *self,
        utf8lex_settings_t *settings,
        utf8lex_buffer_t *buffer
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_state_init()");

  if (self == NULL
      || buffer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_state_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (settings == NULL)
  {
    utf8lex_settings_init_defaults(&(self->settings));
  }
  else
  {
    utf8lex_settings_copy(
        settings,            // from
        &(self->settings));  // to
  }

  self->num_tracing_indents = (uint32_t) 0;

  self->buffer = buffer;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
    self->loc[unit].after = -2;
  }

  self->num_used_sub_tokens = (uint32_t) 0;
  // We do not initialize each sub-token until we need it.

  UTF8LEX_DEBUG("EXIT utf8lex_state_init()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_state_clear(
        utf8lex_state_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_state_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_state_clear()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = utf8lex_settings_clear(&(self->settings));
  self->num_tracing_indents = (uint32_t) 0;

  self->buffer = NULL;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
    self->loc[unit].after = -2;
  }

  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_state_clear()");
    return error;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_state_clear()");
  return UTF8LEX_OK;
}
