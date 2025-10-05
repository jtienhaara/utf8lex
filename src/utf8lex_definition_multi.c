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
  UTF8LEX_DEBUG("ENTER utf8lex_reference_init()");

  if (self == NULL
      || name == NULL
      || parent == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (min < 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_init()");
    return UTF8LEX_ERROR_BAD_MIN;
  }
  else if (max != -1
           && max < min)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_init()");
    return UTF8LEX_ERROR_BAD_MAX;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_init()");
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
      UTF8LEX_DEBUG("EXIT utf8lex_reference_init()");
      return UTF8LEX_ERROR_STATE;
    }

    self->parent->references = self;
  }
  else if (self->prev == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_init()");
    return UTF8LEX_ERROR_STATE;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_reference_init()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_reference_clear(
        utf8lex_reference_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_reference_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_clear()");
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

  UTF8LEX_DEBUG("EXIT utf8lex_reference_clear()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_reference_resolve(
        utf8lex_reference_t *self,
        utf8lex_definition_t *db
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_reference_resolve()");

  if (self == NULL
      || self->parent == NULL
      || db == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_resolve()");
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
      UTF8LEX_DEBUG("EXIT utf8lex_reference_resolve()");
      return error;
    }

    ancestor = ancestor->parent;
  }

  if (is_infinite_loop == true)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_resolve()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (error == UTF8LEX_OK)
  {
    if (self->definition_or_null == NULL)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_reference_resolve()");
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    UTF8LEX_DEBUG("EXIT utf8lex_reference_resolve()");
    return UTF8LEX_OK;
  }
  else if (error != UTF8LEX_ERROR_NOT_FOUND)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_resolve()");
    return error;
  }

  error = utf8lex_definition_find(
      db,  // first_definition
      self->definition_name, // name
      &(self->definition_or_null));  // found_pointer

  if (error == UTF8LEX_OK
      && self->definition_or_null == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_resolve()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_reference_resolve()");
    return error;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_reference_resolve()");
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
  UTF8LEX_DEBUG("ENTER utf8lex_multi_definition_init()");

  if (self == NULL
      || name == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_init()");
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }
  else if (multi_type <= UTF8LEX_MULTI_TYPE_NONE
           || multi_type >= UTF8LEX_MULTI_TYPE_MAX)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_init()");
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
        UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_init()");
        return UTF8LEX_ERROR_STATE;
      }

      self->parent->db = (utf8lex_definition_t *) self;
    }
    else if (self->base.prev == NULL)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_init()");
      return UTF8LEX_ERROR_STATE;
    }
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
      UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_init()");
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    self->base.prev->next = (utf8lex_definition_t *) self;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_init()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_multi_definition_clear(
        // self must be utf8lex_multi_definition_t *:
        utf8lex_definition_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_multi_definition_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_clear()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->definition_type != UTF8LEX_DEFINITION_TYPE_MULTI)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_clear()");
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
      UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_clear()");
      return error;
    }

    reference = next_reference;
  }

  if (is_infinite_loop == true)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_clear()");
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
      UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_clear()");
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    utf8lex_error_t error = child->definition_type->clear(child);
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_clear()");
      return error;
    }

    child = next_child;
  }

  if (is_infinite_loop == true)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_clear()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  multi_definition->base.definition_type = NULL;
  multi_definition->base.id = (uint32_t) 0;
  multi_definition->base.name = NULL;

  multi_definition->references = NULL;
  multi_definition->db = NULL;

  multi_definition->parent = NULL;

  UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_clear()");
  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_multi_definition_resolve(
        utf8lex_multi_definition_t *self,
        utf8lex_definition_t *db  // The main database to resolve references.
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_multi_definition_resolve()");

  if (self == NULL
      || db == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_resolve()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->references == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_resolve()");
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
      UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_resolve()");
      return error;
    }

    reference = reference->next;
  }

  if (is_infinite_loop == true)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_resolve()");
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
      UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_resolve()");
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
      UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_resolve()");
      return error;
    }

    child = child->next;
  }

  if (is_infinite_loop == true)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_resolve()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_resolve()");
  return UTF8LEX_OK;
}


//
// Formats the name and pattern of the specified utf8lex_definition_t
// into the specified string, returning UTF8LEX_MORE if it was truncated.
//
static utf8lex_error_t utf8lex_multi_definition_to_str(
        utf8lex_definition_t *self,
        unsigned char *str,
        size_t max_bytes
        )
{
  if (self == NULL
      || self->name == NULL
      || self->definition_type == NULL
      || self->definition_type->name == NULL
      || str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->definition_type != UTF8LEX_DEFINITION_TYPE_MULTI)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_multi_definition_to_str()");
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  utf8lex_multi_definition_t *multi_definition =
    (utf8lex_multi_definition_t *) self;

  size_t num_bytes_written = (size_t) 0;

  // Type of the definition.
  num_bytes_written += snprintf(str + num_bytes_written,
                                max_bytes - num_bytes_written,
                                "multi ");

  // Name of the definition.
  num_bytes_written += snprintf(str + num_bytes_written,
                                max_bytes - num_bytes_written,
                                "'%s' ( ",
                                self->name);

  utf8lex_error_t error = UTF8LEX_OK;
  utf8lex_reference_t *reference = multi_definition->references;
  uint32_t infinite_loop = UTF8LEX_REFERENCES_LENGTH_MAX;
  bool is_infinite_loop = true;
  for (uint32_t r = 0; r < infinite_loop; r ++)
  {
    if (reference == NULL)
    {
      is_infinite_loop = false;
      break;
    }

    if (r == 0)
    {
      num_bytes_written += snprintf(str + num_bytes_written,
                                    max_bytes - num_bytes_written,
                                    "%s",
                                    reference->definition_name);
    }
    else if (multi_definition->multi_type == UTF8LEX_MULTI_TYPE_SEQUENCE)
    {
      num_bytes_written += snprintf(str + num_bytes_written,
                                    max_bytes - num_bytes_written,
                                    ", %s",
                                    reference->definition_name);
    }
    else if (multi_definition->multi_type == UTF8LEX_MULTI_TYPE_OR)
    {
      num_bytes_written += snprintf(str + num_bytes_written,
                                    max_bytes - num_bytes_written,
                                    " | %s",
                                    reference->definition_name);
    }
    else
    {
      error = UTF8LEX_ERROR_BAD_MULTI_TYPE;
      num_bytes_written += snprintf(str + num_bytes_written,
                                    max_bytes - num_bytes_written,
                                    " ? %s",
                                    reference->definition_name);
    }

    if (reference->min != 1
        || reference->max != 1)
    {
      num_bytes_written += snprintf(str + num_bytes_written,
                                    max_bytes - num_bytes_written,
                                    "[%d..%d]",
                                    reference->min,
                                    reference->max);
    }

    reference = reference->next;
  }

  num_bytes_written += snprintf(str + num_bytes_written,
                                max_bytes - num_bytes_written,
                                " )");

  if (is_infinite_loop)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_lex_multi(
        utf8lex_rule_t *rule,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_lex_multi()");

  if (rule == NULL
      || rule->definition == NULL
      || rule->definition->definition_type == NULL
      || rule->definition->name == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token_pointer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (rule->definition->definition_type
           != UTF8LEX_DEFINITION_TYPE_MULTI)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  // Trace pre.
  if (state->settings.is_tracing == true)
  {
    utf8lex_trace_definition_pre(rule->definition, "Lex", state);
  }

  utf8lex_multi_definition_t *multi =
    (utf8lex_multi_definition_t *) rule->definition;

  if (multi->references == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_definition_post(rule->definition,
                                    "Lex (multi no references)",
                                    state,
                                    token_pointer,
                                    UTF8LEX_ERROR_EMPTY_DEFINITION);
    }

    return UTF8LEX_ERROR_EMPTY_DEFINITION;
  }

  // We need to push our own state, and pop on either success or error,
  // so that we do not update the state's location until
  // the entire multi-definition has been completely matched.
  utf8lex_state_t multi_state;
  utf8lex_buffer_t multi_buffer;
  utf8lex_settings_t multi_settings;
  utf8lex_error_t error = utf8lex_buffer_init(&multi_buffer,  // self
                                              NULL,  // prev
                                              state->buffer->str,  // str
                                              state->buffer->is_eof);  // is_eof
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_definition_post(rule->definition,
                                    "Lex (multi buffer init)",
                                    state,
                                    token_pointer,
                                    error);
    }
    return error;
  }
  error = utf8lex_settings_copy(&(state->settings), &multi_settings);
  if (error != UTF8LEX_OK)
  {
    utf8lex_buffer_clear(&multi_buffer);
    UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_definition_post(rule->definition,
                                    "Lex (multi settings copy)",
                                    state,
                                    token_pointer,
                                    error);
    }
    return error;
  }
  error = utf8lex_state_init(&multi_state,             // self
                             &multi_settings,          // settings
                             &multi_buffer,            // buffer
                             state->stack_depth + 1);  // stack_depth
  if (error != UTF8LEX_OK)
  {
    utf8lex_buffer_clear(&multi_buffer);
    utf8lex_settings_clear(&multi_settings);
    UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_definition_post(rule->definition,
                                    "Lex (multi state init)",
                                    state,
                                    token_pointer,
                                    error);
    }
    return error;
  }
  multi_state.num_tracing_indents = state->num_tracing_indents;

  utf8lex_rule_t child_rule;
  utf8lex_token_t child_token;

  // Tracks the location of the whole token, across all sub-tokens
  // (start always remains the same, but length, after and hash are
  // updated for each matching sub-token).
  utf8lex_location_t sequence_loc[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    multi_state.loc[unit].start = state->loc[unit].start;
    multi_state.loc[unit].length = 0;
    multi_state.loc[unit].after = -1;
    multi_state.loc[unit].hash = (unsigned long) 0;

    multi_buffer.loc[unit].start = state->buffer->loc[unit].start;
    multi_buffer.loc[unit].length = 0;
    multi_buffer.loc[unit].after = -1;
    multi_buffer.loc[unit].hash = (unsigned long) 0;

    sequence_loc[unit].start = state->loc[unit].start;
    sequence_loc[unit].length = (int) 0;
    sequence_loc[unit].after = (int) -1;  // No reset after token.
    sequence_loc[unit].hash = (unsigned long) 0;
  }

  // Initialize the child_token so it's not full of junk
  // (lexing with it will overwrite these initial values).
  child_token.rule = NULL;
  child_token.definition = NULL;
  child_token.start_byte = -1;
  child_token.length_bytes = -1;
  child_token.str = NULL;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    child_token.loc[unit].start = -1;
    child_token.loc[unit].length = -1;
    child_token.loc[unit].after = -1;
    child_token.loc[unit].hash = -1;
  }
  child_token.sub_tokens = NULL;
  child_token.parent_or_null = NULL;

  utf8lex_reference_t *reference = multi->references;
  uint32_t infinite_loop = UTF8LEX_REFERENCES_LENGTH_MAX;
  bool is_infinite_loop = true;
  utf8lex_definition_t *matching_definition = NULL;
  utf8lex_sub_token_t *first_sub_token = NULL;
  utf8lex_sub_token_t *prev_sub_token = NULL;
  int num_sub_tokens = 0;
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
      utf8lex_state_clear(&multi_state);

      UTF8LEX_DEBUG("EXIT utf8lex_lex_multi() reference");
      // Trace post.
      if (state->settings.is_tracing == true)
      {
        utf8lex_trace_definition_post(rule->definition,
                                      "Lex (multi reference definition null)",
                                      state,
                                      token_pointer,
                                      UTF8LEX_ERROR_UNRESOLVED_DEFINITION);
      }
      return UTF8LEX_ERROR_UNRESOLVED_DEFINITION;
    }
    else if (definition->definition_type == NULL)
    {
      utf8lex_state_clear(&multi_state);

      UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
      // Trace post.
      if (state->settings.is_tracing == true)
      {
        utf8lex_trace_definition_post(rule->definition,
                                      "Lex (multi definition type null)",
                                      state,
                                      token_pointer,
                                      UTF8LEX_ERROR_NULL_POINTER);
      }
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    error = utf8lex_rule_init(
        &child_rule,  // self
        NULL,  // prev
        definition->name,  // name
        definition,  // definition
        "",  // code
        (size_t) 0);  // code_length_bytes

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
        child_rule.definition = NULL;  // definition is toplevel, don't clear.
        utf8lex_rule_clear(&child_rule);
        utf8lex_state_clear(&multi_state);

        UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
        // Trace post.
        if (state->settings.is_tracing == true)
        {
          utf8lex_trace_definition_post(rule->definition,
                                        "Lex (multi child lex)",
                                        state,
                                        token_pointer,
                                        error);
        }
        return error;
      }

      // The child_token matched the current child definition.
      child_token.definition = definition;

      // Create a sub-token for the matching child.
      if (state->num_used_sub_tokens >= UTF8LEX_SUB_TOKENS_LENGTH_MAX)
      {
        // No more sub-tokens in the toplevel state, we can't
        // allocate any more.
        child_rule.definition = NULL;  // definition is toplevel, don't clear.
        utf8lex_rule_clear(&child_rule);
        utf8lex_token_clear(&child_token);
        utf8lex_state_clear(&multi_state);
        UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
        // Trace post.
        if (state->settings.is_tracing == true)
        {
          utf8lex_trace_definition_post(rule->definition,
                                        "Lex (multi too many sub-tokens)",
                                        state,
                                        token_pointer,
                                        UTF8LEX_ERROR_SUB_TOKENS_EXHAUSTED);
        }
        return UTF8LEX_ERROR_SUB_TOKENS_EXHAUSTED;
      }
      utf8lex_sub_token_t *sub_token =
        &(state->sub_tokens[state->num_used_sub_tokens]);
      state->num_used_sub_tokens ++;
      num_sub_tokens ++;
      // Initialize the sub-token, and let it steal our lexed child token's
      // sub-tokens, if any.
      error = utf8lex_sub_token_init(
              sub_token,  // self
              prev_sub_token,  // prev
              child_rule.definition,  // definition
              &child_token,  // token
              &multi_state,  // from_state
              state);  // to_state
      if (error != UTF8LEX_OK)
      {
        child_rule.definition = NULL;  // definition is toplevel, don't clear.
        utf8lex_rule_clear(&child_rule);
        utf8lex_token_clear(&child_token);
        utf8lex_state_clear(&multi_state);

        UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
        // Trace post.
        if (state->settings.is_tracing == true)
        {
          utf8lex_trace_definition_post(rule->definition,
                                        "Lex (multi sub-token init)",
                                        state,
                                        token_pointer,
                                        error);
        }
        return error;
      }
      sub_token->token.parent_or_null = token_pointer;
      prev_sub_token = sub_token;
      if (first_sub_token == NULL)
      {
        first_sub_token = sub_token;
      }

      for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
           unit < UTF8LEX_UNIT_MAX;
           unit ++)
      {
        multi_buffer.loc[unit].start += child_token.loc[unit].length;
        multi_buffer.loc[unit].length = 0;
        multi_buffer.loc[unit].after = child_token.loc[unit].after;
        multi_buffer.loc[unit].hash
          <<= (8 * child_token.loc[UTF8LEX_UNIT_BYTE].length);
        multi_buffer.loc[unit].hash |= child_token.loc[unit].hash;

        multi_state.loc[unit].start += child_token.loc[unit].length;
        multi_state.loc[unit].length = 0;
        multi_state.loc[unit].after = child_token.loc[unit].after;
        multi_state.loc[unit].hash
          <<= (8 * child_token.loc[UTF8LEX_UNIT_BYTE].length);
        multi_state.loc[unit].hash |= child_token.loc[unit].hash;

        sequence_loc[unit].length += child_token.loc[unit].length;
        sequence_loc[unit].after = child_token.loc[unit].after;
        sequence_loc[unit].hash
          <<= (8 * child_token.loc[UTF8LEX_UNIT_BYTE].length);
        sequence_loc[unit].hash |= child_token.loc[unit].hash;
      }
    }

    if (m == UTF8LEX_REFERENCES_LENGTH_MAX)
    {
      child_rule.definition = NULL;  // definition is toplevel, don't clear.
      utf8lex_rule_clear(&child_rule);
      utf8lex_token_clear(&child_token);
      utf8lex_state_clear(&multi_state);

      UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
      // Trace post.
      if (state->settings.is_tracing == true)
      {
        utf8lex_trace_definition_post(rule->definition,
                                      "Lex (multi max refs)",
                                      state,
                                      token_pointer,
                                      UTF8LEX_ERROR_INFINITE_LOOP);
      }
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }
    else if (m < reference->min
             && multi->multi_type == UTF8LEX_MULTI_TYPE_OR)
    {
      // Didn't match.
      // Carry on searching for a definition that matches the incoming text.
      if (reference->next == NULL)
      {
        child_rule.definition = NULL;  // definition is toplevel, don't clear.
        utf8lex_rule_clear(&child_rule);
        utf8lex_token_clear(&child_token);
        utf8lex_state_clear(&multi_state);

        UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
        // Trace post.
        if (state->settings.is_tracing == true)
        {
          utf8lex_trace_definition_post(rule->definition,
                                        "Lex (multi min OR sub-tokens)",
                                        state,
                                        token_pointer,
                                        UTF8LEX_NO_MATCH);
        }
        return UTF8LEX_NO_MATCH;
      }

      reference = reference->next;
      num_sub_tokens = 0;
      continue;
    }
    else if (m < reference->min)
    {
      child_rule.definition = NULL;  // definition is toplevel, don't clear.
      utf8lex_rule_clear(&child_rule);
      utf8lex_token_clear(&child_token);
      utf8lex_state_clear(&multi_state);

      UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
      // Trace post.
      if (state->settings.is_tracing == true)
      {
        utf8lex_trace_definition_post(rule->definition,
                                      "Lex (multi min sub-tokens)",
                                      state,
                                      token_pointer,
                                      UTF8LEX_NO_MATCH);
      }
      return UTF8LEX_NO_MATCH;
    }
    else if (multi->multi_type == UTF8LEX_MULTI_TYPE_OR)
    {
      is_infinite_loop = false;
      matching_definition = definition;
      child_rule.definition = NULL;  // definition is toplevel, don't clear.
      utf8lex_rule_clear(&child_rule);
      utf8lex_token_clear(&child_token);
      break;
    }
    else if (multi->multi_type == UTF8LEX_MULTI_TYPE_SEQUENCE)
    {
      if (matching_definition == NULL)
      {
        // Tie the token to the toplevel sequence multi-definition, rather than
        // to a single child definition.
        matching_definition = rule->definition;
      }
      // Continue matching the sequence
    }

    child_rule.definition = NULL;  // definition is toplevel, don't clear.
    utf8lex_rule_clear(&child_rule);
    utf8lex_token_clear(&child_token);

    reference = reference->next;
  }

  if (is_infinite_loop == true)
  {
    utf8lex_state_clear(&multi_state);

    UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_definition_post(rule->definition,
                                    "Lex (multi infinite references)",
                                    state,
                                    token_pointer,
                                    UTF8LEX_ERROR_INFINITE_LOOP);
    }
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (matching_definition == NULL)
  {
    utf8lex_state_clear(&multi_state);
    UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_definition_post(rule->definition,
                                    "Lex (multi no matching definition)",
                                    state,
                                    token_pointer,
                                    UTF8LEX_ERROR_STATE);
    }
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
    state->loc[unit].hash = sequence_loc[unit].hash;
  }

  // If we only have 1 sub-token, just free it.
  if (num_sub_tokens == 1)
  {
    error = utf8lex_sub_token_clear(first_sub_token);
    if (error != UTF8LEX_OK)
    {
      utf8lex_state_clear(&multi_state);

      UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
      // Trace post.
      if (state->settings.is_tracing == true)
      {
        utf8lex_trace_definition_post(rule->definition,
                                      "Lex (multi token init)",
                                      state,
                                      token_pointer,
                                      error);
      }
      return error;
    }

    state->num_used_sub_tokens --;
    num_sub_tokens = 0;
    first_sub_token = NULL;
  }

  error = utf8lex_token_init(
      token_pointer,  // self
      rule,  // rule
      matching_definition,
      sequence_loc,  // Resets for newlines, and lengths in bytes, chars, etc.
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    utf8lex_state_clear(&multi_state);

    UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_definition_post(rule->definition,
                                    "Lex (multi token init)",
                                    state,
                                    token_pointer,
                                    error);
    }
    return error;
  }

  token_pointer->num_sub_tokens = num_sub_tokens;
  token_pointer->sub_tokens = first_sub_token;

  utf8lex_state_clear(&multi_state);

  UTF8LEX_DEBUG("EXIT utf8lex_lex_multi()");
  // Trace post.
  if (state->settings.is_tracing == true)
  {
    utf8lex_trace_definition_post(rule->definition,
                                  "Lex (multi)",
                                  state,
                                  token_pointer,
                                  UTF8LEX_OK);
  }

  return UTF8LEX_OK;
}


// A token definition that matches one or more other definitions,
// in sequence and/or logically grouped.
static utf8lex_definition_type_t UTF8LEX_DEFINITION_TYPE_MULTI_INTERNAL =
  {
    .name = "MULTI",
    .lex = utf8lex_lex_multi,
    .to_str = utf8lex_multi_definition_to_str,
    .clear = utf8lex_multi_definition_clear
  };
utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_MULTI =
  &UTF8LEX_DEFINITION_TYPE_MULTI_INTERNAL;
