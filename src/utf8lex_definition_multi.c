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
#include <inttypes.h>  // For int32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strlen(), memcpy().

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                      utf8lex_multi_definition_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_reference_init(
        utf8lex_reference_t *self,
        utf8lex_reference_t *prev,  // Previous reference, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        int min,  // Minimum number of tokens (0 or more).
        int max,  // Maximum tokens, or -1 for no limit.
        utf8lex_multi_definition_t *parent  // Parent multi-definition.
        )
{
  if (self == NULL
      || name == NULL
      || parent == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (min <= 0)
  {
    return UTF8LEX_ERROR_BAD_MIN;
  }
  else if (max != -1
           && max < min)
  {
    return UTF8LEX_ERROR_BAD_MAX;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  self->definition_name = name;
  self->definition_or_null = NULL;  // Unresolved.

  self->min = min;
  self->max = max;

  self->next = NULL;
  self->prev = prev;

  self->parent = parent;

  if (self->parent->references == NULL)
  {
    if (self->prev != NULL)
    {
      return UTF8LEX_ERROR_STATE;
    }

    self->parent->references = self;
  }
  else if (self->prev == NULL)
  {
    return UTF8LEX_ERROR_STATE;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_reference_clear(
        utf8lex_reference_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (self->next != NULL)
  {
    self->next->prev = self->prev;
  }
  if (self->prev != NULL)
  {
    self->prev->next = self->next;
  }

  self->definition_name = NULL;
  self->definition_or_null = NULL;

  self->min = 0;
  self->max = 0;

  self->next = NULL;
  self->prev = NULL;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_reference_resolve(
        utf8lex_reference_t *self,
        utf8lex_definition_t *db
        )
{
  if (self == NULL
      || self->parent == NULL
      || db == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_ERROR_NOT_FOUND;

  utf8lex_multi_definition_t *ancestor = self->parent;
  uint32_t infinite_loop = UTF8LEX_MULTI_DEFINITION_DEPTH_MAX;
  bool is_infinite_loop = true;
  for (uint32_t d = 0; d < infinite_loop; d ++)
  {
    if (ancestor == NULL)
    {
      is_infinite_loop = false;
      break;
    }

    if (ancestor->db == NULL)
    {
      // Only references here, no definitions.
      ancestor = ancestor->parent;
      continue;
    }

    error = utf8lex_definition_find(
        ancestor->db,  // first_definition
        self->definition_name, // name
        &(self->definition_or_null));  // found_pointer

    if (error == UTF8LEX_OK)
    {
      is_infinite_loop = false;
      break;
    }
    else if (error != UTF8LEX_ERROR_NOT_FOUND)
    {
      return error;
    }

    ancestor = ancestor->parent;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (error == UTF8LEX_OK)
  {
    if (self->definition_or_null == NULL)
    {
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    return UTF8LEX_OK;
  }
  else if (error != UTF8LEX_ERROR_NOT_FOUND)
  {
    return error;
  }

  error = utf8lex_definition_find(
      db,  // first_definition
      self->definition_name, // name
      &(self->definition_or_null));  // found_pointer

  if (error == UTF8LEX_OK
      && self->definition_or_null == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_multi_definition_init(
        utf8lex_multi_definition_t *self,
        utf8lex_definition_t *prev,  // Previous definition in DB, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        utf8lex_multi_definition_t *parent,  // Parent or NULL.
        utf8lex_multi_type_t multi_type  // Sequence or ORed references, etc.
        )
{
  if (self == NULL
      || name == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }
  else if (multi_type <= UTF8LEX_MULTI_TYPE_NONE
           || multi_type >= UTF8LEX_MULTI_TYPE_MAX)
  {
    return UTF8LEX_ERROR_BAD_MULTI_TYPE;
  }

  self->base.definition_type = UTF8LEX_DEFINITION_TYPE_MULTI;
  self->base.name = name;
  self->base.next = NULL;
  self->base.prev = prev;

  self->multi_type = multi_type;
  self->references = NULL;  // Empty to start.  We'll add references here.
  self->db = NULL;  // Empty to start.  We'll add | multi-definitions here.

  self->parent = parent;  // If we're part of a parent's db.  Otherwise NULL.

  if (self->parent != NULL)
  {
    if (self->parent->db == NULL)
    {
      if (self->base.prev != NULL)
      {
        return UTF8LEX_ERROR_STATE;
      }

      self->parent->db = (utf8lex_definition_t *) self;
    }
    else if (self->base.prev == NULL)
    {
      return UTF8LEX_ERROR_STATE;
    }
  }

  if (self->base.prev == NULL)
  {
    self->base.id = (uint32_t) 0;
  }
  else
  {
    self->base.id = self->base.prev->id + 1;
    if (self->base.id >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
    {
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    self->base.prev->next = (utf8lex_definition_t *) self;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_multi_definition_clear(
        // self must be utf8lex_multi_definition_t *:
        utf8lex_definition_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->definition_type != UTF8LEX_DEFINITION_TYPE_MULTI)
  {
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  utf8lex_multi_definition_t *multi_definition =
    (utf8lex_multi_definition_t *) self;

  if (multi_definition->base.next != NULL)
  {
    multi_definition->base.next->prev = multi_definition->base.prev;
  }
  if (multi_definition->base.prev != NULL)
  {
    multi_definition->base.prev->next = multi_definition->base.next;
  }

  utf8lex_reference_t *reference = multi_definition->references;
  utf8lex_reference_t *next_reference = NULL;
  uint32_t infinite_loop = UTF8LEX_REFERENCES_LENGTH_MAX;
  bool is_infinite_loop = true;
  for (uint32_t r = 0; r < infinite_loop; r ++)
  {
    if (reference == NULL)
    {
      is_infinite_loop = false;
      break;
    }

    next_reference = reference->next;
    utf8lex_error_t error = utf8lex_reference_clear(reference);
    if (error != UTF8LEX_OK)
    {
      return error;
    }

    reference = next_reference;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  utf8lex_definition_t *child = multi_definition->db;
  utf8lex_definition_t *next_child = NULL;
  infinite_loop = UTF8LEX_DEFINITIONS_DB_LENGTH_MAX;
  is_infinite_loop = true;
  for (uint32_t d = 0; d < infinite_loop; d ++)
  {
    if (child == NULL)
    {
      is_infinite_loop = false;
      break;
    }

    next_child = child->next;
    if (child->definition_type == NULL
        || child->definition_type->clear == NULL)
    {
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    utf8lex_error_t error = child->definition_type->clear(child);
    if (error != UTF8LEX_OK)
    {
      return error;
    }

    child = next_child;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  multi_definition->base.definition_type = NULL;
  multi_definition->base.id = (uint32_t) 0;
  multi_definition->base.name = NULL;

  multi_definition->references = NULL;
  multi_definition->db = NULL;

  multi_definition->parent = NULL;

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_multi_definition_resolve(
        utf8lex_multi_definition_t *self,
        utf8lex_definition_t *db  // The main database to resolve references.
        )
{
  if (self == NULL
      || db == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->references == NULL)
  {
    return UTF8LEX_ERROR_EMPTY_DEFINITION;
  }

  utf8lex_reference_t *reference = self->references;
  uint32_t infinite_loop = UTF8LEX_REFERENCES_LENGTH_MAX;
  bool is_infinite_loop = true;
  for (uint32_t r = 0; r < infinite_loop; r ++)
  {
    if (reference == NULL)
    {
      is_infinite_loop = false;
      break;
    }

    utf8lex_error_t error = utf8lex_reference_resolve(reference,
                                                      db);
    if (error != UTF8LEX_OK)
    {
      return error;
    }

    reference = reference->next;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  utf8lex_definition_t *child = self->db;
  infinite_loop = UTF8LEX_DEFINITIONS_DB_LENGTH_MAX;
  is_infinite_loop = true;
  for (uint32_t d = 0; d < infinite_loop; d ++)
  {
    if (child == NULL)
    {
      is_infinite_loop = false;
      break;
    }

    if (child->definition_type == NULL
        || child->definition_type->clear == NULL)
    {
      return UTF8LEX_ERROR_NULL_POINTER;
    }
    else if (child->definition_type != UTF8LEX_DEFINITION_TYPE_MULTI)
    {
      // Skip.
      child = child->next;
      continue;
    }

    utf8lex_multi_definition_t *multi = (utf8lex_multi_definition_t *) child;
    utf8lex_error_t error = utf8lex_multi_definition_resolve(multi,  // self
                                                             db);  // db
    if (error != UTF8LEX_OK)
    {
      return error;
    }

    child = child->next;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_lex_multi(
        utf8lex_rule_t *rule,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  if (rule == NULL
      || rule->definition == NULL
      || rule->definition->definition_type == NULL
      || rule->definition->name == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (rule->definition->definition_type
           != UTF8LEX_DEFINITION_TYPE_MULTI)
  {
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  utf8lex_multi_definition_t *multi =
    (utf8lex_multi_definition_t *) rule->definition;

  if (multi->references == NULL)
  {
    return UTF8LEX_ERROR_EMPTY_DEFINITION;
  }

  // We need to push our own state, and pop on either success or error,
  // so that we do not update the state's location until
  // the entire multi-definition has been completely matched.
  utf8lex_state_t multi_state;
  utf8lex_buffer_t multi_buffer;
  utf8lex_error_t error = utf8lex_buffer_init(&multi_buffer,  // self
                                              NULL,  // prev
                                              state->buffer->str,  // str
                                              state->buffer->is_eof);  // is_eof
  error = utf8lex_state_init(&multi_state,  // self
                             &multi_buffer);  // buffer

  utf8lex_location_t sequence_loc[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    multi_state.loc[unit].start = state->loc[unit].start;
    multi_state.loc[unit].length = 0;
    multi_state.loc[unit].after = -1;

    multi_buffer.loc[unit].start = state->buffer->loc[unit].start;
    multi_buffer.loc[unit].length = 0;
    multi_buffer.loc[unit].after = -1;

    sequence_loc[unit].start = state->loc[unit].start;
    sequence_loc[unit].length = (int) 0;
    sequence_loc[unit].after = (int) -1;  // No reset after token.
  }

  utf8lex_reference_t *reference = multi->references;
  uint32_t infinite_loop = UTF8LEX_REFERENCES_LENGTH_MAX;
  bool is_infinite_loop = true;
  utf8lex_definition_t *matching_definition = NULL;
  for (uint32_t r = 0; r < infinite_loop; r ++)
  {
    if (reference == NULL)
    {
      is_infinite_loop = false;
      break;
    }

    utf8lex_definition_t *definition = reference->definition_or_null;
    if (definition == NULL)
    {
      return UTF8LEX_ERROR_UNRESOLVED_DEFINITION;
    }
    else if (definition->definition_type == NULL)
    {
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    utf8lex_rule_t child_rule;
    error = utf8lex_rule_init(
        &child_rule,  // self
        NULL,  // prev
        definition->name,  // name
        definition,  // definition
        "",  // code
        (size_t) 0);  // code_length_bytes
    utf8lex_token_t child_token;

    int max = reference->max;
    if (max == -1)
    {
      max = (int) UTF8LEX_REFERENCES_LENGTH_MAX;
    }
    int m;
    for (m = 0; m < max; m ++)
    {
      error = definition->definition_type->lex(
          &child_rule,
          &multi_state,
          &child_token);
      if (error == UTF8LEX_NO_MATCH)
      {
        break;
      }
      else if (error != UTF8LEX_OK)
      {
        return error;
      }

      for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
           unit < UTF8LEX_UNIT_MAX;
           unit ++)
      {
        multi_buffer.loc[unit].start += child_token.loc[unit].length;
        multi_buffer.loc[unit].length = 0;
        multi_buffer.loc[unit].after = child_token.loc[unit].after;

        multi_state.loc[unit].start += child_token.loc[unit].length;
        multi_state.loc[unit].length = 0;
        multi_state.loc[unit].after = child_token.loc[unit].after;

        sequence_loc[unit].length += child_token.loc[unit].length;
        sequence_loc[unit].after = child_token.loc[unit].after;
      }
    }

    if (m == UTF8LEX_REFERENCES_LENGTH_MAX)
    {
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }
    else if (m < reference->min
             && multi->multi_type == UTF8LEX_MULTI_TYPE_OR)
    {
      // Carry on searching for a definition that matches the incoming text.
      if (reference->next == NULL)
      {
        return UTF8LEX_NO_MATCH;
      }

      reference = reference->next;
      continue;
    }
    else if (m < reference->min)
    {
      return UTF8LEX_NO_MATCH;
    }
    else if (multi->multi_type == UTF8LEX_MULTI_TYPE_OR)
    {
      is_infinite_loop = false;
      matching_definition = definition;
      break;
    }
    else if (multi->multi_type == UTF8LEX_MULTI_TYPE_SEQUENCE)
    {
      if (matching_definition == NULL)
      {
        // Tie the token to the first definition in the sequence
        // (even though there might be 2 or more definitions in sequence).
        matching_definition = definition;
      }
      // Continue matching the sequence
    }

    reference = reference->next;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (matching_definition == NULL)
  {
    return UTF8LEX_ERROR_STATE;
  }

  // Matched the multi-definition references exactly.
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state->loc[unit].start = sequence_loc[unit].start;
    state->loc[unit].length = sequence_loc[unit].length;
    state->loc[unit].after = sequence_loc[unit].after;
  }

  error = utf8lex_token_init(
      token_pointer,  // self
      rule,  // rule
      matching_definition,
      sequence_loc,  // Resets for newlines, and lengths in bytes, chars, etc.
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


// A token definition that matches one or more other definitions,
// in sequence and/or logically grouped.
static utf8lex_definition_type_t UTF8LEX_DEFINITION_TYPE_MULTI_INTERNAL =
  {
    .name = "MULTI",
    .lex = utf8lex_lex_multi,
    .clear = utf8lex_multi_definition_clear
  };
utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_MULTI =
  &UTF8LEX_DEFINITION_TYPE_MULTI_INTERNAL;
