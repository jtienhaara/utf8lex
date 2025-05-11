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
#include <inttypes.h>  // For int32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strlen(), memcpy().

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                      utf8lex_literal_definition_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_literal_definition_init(
        utf8lex_literal_definition_t *self,
        utf8lex_definition_t *prev,  // Previous definition in DB, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        unsigned char *str
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_literal_definition_init()");

  if (self == NULL
      || name == NULL
      || str == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_literal_definition_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_literal_definition_init()");
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  size_t num_bytes = strlen(str);
  if (num_bytes == (size_t) 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_literal_definition_init()");
    return UTF8LEX_ERROR_EMPTY_DEFINITION;
  }

  self->base.definition_type = UTF8LEX_DEFINITION_TYPE_LITERAL;
  self->base.name = name;
  self->base.next = NULL;
  self->base.prev = prev;
  self->str = str;

  // We know how many bytes the ltieral is.
  // Now we'll now use utf8proc to count how many characters,
  // graphemes, lines, etc. are in the matching region.

  utf8lex_string_t utf8lex_str;
  utf8lex_string_init(&utf8lex_str,  // self
                      num_bytes,     // max_length_bytes
                      num_bytes,     // length_bytes
                      self->str);    // bytes
  utf8lex_buffer_t buffer;
  utf8lex_buffer_init(&buffer,       // self
                      NULL,          // prev
                      &utf8lex_str,  // str
                      true);         // is_eof
  utf8lex_state_t state;
  utf8lex_state_init(&state,         // self
                     &buffer);       // buffer
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state.loc[unit].start = 0;
    state.loc[unit].length = 0;
    state.loc[unit].after = -1;
    state.loc[unit].hash = (unsigned long) 0;
  }

  off_t offset = (off_t) 0;
  utf8lex_location_t literal_loc[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    literal_loc[unit].start = (int) 0;
    literal_loc[unit].length = (int) 0;
    literal_loc[unit].after = (int) -1;  // No reset.
  }

  // Keep reading graphemes until we've reached the end of the literal string:
  for (int ug = 0;
       literal_loc[UTF8LEX_UNIT_BYTE].length < (int) num_bytes;
       ug ++)
  {
    // Read in one UTF-8 grapheme cluster per loop iteration:
    off_t grapheme_offset = offset;
    utf8lex_location_t grapheme_loc[UTF8LEX_UNIT_MAX];  // Unitialized is fine.
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        &state,  // state, including absolute locations.
        &grapheme_offset,  // start byte, relative to start of buffer string.
        grapheme_loc,  // Char, grapheme newline resets, and grapheme lengths
        &codepoint,  // codepoint
        &cat  //cat
        );

    if (error != UTF8LEX_OK)
    {
      // The literal is a string that utf8proc either needs MORE bytes for,
      // or rejected outright.  We can't accept it.
      UTF8LEX_DEBUG("EXIT utf8lex_literal_definition_init()");
      return error;
    }

    // We found another grapheme inside the literal string.
    // Keep looking for more graphemes inside the literal string.
    offset = grapheme_offset;
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      // Ignore the grapheme's start location.
      // Add to length (bytes, chars, graphemes, lines):
      literal_loc[unit].length += grapheme_loc[unit].length;
      // Possible resets to char, grapheme position due to newlines:
      literal_loc[unit].after = grapheme_loc[unit].after;
      // Hash of the grapheme (unsigned long, can wrap to 0):
      literal_loc[unit].hash = grapheme_loc[unit].hash;
    }
  }

  // Check to make sure strlen and utf8proc agree on # bytes.  (They should.)
  if (literal_loc[UTF8LEX_UNIT_BYTE].length != (int) num_bytes)
  {
    fprintf(stderr,
            "*** strlen and utf8proc disagree: strlen = %d vs utf8proc = %d\n",
            (int) num_bytes,
            literal_loc[UTF8LEX_UNIT_BYTE].length);
  }

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = literal_loc[unit].start;
    self->loc[unit].length = literal_loc[unit].length;
    self->loc[unit].after = literal_loc[unit].after;
    self->loc[unit].hash = literal_loc[unit].hash;
  }

  if (self->base.prev == NULL)
  {
    self->base.id = (uint32_t) 1;
  }
  else
  {
    self->base.id = self->base.prev->id + 1;
    if (self->base.id > UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_literal_definition_init()");
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    self->base.prev->next = (utf8lex_definition_t *) self;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_literal_definition_init()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_literal_definition_clear(
        utf8lex_definition_t *self  // Must be utf8lex_literal_definition_t *
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_literal_definition_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_literal_definition_clear()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->definition_type != UTF8LEX_DEFINITION_TYPE_LITERAL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_literal_definition_clear()");
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  utf8lex_literal_definition_t *literal_definition =
    (utf8lex_literal_definition_t *) self;

  if (literal_definition->base.next != NULL)
  {
    literal_definition->base.next->prev = literal_definition->base.prev;
  }
  if (literal_definition->base.prev != NULL)
  {
    literal_definition->base.prev->next = literal_definition->base.next;
  }

  literal_definition->base.definition_type = NULL;
  literal_definition->base.id = (uint32_t) 0;
  literal_definition->base.name = NULL;
  literal_definition->str = NULL;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    literal_definition->loc[unit].start = -1;
    literal_definition->loc[unit].length = -1;
    literal_definition->loc[unit].after = -2;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_literal_definition_clear()");
  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_lex_literal(
        utf8lex_rule_t *rule,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_lex_literal()");

  if (rule == NULL
      || rule->definition == NULL
      || rule->definition->definition_type == NULL
      || rule->definition->name == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token_pointer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex_literal()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (rule->definition->definition_type
           != UTF8LEX_DEFINITION_TYPE_LITERAL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex_literal()");
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  size_t remaining_bytes = state->buffer->str->length_bytes - (size_t) offset;

  utf8lex_literal_definition_t *literal =
    (utf8lex_literal_definition_t *) rule->definition;
  size_t token_length_bytes = literal->loc[UTF8LEX_UNIT_BYTE].length;

  for (off_t c = (off_t) 0;
       (size_t) c < remaining_bytes && (size_t) c < token_length_bytes;
       c ++)
  {
    if (state->buffer->str->bytes[offset + c] != literal->str[c])
    {
      UTF8LEX_DEBUG("EXIT utf8lex_lex_literal()");
      return UTF8LEX_NO_MATCH;
    }
  }

  if (remaining_bytes < token_length_bytes)
  {
    // Not enough bytes to read the string.
    // (It was matching for maybe a few bytes, anyway.)
    if (state->buffer->is_eof)
    {
      // No more bytes can be read in, we're at EOF.
      UTF8LEX_DEBUG("EXIT utf8lex_lex_literal()");
      return UTF8LEX_NO_MATCH;
    }
    else
    {
      // Need to read more bytes for the full grapheme.
      UTF8LEX_DEBUG("EXIT utf8lex_lex_literal()");
      return UTF8LEX_MORE;
    }
  }

  // Matched the literal exactly.
  utf8lex_location_t token_loc[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    token_loc[unit].start = state->loc[unit].start;
    token_loc[unit].length = literal->loc[unit].length;
    token_loc[unit].after = literal->loc[unit].after;  // -1 or new location.
    token_loc[unit].hash = literal->loc[unit].hash;
  }

  utf8lex_error_t error = utf8lex_token_init(
      token_pointer,  // self
      rule,  // rule
      rule->definition,  // definition
      token_loc,  // Resets for newlines, and lengths in bytes, chars, etc.
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex_literal()");
    return error;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_lex_literal()");
  return UTF8LEX_OK;
}


// A token definition that matches a literal string,
// such as "int" or "==" or "proc" and so on:
static utf8lex_definition_type_t UTF8LEX_DEFINITION_TYPE_LITERAL_INTERNAL =
  {
    .name = "LITERAL",
    .lex = utf8lex_lex_literal,
    .clear = utf8lex_literal_definition_clear
  };
utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_LITERAL =
  &UTF8LEX_DEFINITION_TYPE_LITERAL_INTERNAL;
