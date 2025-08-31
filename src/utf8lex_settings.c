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
#include <stdbool.h>  // For bool, true, false.

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                         utf8lex_settings_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_settings_init(
        utf8lex_settings_t *self,
        unsigned char *input_filename,  // or NULL.
        unsigned char *output_filename,  // or NULL.
        bool is_tracing
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_settings_init()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_settings_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->input_filename = input_filename;
  self->output_filename = output_filename;
  self->is_tracing = is_tracing;

  UTF8LEX_DEBUG("EXIT utf8lex_settings_init()");
  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_settings_init_defaults(
        utf8lex_settings_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_settings_init_defaults()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_settings_init_defaults()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = utf8lex_settings_init(
      self,   // self
      NULL,   // input_filename
      NULL,   // output_filename
      false); // is_tracing

  UTF8LEX_DEBUG("EXIT utf8lex_settings_init_defaults()");
  return error;
}


utf8lex_error_t utf8lex_settings_copy(
        utf8lex_settings_t *from,
        utf8lex_settings_t *to
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_settings_copy()");

  if (from == NULL
      || to == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_settings_copy()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = utf8lex_settings_init(
      to,                     // self
      from->input_filename,   // input_filename
      from->output_filename,  // output_filename
      from->is_tracing);      // is_tracing

  UTF8LEX_DEBUG("EXIT utf8lex_settings_copy()");
  return error;
}


utf8lex_error_t utf8lex_settings_clear(
        utf8lex_settings_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_settings_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_settings_clear()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->input_filename = NULL;
  self->output_filename = NULL;
  self->is_tracing = true;

  UTF8LEX_DEBUG("EXIT utf8lex_settings_clear()");
  return UTF8LEX_OK;
}
