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
#include <string.h>  // For strcpy

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                            utf8lex_trace_pre()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_trace_pre(
        utf8lex_rule_t *rule,
        utf8lex_state_t *state
        )
{
  if (rule == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || state->buffer->str->bytes == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  unsigned char location_str[32];
  utf8lex_error_t trace_error = utf8lex_state_location_copy_string(
      state,          // state
      location_str,   // str
      32);            // max_bytes
  if (trace_error != UTF8LEX_OK)
  {
    strcpy(location_str, "?.?");
  }

  unsigned char next_byte = '?';
  int byte_num = state->loc[UTF8LEX_UNIT_BYTE].start;
  if (byte_num >= 0
      && byte_num < (int) state->buffer->str->length_bytes)
  {
    next_byte = state->buffer->str->bytes[byte_num];
  }

  fprintf(stdout, "TRACE: pre rule %u '%s' [%s]: '%c' (%d)\n",
          rule->id,
          rule->name,
          location_str,
          next_byte,
          (int) next_byte);
  fflush(stdout);

  return trace_error;
}


// ---------------------------------------------------------------------
//                            utf8lex_trace_pre()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_trace_post(
        utf8lex_rule_t *rule,
        utf8lex_state_t *state,
        utf8lex_token_t *token,
        utf8lex_error_t lex_error
        )
{
  if (rule == NULL
      || state == NULL
      || token == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t trace_error = UTF8LEX_OK;

  unsigned char location_str[32];
  trace_error = utf8lex_state_location_copy_string(
      state,          // state
      location_str,   // str
      32);            // max_bytes
  if (trace_error != UTF8LEX_OK)
  {
    strcpy(location_str, "?.?");
  }

  unsigned char trace_details[256];
  if (lex_error == UTF8LEX_OK)
  {
    trace_error = utf8lex_token_copy_string(
        token,          // self
        trace_details,  // str
        (size_t) 256);  // max_bytes
    if (trace_error == UTF8LEX_OK)
    {
      fprintf(stdout, "TRACE: post rule %u '%s' [%s]: token '%s'\n",
              rule->id,
              rule->name,
              location_str,
              trace_details);
    }
    else
    {
      fprintf(stdout, "TRACE: post rule %u '%s' [%s]: token (can't print)\n",
              rule->id,
              rule->name,
              location_str);
    }
  }
  else
  {
    utf8lex_string_t error_str;
    trace_error = utf8lex_string_init(
        &error_str,      // self
        (size_t) 255,    // max_length_bytes
        (size_t) 0,      // length_bytes
        trace_details);  // bytes
    if (trace_error == UTF8LEX_OK)
    {
      trace_error = utf8lex_error_string(
          &error_str,  // str
          lex_error);
      error_str.bytes[error_str.length_bytes] = '\0';
    }
    if (trace_error == UTF8LEX_OK)
    {
      fprintf(stdout, "TRACE: post rule %u '%s' [%s]: lex error %u '%s'\n",
              rule->id,
              rule->name,
              location_str,
              (unsigned long) lex_error,
              error_str.bytes);
    }
    else
    {
      fprintf(stdout, "TRACE: post rule %u '%s' [%s]: lex error %u (can't print)\n",
              rule->id,
              rule->name,
              location_str,
              (unsigned long) lex_error);
    }
  }

  fflush(stdout);
  return trace_error;
}
