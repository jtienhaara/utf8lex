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
#include <string.h>  // For memcpy().

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                        utf8lex_rule_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_rule_init(
        utf8lex_rule_t *self,
        utf8lex_rule_t *prev,
        unsigned char *name,
        utf8lex_definition_t *definition,
        unsigned char *code,
        size_t code_length_bytes
        )
{
  if (self == NULL
      || name == NULL
      || definition == NULL
      || definition->definition_type == NULL
      || definition->definition_type->lex == NULL
      || code == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }
  else if (code_length_bytes < (size_t) 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  self->next = NULL;
  self->prev = prev;
  // id is set below, in the if/else statements.
  self->name = name;
  self->definition = definition;
  self->code = code;
  self->code_length_bytes = code_length_bytes;
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

utf8lex_error_t utf8lex_rule_clear(
        utf8lex_rule_t *self
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

  if (self->definition != NULL
      && self->definition->definition_type != NULL
      && self->definition->definition_type->clear != NULL)
  {
    self->definition->definition_type->clear(self->definition);
  }

  self->next = NULL;
  self->prev = NULL;
  self->id = (uint32_t) 0;
  self->name = NULL;
  self->definition = NULL;
  self->code = NULL;
  self->code_length_bytes = (size_t) -1;

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                           utf8lex_token_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_token_init(
        utf8lex_token_t *self,
        utf8lex_rule_t *rule,
        utf8lex_location_t token_loc[UTF8LEX_UNIT_MAX],  // Resets, lengths.
        utf8lex_state_t *state  // For buffer and absolute location.
        )
{
  if (self == NULL
      || rule == NULL
      || token_loc == NULL
      || state == NULL
      || state->loc == NULL
      || state->buffer == NULL
      || state->buffer->loc == NULL
      || state->buffer->str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Check valid buffer relative start locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (state->buffer->loc[unit].start < 0)
    {
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
      return UTF8LEX_ERROR_BAD_START;
    }
    else if (token_loc[unit].length < 0)
    {
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    else if (token_loc[unit].after < -1)  // -1 (reset) is OK.
    {
      return UTF8LEX_ERROR_BAD_AFTER;
    }
  }
  int start_byte = state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  int length_bytes = token_loc[UTF8LEX_UNIT_BYTE].length;
  if (length_bytes <= 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  else if ((start_byte + length_bytes) > state->buffer->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  self->rule = rule;
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

  self->rule = NULL;
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
