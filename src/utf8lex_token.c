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
//                        utf8lex_token_type_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_token_type_init(
        utf8lex_token_type_t *self,
        utf8lex_token_type_t *prev,
        unsigned char *name,
        utf8lex_abstract_pattern_t *pattern,
        unsigned char *code
        )
{
  if (self == NULL
      || name == NULL
      || pattern == NULL
      || pattern->pattern_type == NULL
      || pattern->pattern_type->lex == NULL
      || code == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  self->next = NULL;
  self->prev = prev;
  // id is set below, in the if/else statements.
  self->name = name;
  self->pattern = pattern;
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

  if (self->pattern != NULL
      && self->pattern->pattern_type != NULL
      && self->pattern->pattern_type->clear != NULL)
  {
    self->pattern->pattern_type->clear(self->pattern);
  }

  self->next = NULL;
  self->prev = NULL;
  self->id = (uint32_t) 0;
  self->name = NULL;
  self->pattern = NULL;
  self->code = NULL;

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
