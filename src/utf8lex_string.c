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
#include <string.h>  // For strlen(), strncmp(), memcpy

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
  UTF8LEX_DEBUG("ENTER utf8lex_string_init()");

  if (self == NULL
      || bytes == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_string_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (length_bytes < (size_t) 0
           || max_length_bytes < length_bytes)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_string_init()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  self->max_length_bytes = max_length_bytes;
  self->length_bytes = length_bytes;
  self->bytes = bytes;

  UTF8LEX_DEBUG("EXIT utf8lex_string_init()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_string_clear(
        utf8lex_string_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_string_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_string_clear()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->max_length_bytes = (size_t) 0;
  self->length_bytes = (size_t) 0;
  self->bytes = NULL;

  UTF8LEX_DEBUG("EXIT utf8lex_string_clear()");
  return UTF8LEX_OK;
}

// Helper to initialize a statically declared utf8lex_string_t.
utf8lex_error_t utf8lex_string(
        utf8lex_string_t *self,
        int max_length_bytes,
        unsigned char *content
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_string()");

  if (self == NULL
      || content == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_string()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (max_length_bytes <= 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_string()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  utf8lex_error_t error = utf8lex_string_init(
      self,  // self
      (size_t) max_length_bytes,  // max_length_bytes
      strlen(content),  // length_bytes
      content);  // bytes
  UTF8LEX_DEBUG("EXIT utf8lex_string()");
  return error;
}


// Compare the utf8lex_string_t at a specific start position and length
// to the specified content, with strcmp-style result stored in
// the specified int pointer.
utf8lex_error_t utf8lex_strcmp(
        utf8lex_string_t *self,
        utf8lex_location_t loc[UTF8LEX_UNIT_MAX],
        unsigned char *content,
        int *strcmp_result_pointer
        )
{
  if (content == NULL
      || self == NULL
      || self->bytes == NULL
      || strcmp_result_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  int start = loc[UTF8LEX_UNIT_BYTE].start;
  int length = loc[UTF8LEX_UNIT_BYTE].length;
  if (start < 0
      || start >= self->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length < 0
           || (start + length) > self->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  unsigned char *ptr = (unsigned char *) self->bytes + (off_t) start;
  *strcmp_result_pointer = strncmp(content,
                                   ptr,
                                   (size_t) length);

  return UTF8LEX_OK;
}


// Copies the utf8lex_string_t at a specific start position and length
// to the specified content.
utf8lex_error_t utf8lex_strcpy(
        utf8lex_string_t *self,
        utf8lex_location_t loc[UTF8LEX_UNIT_MAX],
        unsigned char *content
        )
{
  if (self == NULL
      || self->bytes == NULL
      || content == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  int start = loc[UTF8LEX_UNIT_BYTE].start;
  int length = loc[UTF8LEX_UNIT_BYTE].length;
  if (start < 0
      || start >= self->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length < 0
           || (start + length) > self->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  unsigned char *ptr = (unsigned char *) self->bytes + (off_t) start;
  memcpy(content,
          ptr,
          (size_t) length);

  return UTF8LEX_OK;
}
