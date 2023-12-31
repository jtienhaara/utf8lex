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
#include <stdbool.h>  // For bool, true, false.

#include "utf8lex.h"


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

utf8lex_error_t utf8lex_lex(
        utf8lex_token_type_t *first_token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  if (first_token_type == NULL
      || state == NULL
      || token_pointer == NULL)
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
    if (token_type->pattern == NULL
        || token_type->pattern->pattern_type == NULL
        || token_type->pattern->pattern_type->lex == NULL)
    {
      error = UTF8LEX_ERROR_NULL_POINTER;
      break;
    }

    // Call the pattern_type's lexer.  On successful tokenization,
    // it will set the absolute offset and lengths of the token
    // (and optionally update the lengths stored in the buffer
    // and absolute state).
    error = token_type->pattern->pattern_type->lex(
        token_type,
        state,
        token_pointer);

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
    int length_units = token_pointer->loc[unit].length;

    // Update buffer locations past end of this token:
    state->buffer->loc[unit].start += length_units;
    state->buffer->loc[unit].length = 0;
    // Update absolute locations past end of this token:
    state->loc[unit].start += length_units;
    state->loc[unit].length = 0;
  }

  return UTF8LEX_OK;
}
