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
#include <inttypes.h>  // For int32_t.

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                       utf8lex_cat_definition_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_cat_definition_init(
        utf8lex_cat_definition_t *self,
        utf8lex_definition_t *prev,  // Previous definition in DB, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        utf8lex_cat_t cat,  // The category, such as UTF8LEX_GROUP_LETTER.
        int min,  // Minimum consecutive occurrences of the cat (1 or more).
        int max  // Maximum consecutive occurrences (-1 = no limit).
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_cat_definition_init()");

  if (self == NULL
      || name == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_init()");
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }
  else if (cat <= UTF8LEX_CAT_NONE
           || cat >= UTF8LEX_CAT_MAX)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_init()");
    return UTF8LEX_ERROR_CAT;
  }
  else if (min <= 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_init()");
    return UTF8LEX_ERROR_BAD_MIN;
  }
  else if (max != -1
           && max < min)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_init()");
    return UTF8LEX_ERROR_BAD_MAX;
  }

  self->base.definition_type = UTF8LEX_DEFINITION_TYPE_CAT;
  self->base.name = name;
  self->base.next = NULL;
  self->base.prev = prev;
  self->cat = cat;
  utf8lex_format_cat(self->cat, self->str);
  self->min = min;
  self->max = max;

  if (self->base.prev == NULL)
  {
    self->base.id = (uint32_t) 1;
  }
  else
  {
    self->base.id = self->base.prev->id + 1;
    if (self->base.id > UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_init()");
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    self->base.prev->next = (utf8lex_definition_t *) self;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_init()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_cat_definition_clear(
        utf8lex_definition_t *self  // Must be utf8lex_cat_definition_t *
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_cat_definition_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_clear()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->definition_type != UTF8LEX_DEFINITION_TYPE_CAT)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_clear()");
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  utf8lex_cat_definition_t *cat_definition =
    (utf8lex_cat_definition_t *) self;

  if (cat_definition->base.next != NULL)
  {
    cat_definition->base.next->prev = cat_definition->base.prev;
  }
  if (cat_definition->base.prev != NULL)
  {
    cat_definition->base.prev->next = cat_definition->base.next;
  }

  cat_definition->base.definition_type = NULL;
  cat_definition->base.id = (uint32_t) 0;
  cat_definition->base.name = NULL;
  cat_definition->cat = UTF8LEX_CAT_NONE;
  cat_definition->str[0] = 0;
  cat_definition->min = 0;
  cat_definition->max = 0;

  UTF8LEX_DEBUG("EXIT utf8lex_cat_definition_clear()");
  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_lex_cat(
        utf8lex_rule_t *rule,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_lex_cat()");

  if (rule == NULL
      || rule->definition == NULL
      || rule->definition->definition_type == NULL
      || rule->definition->name == NULL
      || state == NULL
      || state->loc == NULL
      || state->buffer == NULL
      || state->buffer->loc == NULL
      || state->buffer->str == NULL
      || token_pointer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex_cat()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (rule->definition->definition_type
           != UTF8LEX_DEFINITION_TYPE_CAT)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex_cat()");
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  // Trace pre.
  if (state->settings.is_tracing == true)
  {
    utf8lex_trace_definition_pre(rule->definition, "Lex", state);
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  utf8lex_location_t token_loc[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    token_loc[unit].start = state->loc[unit].start;
    token_loc[unit].length = (int) 0;
    token_loc[unit].after = -1;  // No reset.
    token_loc[unit].hash = (unsigned long) 0;
  }

  utf8lex_cat_definition_t *cat_definition =
    (utf8lex_cat_definition_t *) rule->definition;
  for (int ug = 0;
       cat_definition->max == -1 || ug < cat_definition->max;
       ug ++)
  {
    // Read in one UTF-8 grapheme cluster:
    off_t grapheme_offset = offset;
    utf8lex_location_t grapheme_loc[UTF8LEX_UNIT_MAX];  // Unitialized is fine.
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        state,  // state, including absolute locations.
        &grapheme_offset,  // start byte, relative to start of buffer string.
        grapheme_loc,  // Char, grapheme newline resets, and grapheme lengths
        &codepoint,  // codepoint
        &cat  //cat
        );

    if (error == UTF8LEX_MORE)
    {
      // Trace post.
      if (state->settings.is_tracing == true)
      {
        utf8lex_trace_definition_post(rule->definition,
                                      "Lex (more)",
                                      state,
                                      token_pointer,
                                      error);
      }

      UTF8LEX_DEBUG("EXIT utf8lex_lex_cat()");
      return error;
    }
    else if (error != UTF8LEX_OK)
    {
      if (ug < cat_definition->min)
      {
        // Trace post.
        if (state->settings.is_tracing == true)
        {
          utf8lex_trace_definition_post(rule->definition,
                                        "Lex (too few)",
                                        state,
                                        token_pointer,
                                        error);
        }

        UTF8LEX_DEBUG("EXIT utf8lex_lex_cat()");
        return error;
      }
      else
      {
        // Finished reading at least (min) graphemes.  Done.
        UTF8LEX_DEBUG("EXIT utf8lex_lex_cat()");
        // We'll return to this bad UTF-8 grapheme the next time we lex.
        break;
      }
    }

    if (cat_definition->cat & cat)
    {
      // A/the category we're looking for.
      error = UTF8LEX_OK;
    }
    else if (ug < cat_definition->min)
    {
      // Not the category we're looking for, and we haven't found
      // at least (min) graphemes matching this category, so fail
      // with no match.
      // Trace post.
      if (state->settings.is_tracing == true)
      {
        utf8lex_trace_definition_post(rule->definition,
                                      "Lex (wrong cat, too few)",
                                      state,
                                      token_pointer,
                                      UTF8LEX_NO_MATCH);
      }

      UTF8LEX_DEBUG("EXIT utf8lex_lex_cat()");
      return UTF8LEX_NO_MATCH;
    }
    else
    {
      // Not a/the category we're looking for, but we already
      // finished reading at least (min) graphemes, so we're done.
      break;
    }

    // We found another grapheme of the expected cat.
    // Keep looking for more matches for this token,
    // until we hit the max.
    offset = grapheme_offset;
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      // Ignore the grapheme's start location.
      // Add to length (bytes, chars, graphemes, lines):
      token_loc[unit].length += grapheme_loc[unit].length;
      // Possible resets to char, grapheme position due to newlines:
      token_loc[unit].after = grapheme_loc[unit].after;
      // Hash of the grapheme (unsigned long, can wrap to 0):
      token_loc[unit].hash = grapheme_loc[unit].hash;
    }
  }

  // Found what we're looking for.
  utf8lex_error_t error = utf8lex_token_init(
      token_pointer,  // self
      rule,  // rule
      rule->definition,  // definition
      token_loc,  // Resets for newlines, and lengths in bytes, chars, etc.
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_definition_post(rule->definition,
                                    "Lex (token init)",
                                    state,
                                    token_pointer,
                                    error);
    }

    UTF8LEX_DEBUG("EXIT utf8lex_lex_cat()");
    return error;
  }

  // Trace post.
  if (state->settings.is_tracing == true)
  {
    utf8lex_trace_definition_post(rule->definition,
                                  "Lex",
                                  state,
                                  token_pointer,
                                  UTF8LEX_OK);
  }

  UTF8LEX_DEBUG("EXIT utf8lex_lex_cat()");
  return UTF8LEX_OK;
}


// A token definition that matches a sequence of N characters
// of a specific utf8lex_cat_t cat, such as UTF8LEX_GROUP_WHITESPACE:
static utf8lex_definition_type_t UTF8LEX_DEFINITION_TYPE_CAT_INTERNAL =
  {
    .name = "CATEGORY",
    .lex = utf8lex_lex_cat,
    .clear = utf8lex_cat_definition_clear
  };
utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_CAT =
  &UTF8LEX_DEFINITION_TYPE_CAT_INTERNAL;
