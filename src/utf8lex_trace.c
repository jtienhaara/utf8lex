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
#include <string.h>  // For strcpy, strcat

#include <inttypes.h>  // For uint32_t

#include "utf8lex.h"


static utf8lex_error_t utf8lex_trace_indents(
    uint32_t num_indents,
    unsigned char indents[256]
    )
{
  if (indents == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  indents[0] = '\0';
  for (int indent = 0; indent < num_indents; indent ++)
  {
    if (indent >= 127)
    {
      // 256 char buffer isn't big enough for 128 * 2 byte indents plus '\0'.
      return UTF8LEX_MORE;
    }

    strcat(indents, "  ");
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_trace_location_to_str(
        utf8lex_state_t *state,
        unsigned char location_str[32]
        )
{
  if (state == NULL
      || location_str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = utf8lex_state_location_copy_string(
      state,          // state
      location_str,   // str
      32);            // max_bytes
  if (error != UTF8LEX_OK)
  {
    strcpy(location_str, "?.?");
  }

  return error;
}


static utf8lex_error_t utf8lex_trace_next_byte(
                                               utf8lex_state_t *state,
                                               unsigned char *next_byte_pointer
                                               )
{
  if (state == NULL
      || next_byte_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  *next_byte_pointer = '?';
  int byte_num = state->loc[UTF8LEX_UNIT_BYTE].start;
  if (byte_num >= 0
      && byte_num < (int) state->buffer->str->length_bytes)
  {
    *next_byte_pointer = state->buffer->str->bytes[byte_num];
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_trace_details_with_token(
        utf8lex_token_t *token,
        unsigned char trace_details[256]
        )
{
  if (token == NULL
      || trace_details == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  unsigned char token_text[256];
  utf8lex_error_t trace_error = utf8lex_token_copy_string(
      token,          // self
      token_text,     // str
      (size_t) 256);  // max_bytes
  if (trace_error == UTF8LEX_OK)
  {
    trace_error = utf8lex_printable_str(
            trace_details,
            (size_t) 256,
            token_text,
            UTF8LEX_PRINTABLE_ALL);
  }

  return trace_error;
}


static utf8lex_error_t utf8lex_trace_error(
        utf8lex_error_t lex_error,
        unsigned char trace_details[256]
        )
{
  if (trace_details == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_string_t error_str;
  utf8lex_error_t trace_error = utf8lex_string_init(
      &error_str,      // self
      (size_t) 255,    // max_length_bytes
      (size_t) 0,      // length_bytes
      trace_details);  // bytes

  if (trace_error != UTF8LEX_OK)
  {
    return trace_error;
  }

  trace_error = utf8lex_error_string(
      &error_str,  // str
      lex_error);
  error_str.bytes[error_str.length_bytes] = '\0';

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                            utf8lex_trace_definition_pre()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_trace_definition_pre(
        utf8lex_definition_t *definition,
        unsigned char *trace,
        utf8lex_state_t *state
        )
{
  if (definition == NULL
      || definition->definition_type == NULL
      || definition->definition_type->to_str == NULL
      || trace == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || state->buffer->str->bytes == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  unsigned char location_str[32];
  utf8lex_error_t trace_error = utf8lex_trace_location_to_str(state,
                                                              location_str);

  unsigned char next_byte;
  trace_error = utf8lex_trace_next_byte(state, &next_byte);

  unsigned char indents[256];
  utf8lex_error_t indent_error = utf8lex_trace_indents(
      state->num_tracing_indents,
      indents);
  state->num_tracing_indents ++;

  unsigned char definition_str[256];
  trace_error = definition->definition_type->to_str(definition,
                                                    definition_str,
                                                    256);

  fprintf(stdout, "TRACE: %spre  %s %s [%s]: '%c' (%d)\n",
          indents,
          definition_str,
          trace,
          location_str,
          next_byte,
          (int) next_byte);
  fflush(stdout);

  return trace_error;
}


// ---------------------------------------------------------------------
//                            utf8lex_trace_definition_post()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_trace_definition_post(
        utf8lex_definition_t *definition,
        unsigned char *trace,
        utf8lex_state_t *state,
        utf8lex_token_t *token,
        utf8lex_error_t lex_error
        )
{
  if (definition == NULL
      || definition->definition_type == NULL
      || definition->definition_type->to_str == NULL
      || trace == NULL
      || state == NULL
      || token == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  unsigned char indents[256];
  state->num_tracing_indents --;
  utf8lex_error_t indent_error = utf8lex_trace_indents(
      state->num_tracing_indents,
      indents);

  utf8lex_error_t trace_error = UTF8LEX_OK;

  unsigned char location_str[32];
  trace_error = utf8lex_trace_location_to_str(state, location_str);

  unsigned char definition_str[256];
  trace_error = definition->definition_type->to_str(definition,
                                                    definition_str,
                                                    256);

  unsigned char trace_details[256];
  if (lex_error == UTF8LEX_OK)
  {
    trace_error = utf8lex_trace_details_with_token(token,
                                                   trace_details);
    if (trace_error == UTF8LEX_OK)
    {
      fprintf(stdout, "TRACE: %spost %s %s [%s] SUCCESS: token '%s'\n",
              indents,
              definition_str,
              trace,
              location_str,
              trace_details);
    }
    else
    {
      fprintf(stdout, "TRACE: %spost %s %s [%s] SUCCESS: token (can't print)\n",
              indents,
              definition_str,
              trace,
              location_str);
    }
  }
  else
  {
    trace_error = utf8lex_trace_error(lex_error, trace_details);
    if (trace_error == UTF8LEX_OK)
    {
      fprintf(stdout, "TRACE: %spost %s %s [%s]: lex error %u '%s'\n",
              indents,
              definition_str,
              trace,
              location_str,
              (unsigned long) lex_error,
              trace_details);
    }
    else
    {
      fprintf(stdout, "TRACE: %spost %s %s [%s]: lex error %u (can't print)\n",
              indents,
              definition_str,
              trace,
              location_str,
              (unsigned long) lex_error);
    }
  }

  fflush(stdout);
  return trace_error;
}


// ---------------------------------------------------------------------
//                            utf8lex_trace_rule_pre()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_trace_rule_pre(
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
  utf8lex_error_t trace_error = utf8lex_trace_location_to_str(state,
                                                              location_str);

  unsigned char next_byte;
  trace_error = utf8lex_trace_next_byte(state, &next_byte);

  unsigned char indents[256];
  utf8lex_error_t indent_error = utf8lex_trace_indents(
      state->num_tracing_indents,
      indents);
  state->num_tracing_indents ++;

  fprintf(stdout, "TRACE: %spre  rule %u '%s' [%s]: '%c' (%d)\n",
          indents,
          rule->id,
          rule->name,
          location_str,
          next_byte,
          (int) next_byte);
  fflush(stdout);

  return trace_error;
}


// ---------------------------------------------------------------------
//                            utf8lex_trace_rule_post()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_trace_rule_post(
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

  unsigned char indents[256];
  state->num_tracing_indents --;
  utf8lex_error_t indent_error = utf8lex_trace_indents(
      state->num_tracing_indents,
      indents);

  utf8lex_error_t trace_error = UTF8LEX_OK;

  unsigned char location_str[32];
  trace_error = utf8lex_trace_location_to_str(state, location_str);

  unsigned char trace_details[256];
  if (lex_error == UTF8LEX_OK)
  {
    trace_error = utf8lex_trace_details_with_token(token,
                                                   trace_details);
    if (trace_error == UTF8LEX_OK)
    {
      fprintf(stdout, "TRACE: %spost rule %u '%s' [%s] SUCCESS: token '%s'\n",
              indents,
              rule->id,
              rule->name,
              location_str,
              trace_details);
    }
    else
    {
      fprintf(stdout, "TRACE: %spost rule %u '%s' [%s] SUCCESS: token (can't print)\n",
              indents,
              rule->id,
              rule->name,
              location_str);
    }
  }
  else
  {
    trace_error = utf8lex_trace_error(lex_error, trace_details);
    if (trace_error == UTF8LEX_OK)
    {
      fprintf(stdout, "TRACE: %spost rule %u '%s' [%s]: lex error %u '%s'\n",
              indents,
              rule->id,
              rule->name,
              location_str,
              (unsigned long) lex_error,
              trace_details);
    }
    else
    {
      fprintf(stdout, "TRACE: %spost rule %u '%s' [%s]: lex error %u (can't print)\n",
              indents,
              rule->id,
              rule->name,
              location_str,
              (unsigned long) lex_error);
    }
  }

  fflush(stdout);
  return trace_error;
}


extern utf8lex_error_t utf8lex_trace_pre(
        unsigned char *trace,
        utf8lex_state_t *state
        );
extern utf8lex_error_t utf8lex_trace_post(
        unsigned char *trace,
        utf8lex_state_t *state,
        utf8lex_error_t lex_error
        );


// ---------------------------------------------------------------------
//                            utf8lex_trace_pre()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_trace_pre(
        unsigned char *trace,
        utf8lex_state_t *state
        )
{
  if (trace == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || state->buffer->str->bytes == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  unsigned char location_str[32];
  utf8lex_error_t trace_error = utf8lex_trace_location_to_str(state,
                                                              location_str);

  unsigned char next_byte;
  trace_error = utf8lex_trace_next_byte(state, &next_byte);

  unsigned char indents[256];
  utf8lex_error_t indent_error = utf8lex_trace_indents(
      state->num_tracing_indents,
      indents);
  state->num_tracing_indents ++;

  fprintf(stdout, "TRACE: %spre  %s [%s]: '%c' (%d)\n",
          indents,
          trace,
          location_str,
          next_byte,
          (int) next_byte);
  fflush(stdout);

  return trace_error;
}


// ---------------------------------------------------------------------
//                            utf8lex_trace_post()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_trace_post(
        unsigned char *trace,
        utf8lex_state_t *state,
        utf8lex_error_t lex_error
        )
{
  if (trace == NULL
      || state == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  unsigned char indents[256];
  state->num_tracing_indents --;
  utf8lex_error_t indent_error = utf8lex_trace_indents(
      state->num_tracing_indents,
      indents);

  utf8lex_error_t trace_error = UTF8LEX_OK;

  unsigned char location_str[32];
  trace_error = utf8lex_trace_location_to_str(state, location_str);

  unsigned char trace_details[256];
  if (lex_error == UTF8LEX_OK)
  {
    fprintf(stdout, "TRACE: %spost %s [%s] SUCCESS\n",
            indents,
            trace,
            location_str);
  }
  else
  {
    trace_error = utf8lex_trace_error(lex_error, trace_details);
    if (trace_error == UTF8LEX_OK)
    {
      fprintf(stdout, "TRACE: %spost %s [%s]: lex error %u '%s'\n",
              indents,
              trace,
              location_str,
              (unsigned long) lex_error,
              trace_details);
    }
    else
    {
      fprintf(stdout, "TRACE: %spost %s [%s]: lex error %u (can't print)\n",
              indents,
              trace,
              location_str,
              (unsigned long) lex_error);
    }
  }

  fflush(stdout);
  return trace_error;
}
