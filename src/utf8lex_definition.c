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

#include <inttypes.h>  // For uint32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strcmp()

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                        utf8lex_definition_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_definition_find(
        utf8lex_definition_t *first_definition,  // Database to search.
        unsigned char *name,  // Name of definition to search for.
        utf8lex_definition_t ** found_pointer  // Gets set when found.
        )
{
  if (first_definition == NULL
      || name == NULL
      || found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_definition_t *definition = first_definition;
  uint32_t infinite_loop = UTF8LEX_DEFINITIONS_DB_LENGTH_MAX;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  for (uint32_t d = 0; d < infinite_loop; d ++)
  {
    if (strcmp(definition->name, name) == 0)
    {
      *found_pointer = definition;
      is_infinite_loop = false;
      break;
    }

    if (definition->next == NULL)
    {
      *found_pointer = NULL;
      is_infinite_loop = false;
      break;
    }

    definition = definition->next;
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

utf8lex_error_t utf8lex_definition_find_by_id(
        utf8lex_definition_t *first_definition,  // Database to search.
        uint32_t id,  // The id of the definition to search for.
        utf8lex_definition_t ** found_pointer  // Gets set when found.
        )
{
  if (first_definition == NULL
      || found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }


  utf8lex_definition_t *definition = first_definition;
  uint32_t infinite_loop = UTF8LEX_DEFINITIONS_DB_LENGTH_MAX;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  for (uint32_t d = 0; d < infinite_loop; d ++)
  {
    if (definition->id == id)
    {
      *found_pointer = definition;
      is_infinite_loop = false;
      break;
    }

    if (definition->next == NULL)
    {
      *found_pointer = NULL;
      is_infinite_loop = false;
      break;
    }

    definition = definition->next;
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
