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
#include <inttypes.h>  // For uint32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strcmp().

#include "utf8lex.h"


// Migrates a hierarchy of sub-tokens to be children of the specified parent.
// This involves creating a new sub-token for each instance in the source
// sub-tokens from the specified state.
static utf8lex_error_t utf8lex_sub_token_migrate(
        utf8lex_sub_token_t *to_parent,
        utf8lex_sub_token_t *from_first_sub_token,
        utf8lex_state_t *from_state,
        utf8lex_state_t *to_state
        )
{
  to_parent->token.num_sub_tokens = 0;
  to_parent->token.sub_tokens = NULL;
  if (from_first_sub_token == NULL)
  {
    return UTF8LEX_OK;
  }

  utf8lex_sub_token_t *from = from_first_sub_token;
  utf8lex_sub_token_t *prev_migrated_sub_token = NULL;
  bool is_infinite_loop = false;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < UTF8LEX_SUB_TOKENS_LENGTH_MAX;
       infinite_loop_protector ++)
  {
    if (from == NULL)
    {
      break;
    }

    utf8lex_sub_token_t *migrated_sub_token =
      &(to_state->sub_tokens[to_state->num_used_sub_tokens]);
    to_state->num_used_sub_tokens ++;

    // Initializing the migrated_sub_token will recurse into its sub-sub-tokens,
    // if any.
    utf8lex_error_t error = utf8lex_sub_token_init(
        migrated_sub_token,  // self
        prev_migrated_sub_token,  // prev
        from->token.definition,  // definition
        &(from->token),  // token
        from_state,  // from_state
        to_state);  // to_state
    if (error != UTF8LEX_OK)
    {
      return error;
    }

    if (to_parent->token.sub_tokens == NULL)
    {
      to_parent->token.sub_tokens = migrated_sub_token;
    }
    to_parent->token.num_sub_tokens ++;

    prev_migrated_sub_token = migrated_sub_token;
    from = from->next;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                        utf8lex_sub_token_t
// ---------------------------------------------------------------------

// Copies the content from the specified token, but changes the sub-token's
// state.
utf8lex_error_t utf8lex_sub_token_init(
        utf8lex_sub_token_t *self,
        utf8lex_sub_token_t *prev,
        utf8lex_definition_t *definition,
        utf8lex_token_t *token,
        utf8lex_state_t *from_state,  // The state of the specified token.
        utf8lex_state_t *to_state  // The state of this sub-token.
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_sub_token_init()");

  if (self == NULL
      || definition == NULL
      || definition->definition_type == NULL
      || definition->name == NULL
      || token == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
      && prev->next != NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  // Set up a rule to that encompasses the definition matched
  // by this sub-token.
  // The toplevel rule points to a multi-definition.
  // To lex this sub-token again, we'll need a rule that points
  // to the child definition of the toplevel multi-definition,
  // a rule pointing to the cat or literal or regex or even child multi
  // definition that successfully lexed this sub-token.
  utf8lex_error_t error = utf8lex_rule_init(
      &(self->rule),  // self
      NULL,  // prev
      definition->name,  // name of the child rule
      definition,  // definition (child definition of the toplevel multi)
      "",  // code
      (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Use a temporary state that has the same start location as the token.
  utf8lex_state_t temporary_state;
  utf8lex_state_init(&temporary_state,        // self
                     &(to_state->settings),   // settings,
                     to_state->buffer,        // buffer
                     to_state->stack_depth);  // stack_depth
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    temporary_state.loc[unit].start = token->loc[unit].start;
    temporary_state.loc[unit].length = 0;
    temporary_state.loc[unit].after = -1;
    temporary_state.loc[unit].hash = 0;
  }

  // Now copy the rest of the content from the specified token,
  // using the temporary state to fake correct location of our state.
  // Do NOT copy sub-tokens to the temporary state.  We'll migrate them
  // to our target state down below.
  int num_sub_tokens = token->num_sub_tokens;
  utf8lex_sub_token_t *first_sub_token = token->sub_tokens;
  token->num_sub_tokens = 0;
  token->sub_tokens = NULL;
  error = utf8lex_token_copy(
      token,  // from
      &(self->token),  // to
      &temporary_state);  // state
  token->num_sub_tokens = num_sub_tokens;
  token->sub_tokens = first_sub_token;
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
    return error;
  }

  // Because of all the location checks, we have to override the sub-token's
  // target state after copying its token with the temporary state.
  self->token.str = to_state->buffer->str;

  // Re-point the token's rule from whatever was parsed (e.g. a stack variable)
  // to the rule we created above.
  self->token.rule = &(self->rule);

  self->id = self->token.definition->id;
  self->name = self->token.definition->name;

  self->next = NULL;
  self->prev = prev;

  if (self->prev != NULL)
  {
    self->prev->next = self;
  }

  // Migrate the sub-tokens to our target state.
  if ((to_state->num_used_sub_tokens + token->num_sub_tokens)
      > UTF8LEX_SUB_TOKENS_LENGTH_MAX)
  {
    utf8lex_token_clear(&(self->token));
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
    return UTF8LEX_ERROR_SUB_TOKENS_EXHAUSTED;
  }
  error = utf8lex_sub_token_migrate(self, token->sub_tokens,
                                    from_state, to_state);
  if (error != UTF8LEX_OK)
  {
    utf8lex_token_clear(&(self->token));
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
    return error;
  }

  if (self->token.definition != definition)
  {
    utf8lex_token_clear(&(self->token));
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
    return UTF8LEX_ERROR_DEFINITION_MISMATCH;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
  return UTF8LEX_OK;
}


extern utf8lex_error_t utf8lex_sub_token_copy(
        utf8lex_sub_token_t *from,
        utf8lex_sub_token_t *to,
        utf8lex_sub_token_t *to_prev,  // Can be NULL.
        utf8lex_state_t *state
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_sub_token_copy()");

  if (from == NULL
      || to == NULL
      // to_prev can be NULL.
      || state == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_copy()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (to_prev != NULL
      && to_prev->next != NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_copy()");
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  utf8lex_error_t error =
    utf8lex_token_init(&(to->token),  // self
                       from->token.rule,  // rule
                       from->token.definition,  // definition
                       from->token.loc,  // token_loc
                       state);  // state
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_copy()");
    return error;
  }

  to->id = from->id;
  to->name = from->name;
  to->next = NULL;
  to->prev = to_prev;

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_sub_token_clear(
        utf8lex_sub_token_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_sub_token_clear()");

  if (self == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_clear()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = utf8lex_token_clear(
          &(self->token));  // self
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_clear()");
    return error;
  }

  self->id = (uint32_t) 0;
  self->name = NULL;

  if (self->next != NULL)
  {
    self->next->prev = self->prev;
  }
  if (self->prev != NULL)
  {
    self->prev->next = self->next;
  }
  self->next = NULL;
  self->prev = NULL;

  UTF8LEX_DEBUG("EXIT utf8lex_sub_token_clear()");
  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_sub_token_find(
        utf8lex_sub_token_t *first_sub_token,  // Database to search
        unsigned char *name,
        int index,  // The N'th matching sub-token (0, 1, 2, ...).
        utf8lex_sub_token_t **found_pointer  // Mutable.
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_sub_token_find()");

  if (first_sub_token == NULL
      || name == NULL
      || found_pointer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (index < 0)
  {
    return UTF8LEX_ERROR_BAD_INDEX;
  }

  utf8lex_sub_token_t *sub_token = first_sub_token;
  utf8lex_sub_token_t *stack[UTF8LEX_MULTI_DEFINITION_DEPTH_MAX];
  int stack_frame = 0;
  stack[stack_frame] = sub_token;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  int num_found_indexes = 0;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < UTF8LEX_SUB_TOKENS_LENGTH_MAX;
       infinite_loop_protector ++)
  {
    if (sub_token == NULL)
    {
      if (stack_frame > 0)
      {
        stack_frame --;
        sub_token = stack[stack_frame];
        continue;
      }

      is_infinite_loop = false;
      break;
    }
    else if (sub_token->name == NULL)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find()");
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    if (strcmp(sub_token->name, name) == 0)
    {
      if (num_found_indexes == index)
      {
        *found_pointer = sub_token;
        is_infinite_loop = false;
        break;
      }
      num_found_indexes ++;
    }

    if (sub_token->token.sub_tokens != NULL)
    {
      stack[stack_frame] = sub_token->next;

      // Depth-first search.
      stack_frame ++;
      if (stack_frame >= UTF8LEX_MULTI_DEFINITION_DEPTH_MAX)
      {
        is_infinite_loop = true;
        break;
      }
      stack[stack_frame] = sub_token->token.sub_tokens;
      sub_token = stack[stack_frame];
      continue;
    }

    sub_token = sub_token->next;
  }

  if (is_infinite_loop == true)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (*found_pointer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find()");
    return UTF8LEX_ERROR_NOT_FOUND;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find()");
  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_sub_token_find_by_id(
        utf8lex_sub_token_t *first_sub_token,  // Database to search
        uint32_t id,
        int index,  // The N'th matching sub-token (0, 1, 2, ...).
        utf8lex_sub_token_t **found_pointer  // Mutable.
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_sub_token_find_by_id()");

  if (first_sub_token == NULL
      || found_pointer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find_by_id()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (id == (uint32_t) 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find_by_id()");
    return UTF8LEX_ERROR_BAD_ID;
  }
  else if (index < 0)
  {
    return UTF8LEX_ERROR_BAD_INDEX;
  }

  utf8lex_sub_token_t *sub_token = first_sub_token;
  utf8lex_sub_token_t *stack[UTF8LEX_MULTI_DEFINITION_DEPTH_MAX];
  int stack_frame = 0;
  stack[stack_frame] = sub_token;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < UTF8LEX_SUB_TOKENS_LENGTH_MAX;
       infinite_loop_protector ++)
  {
    if (sub_token == NULL)
    {
      if (stack_frame > 0)
      {
        stack_frame --;
        sub_token = stack[stack_frame];
        continue;
      }

      is_infinite_loop = false;
      break;
    }
    else if (sub_token->id == (uint32_t) 0)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find_by_id()");
      return UTF8LEX_ERROR_BAD_ID;
    }
    else if (sub_token->id == id)
    {
      *found_pointer = sub_token;
      is_infinite_loop = false;
      break;
    }

    if (sub_token->token.sub_tokens != NULL)
    {
      stack[stack_frame] = sub_token->next;

      // Depth-first search.
      stack_frame ++;
      if (stack_frame >= UTF8LEX_MULTI_DEFINITION_DEPTH_MAX)
      {
        is_infinite_loop = true;
        break;
      }
      stack[stack_frame] = sub_token->token.sub_tokens;
      sub_token = stack[stack_frame];
      continue;
    }

    sub_token = sub_token->next;
  }

  if (is_infinite_loop == true)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find_by_id()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (*found_pointer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find_by_id()");
    return UTF8LEX_ERROR_NOT_FOUND;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find_by_id()");
  return UTF8LEX_OK;
}


// Copies the sub-token text into the specified string, overwriting it.
// Returns UTF8LEX_MORE if the destination string truncates the sub-token:
utf8lex_error_t utf8lex_sub_token_copy_string(
        utf8lex_sub_token_t *self,
        unsigned char *str,
        size_t max_bytes)
{
  UTF8LEX_DEBUG("ENTER utf8lex_sub_token_copy_string()");

  if (self == NULL
      || str == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_copy_string()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = utf8lex_token_copy_string(
                              &(self->token),  // self
                              str,  // str
                              max_bytes);  // max_bytes

  UTF8LEX_DEBUG("EXIT utf8lex_sub_token_copy_string()");
  return error;
}

// Concatenates the sub-token text to the end of the specified string.
// Returns UTF8LEX_MORE if the destination string truncates the sub-token:
utf8lex_error_t utf8lex_sub_token_cat_string(
        utf8lex_sub_token_t *self,
        unsigned char *str,  // Text will be concatenated starting at '\0'.
        size_t max_bytes)
{
  UTF8LEX_DEBUG("ENTER utf8lex_sub_token_cat_string()");

  if (self == NULL
      || str == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_cat_string()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = utf8lex_token_cat_string(
                              &(self->token),  // self
                              str,  // str
                              max_bytes);  // max_bytes

  UTF8LEX_DEBUG("EXIT utf8lex_sub_token_cat_string()");
  return error;
}
