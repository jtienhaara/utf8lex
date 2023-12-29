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

#include "utf8lex.h"


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
  case UTF8LEX_EOF:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_EOF");
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
  case UTF8LEX_ERROR_EMPTY_LITERAL:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_EMPTY_LITERAL");
    break;
  case UTF8LEX_ERROR_REGEX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_REGEX");
    break;
  case UTF8LEX_ERROR_UNIT:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_UNIT");
    break;
  case UTF8LEX_ERROR_INFINITE_LOOP:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_INFINITE_LOOP");
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
  case UTF8LEX_ERROR_BAD_MIN:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_MIN");
    break;
  case UTF8LEX_ERROR_BAD_MAX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_MAX");
    break;
  case UTF8LEX_ERROR_BAD_REGEX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_REGEX");
    break;
  case UTF8LEX_ERROR_BAD_UTF8:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_UTF8");
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
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "unknown error %d", (int) error);
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
