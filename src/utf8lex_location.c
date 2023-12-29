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

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                          utf8lex_location_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_location_init(
        utf8lex_location_t *self,
        int start,
        int length
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (start < 0)
  {
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length < 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  self->start = start;
  self->length = length;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_location_clear(
        utf8lex_location_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->start = -1;
  self->length = -1;

  return UTF8LEX_OK;
}
