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
#include <inttypes.h>  // For uint32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For memcpy().

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                           utf8lex_token_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_token_init(
        utf8lex_token_t *self,
        utf8lex_rule_t *rule,
        utf8lex_definition_t *definition,
        utf8lex_location_t token_loc[UTF8LEX_UNIT_MAX],  // Resets, lengths.
        utf8lex_state_t *state  // For buffer and absolute location.
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_token_init()");

  if (self == NULL
      || rule == NULL
      || definition == NULL
      || token_loc == NULL
      || state == NULL
      || state->loc == NULL
      || state->buffer == NULL
      || state->buffer->loc == NULL
      || state->buffer->str == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Check valid buffer relative start locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (state->buffer->loc[unit].start < 0)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_token_init()");
      return UTF8LEX_ERROR_BAD_START;
    }
  }

  // Check valid absolute start locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (state->loc[unit].start < 0)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_token_init()");
      return UTF8LEX_ERROR_BAD_START;
    }
  }

  // Check token starts, lengths, afters, valid bytes range within buffer:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    // The start location for the token must be absolute,
    // not relative to the buffer:
    if (token_loc[unit].start != state->loc[unit].start)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_token_init()");
      return UTF8LEX_ERROR_BAD_START;
    }
    else if (token_loc[unit].length < 0)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_token_init()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    else if (token_loc[unit].after < -1)  // -1 (reset) is OK.
    {
      UTF8LEX_DEBUG("EXIT utf8lex_token_init()");
      return UTF8LEX_ERROR_BAD_AFTER;
    }
    // We don't generate UTF8LEX_ERROR_BAD_HASH errors here.
  }
  int start_byte = state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  int length_bytes = token_loc[UTF8LEX_UNIT_BYTE].length;
  if (length_bytes <= 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_init()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  else if ((start_byte + length_bytes) > state->buffer->str->length_bytes)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_init()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  self->rule = rule;
  self->definition = definition;

  self->start_byte = start_byte;  // Bytes offset into str where token starts.
  self->length_bytes = length_bytes;  // # bytes in token.
  self->str = state->buffer->str;  // The buffer's string.
  // Absolute locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    // Absolute location from the absolute state:
    self->loc[unit].start = token_loc[unit].start;
    // Length of the token:
    self->loc[unit].length = token_loc[unit].length;
    // Possible reset (both relative buffer and absolute state)
    // start locations (for chars, graphemes only) after this token:
    self->loc[unit].after = token_loc[unit].after;
    // Hash of the token (unsigned long, can wrap to 0):
    self->loc[unit].hash = token_loc[unit].hash;
  }

  self->sub_tokens = NULL;
  self->parent_or_null = NULL;

  UTF8LEX_DEBUG("EXIT utf8lex_token_init()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_token_copy(
        utf8lex_token_t *from,
        utf8lex_token_t *to,
        utf8lex_state_t *state
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_token_copy()");

  if (from == NULL
      || to == NULL
      || state == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_copy()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = utf8lex_token_init(
          to,  // self
          from->rule,  // rule
          from->definition,  // definition
          from->loc,  // token_loc
          state);  // state
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_copy()");
    return error;
  }

  // Now we need to take ownership of any / all sub-tokens,
  // and add them.
  utf8lex_sub_token_t *sub_token = from->sub_tokens;
  bool is_infinite_loop = true;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < UTF8LEX_SUB_TOKENS_LENGTH_MAX;
       infinite_loop_protector ++)
  {
    if (sub_token == NULL)
    {
      is_infinite_loop = false;
      break;
    }

    if (to->sub_tokens == NULL)
    {
      to->sub_tokens = sub_token;
      from->sub_tokens = NULL;
    }

    sub_token->token.parent_or_null = to;

    sub_token = sub_token->next;
  }

  if (is_infinite_loop)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_copy()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  to->parent_or_null = from->parent_or_null;

  UTF8LEX_DEBUG("EXIT utf8lex_token_copy()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_token_clear(
        utf8lex_token_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_token_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_clear()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->rule = NULL;
  self->definition = NULL;

  self->start_byte = -1;
  self->length_bytes = -1;
  self->str = NULL;

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
    self->loc[unit].after = -2;
  }

  self->sub_tokens = NULL;
  self->parent_or_null = NULL;

  UTF8LEX_DEBUG("EXIT utf8lex_token_clear()");
  return UTF8LEX_OK;
}


// Copies the token text into the specified string, overwriting it.
// Returns UTF8LEX_MORE if the destination string truncates the token:
utf8lex_error_t utf8lex_token_copy_string(
        utf8lex_token_t *self,
        unsigned char *str,
        size_t max_bytes)
{
  UTF8LEX_DEBUG("ENTER utf8lex_token_copy_string()");

  if (self == NULL
      || str == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_copy_string()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  int start_byte = self->start_byte;
  int length_bytes = self->length_bytes;
  if (start_byte < 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_copy_string()");
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length_bytes < 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_copy_string()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  else if ((size_t) (start_byte + length_bytes) > self->str->length_bytes)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_copy_string()");
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
    UTF8LEX_DEBUG("EXIT utf8lex_token_copy_string()");
    return UTF8LEX_MORE;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_token_copy_string()");
  return UTF8LEX_OK;
}

// Concatenates the token text to the end of the specified string.
// Returns UTF8LEX_MORE if the destination string truncates the token:
utf8lex_error_t utf8lex_token_cat_string(
        utf8lex_token_t *self,
        unsigned char *str,  // Text will be concatenated starting at '\0'.
        size_t max_bytes)
{
  UTF8LEX_DEBUG("ENTER utf8lex_token_cat_string()");

  if (self == NULL
      || str == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_cat_string()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  size_t length = strlen(str);
  size_t reduced_max = max_bytes - length;
  if (reduced_max <= (size_t) 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_token_cat_string()");
    return UTF8LEX_MORE;
  }

  unsigned char *str_offset = &(str[length]);

  utf8lex_error_t error = utf8lex_token_copy_string(
                              self,  // self
                              str_offset,  // str
                              reduced_max);  // max_bytes

  UTF8LEX_DEBUG("EXIT utf8lex_token_cat_string()");
  return error;
}
