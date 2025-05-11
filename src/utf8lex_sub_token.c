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
#include <inttypes.h>  // For uint32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strcmp().

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                        utf8lex_sub_token_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_sub_token_init(
        utf8lex_sub_token_t *self,
        utf8lex_sub_token_t *prev,
        utf8lex_token_t *token_to_copy,
        utf8lex_state_t *state
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_sub_token_init()");

  if (self == NULL
      || token_to_copy == NULL)
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

  utf8lex_error_t error = utf8lex_token_copy(
          token_to_copy,  // from
          &(self->token),  // to
          state);  // state
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
    return error;
  }

  if (self->token.definition == NULL
      || self->token.definition->name == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->id = self->token.definition->id;
  self->name = self->token.definition->name;

  self->next = NULL;
  self->prev = prev;

  if (self->prev != NULL)
  {
    self->prev->next = self;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_sub_token_init()");
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

  utf8lex_sub_token_t *sub_token = first_sub_token;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < UTF8LEX_SUB_TOKENS_LENGTH_MAX;
       infinite_loop_protector ++)
  {
    if (sub_token == NULL)
    {
      is_infinite_loop = false;
      break;
    }
    else if (sub_token->name == NULL)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_sub_token_find()");
      return UTF8LEX_ERROR_NULL_POINTER;
    }
    else if (strcmp(sub_token->name, name) == 0)
    {
      *found_pointer = sub_token;
      is_infinite_loop = false;
      break;
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

  utf8lex_sub_token_t *sub_token = first_sub_token;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < UTF8LEX_SUB_TOKENS_LENGTH_MAX;
       infinite_loop_protector ++)
  {
    if (sub_token == NULL)
    {
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
