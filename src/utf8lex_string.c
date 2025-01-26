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

// Helper to initialize a statically declared utf8lex_string_t.
utf8lex_error_t utf8lex_string(
        utf8lex_string_t *self,
        int max_length_bytes,
        unsigned char *content
        )
{
  if (self == NULL
      || content == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (max_length_bytes <= 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  utf8lex_error_t error = utf8lex_string_init(
      self,  // self
      (size_t) max_length_bytes,  // max_length_bytes
      (size_t) 0,  // length_bytes
      content);  // bytes
  return error;
}
