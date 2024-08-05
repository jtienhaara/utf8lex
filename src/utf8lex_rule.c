/*
 * utf8lex
 * Copyright © 2023-2024 Johann Tienhaara
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
#include <string.h>  // For strcmp(), strlen()

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                        utf8lex_rule_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_rule_init(
        utf8lex_rule_t *self,
        utf8lex_rule_t *prev,
        unsigned char *name,
        utf8lex_definition_t *definition,
        unsigned char *code,
        size_t code_length_bytes  // (size_t) -1 to use strlen(code).
        )
{
  if (self == NULL
      || name == NULL
      || definition == NULL
      || definition->definition_type == NULL
      || definition->definition_type->lex == NULL
      || code == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  self->next = NULL;
  self->prev = prev;
  // id is set below, in the if/else statements.
  self->name = name;
  self->definition = definition;
  self->code = code;
  if (code_length_bytes < (size_t) 0)
  {
    self->code_length_bytes = strlen(self->code);
  }
  else
  {
    self->code_length_bytes = code_length_bytes;
  }
  if (self->prev != NULL)
  {
    self->id = self->prev->id + 1;
    if (self->id >= UTF8LEX_RULES_DB_LENGTH_MAX)
    {
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    self->prev->next = self;
  }
  else
  {
    self->id = (uint32_t) 0;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_rule_clear(
        utf8lex_rule_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self->next;
  }

  if (self->next != NULL)
  {
    self->next->prev = self->prev;
  }

  if (self->definition != NULL
      && self->definition->definition_type != NULL
      && self->definition->definition_type->clear != NULL)
  {
    self->definition->definition_type->clear(self->definition);
  }

  self->next = NULL;
  self->prev = NULL;
  self->id = (uint32_t) 0;
  self->name = NULL;
  self->definition = NULL;
  self->code = NULL;
  self->code_length_bytes = (size_t) -1;

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_rule_find(
        utf8lex_rule_t *first_rule,  // Database to search.
        unsigned char *name,  // Name of rule to search for.
        utf8lex_rule_t ** found_pointer  // Gets set when found.
        )
{
  if (first_rule == NULL
      || name == NULL
      || found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_rule_t *rule = first_rule;
  uint32_t infinite_loop = UTF8LEX_RULES_DB_LENGTH_MAX;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  for (uint32_t d = 0; d < infinite_loop; d ++)
  {
    if (strcmp(rule->name, name) == 0)
    {
      *found_pointer = rule;
      is_infinite_loop = false;
      break;
    }

    if (rule->next == NULL)
    {
      *found_pointer = NULL;
      is_infinite_loop = false;
      break;
    }

    rule = rule->next;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (*found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NOT_FOUND;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_rule_find_by_id(
        utf8lex_rule_t *first_rule,  // Database to search.
        uint32_t id,  // The id of the rule to search for.
        utf8lex_rule_t ** found_pointer  // Gets set when found.
        )
{
  if (first_rule == NULL
      || found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }


  utf8lex_rule_t *rule = first_rule;
  uint32_t infinite_loop = UTF8LEX_RULES_DB_LENGTH_MAX;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  for (uint32_t d = 0; d < infinite_loop; d ++)
  {
    if (rule->id == id)
    {
      *found_pointer = rule;
      is_infinite_loop = false;
      break;
    }

    if (rule->next == NULL)
    {
      *found_pointer = NULL;
      is_infinite_loop = false;
      break;
    }

    rule = rule->next;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (*found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NOT_FOUND;
  }

  return UTF8LEX_OK;
}
