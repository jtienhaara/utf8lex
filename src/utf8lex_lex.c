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
//                            utf8lex_lex()
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_lex(
        utf8lex_rule_t *first_rule,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_lex()");

  if (first_rule == NULL
      || state == NULL
      || token_pointer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex()");
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
      state->loc[unit].after = -1;
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
        UTF8LEX_DEBUG("EXIT utf8lex_lex()");
        return UTF8LEX_EOF;
      }
      else
      {
        // Please, sir, may I have some more?
        UTF8LEX_DEBUG("EXIT utf8lex_lex()");
        return UTF8LEX_MORE;
      }
    }

    // Move on to the next buffer in the chain.
    state->buffer = state->buffer->next;
  }

  utf8lex_rule_t *matched = NULL;
  for (utf8lex_rule_t *rule = first_rule;
       rule != NULL;
       rule = rule->next)
  {
    utf8lex_error_t error;
    if (rule->definition == NULL
        || rule->definition->definition_type == NULL
        || rule->definition->definition_type->lex == NULL)
    {
      error = UTF8LEX_ERROR_NULL_POINTER;
      break;
    }

    // Trace pre.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_rule_pre(rule, state);
    }

    // Call the definition_type's lexer.  On successful tokenization,
    // it will set the absolute offset and lengths of the token
    // (and optionally update the lengths stored in the buffer
    // and absolute state).
    error = rule->definition->definition_type->lex(
        rule,
        state,
        token_pointer);

    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_rule_post(rule, state, token_pointer, error);
    }

    // Decide what to do with the lex result.
    if (error == UTF8LEX_NO_MATCH)
    {
      // Did not match this one rule.  Carry on with the loop.
      continue;
    }
    else if (error == UTF8LEX_MORE)
    {
      // Need to read more bytes before trying again.
      UTF8LEX_DEBUG("EXIT utf8lex_lex()");
      return error;
    }
    else if (error == UTF8LEX_OK)
    {
      // Matched the rule.  Break out of the loop.
      matched = rule;
      break;
    }
    else
    {
      UTF8LEX_DEBUG("EXIT utf8lex_lex()");
      // Some other error.  Return the error to the caller.
      UTF8LEX_DEBUG("EXIT utf8lex_lex()");
      return error;
    }
  }

  // If we get this far, we've either 1) matched a rule,
  // or 2) not matched any rule.
  if (matched == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex()");
    return UTF8LEX_NO_MATCH;
  }

  // We have a match.
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    int after = token_pointer->loc[unit].after;

    // Update buffer and absolute state locations past end of this token:
    if (token_pointer->loc[unit].after == -1)
    {
      int length_units = token_pointer->loc[unit].length;
      state->buffer->loc[unit].start += length_units;
      state->loc[unit].start += length_units;
    }
    else
    {
      // Chars, graphemes reset at newline:
      state->buffer->loc[unit].start = token_pointer->loc[unit].after;
      state->loc[unit].start = token_pointer->loc[unit].after;
    }
    state->buffer->loc[unit].length = 0;
    state->loc[unit].length = 0;
    state->buffer->loc[unit].after = -1;
    state->loc[unit].after = -1;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_lex()");
  return UTF8LEX_OK;
}
