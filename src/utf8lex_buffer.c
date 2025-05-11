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

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                           utf8lex_buffer_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_buffer_init(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *prev,
        utf8lex_string_t *str,
        bool is_eof
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_buffer_init()");

  if (self == NULL
      || str == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->next = NULL;
  self->prev = prev;

  self->fd = -1;
  self->fp = NULL;

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

  UTF8LEX_DEBUG("EXIT utf8lex_buffer_init()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_buffer_clear(
        utf8lex_buffer_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_buffer_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_clear()");
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

  self->fd = -1;
  self->fp = NULL;

  self->str = NULL;


  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_buffer_clear()");
  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_buffer_add(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *tail
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_buffer_add()");

  if (self == NULL
      || tail == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_add()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (tail->prev != NULL
           || tail->next != NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_add()");
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
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_add()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_buffer_add()");
  return UTF8LEX_OK;
}
