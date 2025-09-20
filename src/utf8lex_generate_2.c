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
#include <fcntl.h>  // For open(), unlink()
#include <inttypes.h>  // For uint32_t
#include <stdbool.h>  // For bool, true, false
#include <string.h>  // For strlen(), strcpy(), strcat, strncpy, strcmp()
#include <unistd.h>  // For write()

#include "utf8lex.h"

#include "utf8lex_generate.h"

enum _ENUM_utf8lex_lex_state
{
  UTF8LEX_LEX_STATE_NONE = -1,

  UTF8LEX_LEX_STATE_DEFINITION,
  UTF8LEX_LEX_STATE_DEFINITION_BODY,
  UTF8LEX_LEX_STATE_MULTI_ID,
  UTF8LEX_LEX_STATE_MULTI_ID_SPACE,
  UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID,
  UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE,
  UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR,
  UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS,
  UTF8LEX_LEX_STATE_MULTI_OR,
  UTF8LEX_LEX_STATE_MULTI_OR_ID,
  UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR,
  UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS,
  UTF8LEX_LEX_STATE_LITERAL,
  UTF8LEX_LEX_STATE_LITERAL_BACKSLASH,
  UTF8LEX_LEX_STATE_LITERAL_COMPLETE,
  UTF8LEX_LEX_STATE_REGEX,
  UTF8LEX_LEX_STATE_REGEX_SPACE,
  UTF8LEX_LEX_STATE_RULE,  // DEF1 ... {rule}

  UTF8LEX_LEX_STATE_COMPLETE,  // DEF1 ...\n
  UTF8LEX_LEX_STATE_ERROR,

  UTF8LEX_LEX_STATE_MAX
};

static char utf8lex_lex_state_names[20][32] = {
  "DEFINITION",
  "DEFINITION_BODY",
  "MULTI_ID",
  "MULTI_ID_SPACE",
  "MULTI_SEQUENCE_ID",
  "MULTI_SEQUENCE_ID_SPACE",
  "MULTI_SEQUENCE_ID_STAR",
  "MULTI_SEQUENCE_ID_PLUS",
  "MULTI_OR",
  "MULTI_OR_ID",
  "MULTI_OR_ID_STAR",
  "MULTI_OR_ID_PLUS",
  "LITERAL",
  "LITERAL_BACKSLASH",
  "LITERAL_COMPLETE",
  "REGEX",
  "REGEX_SPACE",
  "RULE",
  "COMPLETE",
  "ERROR"
};

struct _STRUCT_utf8lex_lex_transition
{
  char * from;
  utf8lex_rule_t *rule;
  utf8lex_lex_state_t to;
};

// Removes a definition from the lexicon database, to be replaced
// by a new definition with the same name.
utf8lex_error_t utf8lex_remove_definition(
        utf8lex_generate_lexicon_t *lex,  // .l file lexicon, this file's db.
        unsigned char *name  // name of the definition to remove.
        )
{
  if (lex == NULL
      || lex->db.definitions_db == NULL
      || name == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_definition_t *definition = lex->db.definitions_db;  // First element.
  for (int d = 0;
       d < lex->db.num_definitions;
       d ++)
  {
    if (definition == NULL)
    {
      return UTF8LEX_ERROR_NULL_POINTER;
    }
    else if (d >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
    {
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    if (strcmp(definition->name, name) != 0)
    {
      definition = definition->next;
      continue;
    }

    utf8lex_definition_t *prev = definition->prev;
    if (prev != NULL)
    {
      prev->next = definition->next;
    }

    utf8lex_definition_t *next = definition->next;
    if (next != NULL)
    {
      next->prev = definition->prev;
    }

    // Remove the definition.
    definition->prev = NULL;
    definition->next = NULL;

    d --;
    // Decrement the number of definitions, but leave the definition names.
    // We don't remove names, since they're still referenced by
    // the definitions that have been removed from the linked list.
    lex->db.num_definitions --;
    if (lex->db.last_definition == definition)
    {
      lex->db.last_definition = prev;
    }
    definition = prev;  // Carry on to next when we iterate the loop.
  }

  return UTF8LEX_OK;
}


// Exported:
utf8lex_error_t utf8lex_generate_definition(
        utf8lex_generate_lexicon_t *lex,  // .l file lexicon, this file's db.
        utf8lex_state_t *state,  // .l file current state.
        unsigned char *name,  // Name of (definition or rule).
        bool is_rule  // true = rules section; false = definitions section.
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_definition()");

  if (lex == NULL
      || state == NULL
      || name == NULL)
  {
    fprintf(stderr, "ERROR 11 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_lex_transition_t grammar[UTF8LEX_LEX_STATE_MAX][16] =
    {
      {
        // (0) UTF8LEX_LEX_STATE_DEFINITION:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_DEFINITION],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_DEFINITION_BODY },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_DEFINITION],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_ERROR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_DEFINITION],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_ERROR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_DEFINITION],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (1) UTF8LEX_LEX_STATE_DEFINITION_BODY:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_DEFINITION_BODY],
          .rule = &(lex->id),
          .to = UTF8LEX_LEX_STATE_MULTI_ID },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_DEFINITION_BODY],
          .rule = &(lex->quote),
          .to = UTF8LEX_LEX_STATE_LITERAL },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_DEFINITION_BODY],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_REGEX },  // DEF1 {...regex...
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_DEFINITION_BODY],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_ERROR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_DEFINITION_BODY],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_REGEX }
      },
      {
        // (2) UTF8LEX_LEX_STATE_MULTI_ID:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_ID_SPACE },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID],
          .rule = &(lex->or),
          .to = UTF8LEX_LEX_STATE_MULTI_OR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 REF1 {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 REF1 (pointless but OK)
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (3) UTF8LEX_LEX_STATE_MULTI_ID_SPACE:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID_SPACE],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_ID_SPACE },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID_SPACE],
          .rule = &(lex->id),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID_SPACE],
          .rule = &(lex->or),
          .to = UTF8LEX_LEX_STATE_MULTI_OR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID_SPACE],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 REF1 {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID_SPACE],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 REF1 (pointless but OK)
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_ID_SPACE],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (4) UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID],
          .rule = &(lex->star),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR },  // DEF1 REF1 REF2*
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID],
          .rule = &(lex->plus),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS },  // DEF1 REF1 REF2+
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 REF1 REF2 REF3 {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 REF1 REF2 REF3...
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (5) UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE],
          .rule = &(lex->star),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR },  // DEF1 REF1 REF2*
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE],
          .rule = &(lex->plus),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS },  // DEF1 REF1 REF2+
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE],
          .rule = &(lex->id),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 REF1 REF2 REF3 {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 REF1 REF2 REF3...
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (6) UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR],
          .rule = &(lex->id),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 REF1 REF2 REF3* {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 REF1 REF2 REF3*...
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (7) UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS],
          .rule = &(lex->id),
          .to = UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 REF1 REF2 REF3+ {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 REF1 REF2 REF3+...
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (8) UTF8LEX_LEX_STATE_MULTI_OR:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_OR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR],
          .rule = &(lex->id),
          .to = UTF8LEX_LEX_STATE_MULTI_OR_ID },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_ERROR },  // DEF1 REF1 | {rule} <- unclosed
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_ERROR },  // DEF1 REF1 | <- unclosed
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (9) UTF8LEX_LEX_STATE_MULTI_OR_ID:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_OR_ID },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID],
          .rule = &(lex->star),
          .to = UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR },  // DEF1 REF1 | REF2*
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID],
          .rule = &(lex->plus),
          .to = UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS },  // DEF1 REF1 | REF2+
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID],
          .rule = &(lex->or),
          .to = UTF8LEX_LEX_STATE_MULTI_OR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 REF1 | REF2 {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 REF1 | REF2 ...
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (10) UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR],
          .rule = &(lex->or),
          .to = UTF8LEX_LEX_STATE_MULTI_OR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 REF1 | REF2* {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 REF1 | REF2* ...
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (11) UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS],
          .rule = &(lex->or),
          .to = UTF8LEX_LEX_STATE_MULTI_OR },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 REF1 | REF2+ {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 REF1 | REF2+ ...
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (12) UTF8LEX_LEX_STATE_LITERAL:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL],
          .rule = &(lex->backslash),
          .to = UTF8LEX_LEX_STATE_LITERAL_BACKSLASH },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL],
          .rule = &(lex->quote),
          .to = UTF8LEX_LEX_STATE_LITERAL_COMPLETE },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_LITERAL },  // DEF1 "...{ <- { part of literal
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_ERROR },  // DEF1 "... <- unclosed quote
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_LITERAL }
      },
      {
        // (13) UTF8LEX_LEX_STATE_LITERAL_BACKSLASH:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL_BACKSLASH],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_LITERAL }
      },
      {
        // (14) UTF8LEX_LEX_STATE_LITERAL_COMPLETE:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL_COMPLETE],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_LITERAL_COMPLETE },
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL_COMPLETE],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 "..." {rule}
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL_COMPLETE],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 "..."
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_LITERAL_COMPLETE],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      },
      {
        // (15) UTF8LEX_LEX_STATE_REGEX:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_REGEX],
          .rule = &(lex->space),
          .to = UTF8LEX_LEX_STATE_REGEX_SPACE },  // DEF1 [a-z] \n
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_REGEX],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 ...anything...
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_REGEX],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_REGEX }
      },
      {
        // (16) UTF8LEX_LEX_STATE_REGEX_SPACE:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_REGEX_SPACE],
          .rule = &(lex->rule_open),
          .to = UTF8LEX_LEX_STATE_RULE },  // DEF1 [a-z] (rule)
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_REGEX_SPACE],
          .rule = &(lex->newline),
          .to = UTF8LEX_LEX_STATE_COMPLETE },  // DEF1 [a-z] \n
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_REGEX_SPACE],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_REGEX }  // DEF [a-z] ... (space in regex)
      },
      {
        // (17) UTF8LEX_LEX_STATE_RULE:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_RULE],
          .rule = &(lex->rule_close),
          .to = UTF8LEX_LEX_STATE_RULE },  // Once nested rules -> 0, COMPLETE.
        // Do NOT stop for newline.
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_RULE],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_RULE }
      },
      {
        // (18) UTF8LEX_LEX_STATE_COMPLETE:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_COMPLETE],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_COMPLETE }
      },
      {
        // (19) UTF8LEX_LEX_STATE_ERROR:
        { .from = (char *) utf8lex_lex_state_names[UTF8LEX_LEX_STATE_ERROR],
          .rule = NULL,
          .to = UTF8LEX_LEX_STATE_ERROR }
      }
    };

  // If this is a new definition for a rule, store the name of the definition
  // (which is the same as the rule name).
  int dn = lex->db.num_definition_names;
  if (name != lex->db.definition_names[dn])
  {
    strcpy(lex->db.definition_names[dn],
           name);
  }
  if ((lex->db.num_definition_names + 1) >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
  {
    fprintf(stderr, "ERROR 12 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
    return UTF8LEX_ERROR_MAX_LENGTH;
  }

  utf8lex_error_t error = UTF8LEX_ERROR_STATE;

  // Prepare a multi-definition IN CASE we need it.
  // We'll remove it later if we do not need it.
  int md = lex->db.num_multi_definitions;
  error = utf8lex_multi_definition_init(
              &(lex->db.multi_definitions[md]),  // self
              lex->db.last_definition,  // prev
              lex->db.definition_names[dn],  // name
              NULL,  // parent
              UTF8LEX_MULTI_TYPE_SEQUENCE);  // multi_type
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
    return error;
  }
  lex->db.num_multi_definitions ++;
  if (lex->db.num_multi_definitions >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
  {
    fprintf(stderr, "ERROR 13 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
    return UTF8LEX_ERROR_MAX_LENGTH;
  }

  // Now parse the definition.
  // DEF1 ...definition... (space first).
  utf8lex_lex_state_t lex_state = UTF8LEX_LEX_STATE_DEFINITION;
  if (is_rule == true)
  {
    // Just straight into ...definition, no DEF1 or space.
    lex_state = UTF8LEX_LEX_STATE_DEFINITION_BODY;
  }
  utf8lex_lex_state_t prev_lex_state = lex_state;
  utf8lex_lex_state_t history[16];
  int infinite_loop_protector = 0;
  utf8lex_definition_type_t *definition_type = NULL;
  lex->db.str[dn][0] = '\0';
  unsigned char regex_space[256];
  regex_space[0] = '\0';
  utf8lex_multi_type_t multi_type = UTF8LEX_MULTI_TYPE_NONE;
  utf8lex_reference_t *last_reference = NULL;
  utf8lex_token_t token;
  int num_nested_rules = 0;

  for (int h = 0; h < 16; h ++) {
    history[h] = UTF8LEX_LEX_STATE_NONE;
  }
  history[0] = prev_lex_state;

  lex->db.rule_code[lex->db.num_rules][0] = 0; // No rule code defined yet.
  while (true)
  {
    if (lex_state == UTF8LEX_LEX_STATE_COMPLETE
        || lex_state == UTF8LEX_LEX_STATE_ERROR)
    {
      break;
    }

    infinite_loop_protector ++;
    if (infinite_loop_protector > UTF8LEX_LEX_FILE_NUM_LINES_MAX)
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_generate_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state,
          (size_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex Aborting \"%s\", possible infinite loop [%d.%d]: \"%s\"\n",
              name,
              state->loc[UTF8LEX_UNIT_LINE].start + 1,
              state->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      fprintf(stderr, "ERROR 14 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_INFINITE_LOOP\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    error = utf8lex_lex(lex->lex_rules,
                        state,
                        &token);
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return error;
    }

    utf8lex_lex_state_t next_lex_state = UTF8LEX_LEX_STATE_NONE;
    const int UTF8LEX_LINE_WIDTH_MAX = 65536;
    for (int t = 0; t < UTF8LEX_LINE_WIDTH_MAX; t ++)
    {
      utf8lex_lex_transition_t *transition = &(grammar[lex_state][t]);
      if (transition == NULL)
      {
        fprintf(stderr, "ERROR 15 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
        return UTF8LEX_ERROR_NULL_POINTER;
      }

      if (transition->rule == NULL)
      {
        next_lex_state = transition->to;
        break;
      }
      else if (transition->rule->id == token.rule->id)
      {
        next_lex_state = transition->to;
        break;
      }
      // Else keep stepping through transitions.
    }

    switch (next_lex_state)
    {
    case UTF8LEX_LEX_STATE_LITERAL:
      definition_type = UTF8LEX_DEFINITION_TYPE_LITERAL;
      // Don't capture the quote ("):
      if (lex_state == UTF8LEX_LEX_STATE_LITERAL
          || lex_state == UTF8LEX_LEX_STATE_LITERAL_BACKSLASH)
      {
        error = utf8lex_token_cat_string(
                    &token,  // self
                    lex->db.str[dn],  // str
                    (size_t) UTF8LEX_LITERAL_REGEX_MAX_BYTES);  // max_bytes
        if (error != UTF8LEX_OK)
        {
          UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
          return error;
        }
      }
      break;

    case UTF8LEX_LEX_STATE_REGEX_SPACE:
      // Transitioning to either more REGEX or RULE or COMPLETE.
      // Store this token's text in case we continue with the REGEX.
      error = utf8lex_token_copy_string(
                  &token,  // self
                  regex_space,  // str
                  (size_t) 256);  // max_bytes
      if (error != UTF8LEX_OK)
      {
        UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
        return error;
      }
      break;

    case UTF8LEX_LEX_STATE_REGEX:
      definition_type = UTF8LEX_DEFINITION_TYPE_REGEX;
      if (lex_state == UTF8LEX_LEX_STATE_REGEX_SPACE)
      {
        // Transitioning back from a space to more regex
        // e.g. DEF1 [a-z] [0-9] (space between [a-z] and [0-9]).
        // Add the space to the regular expression pattern.
        strcat(lex->db.str[dn],
               regex_space);
      }
      error = utf8lex_token_cat_string(
                  &token,  // self
                  lex->db.str[dn],  // str
                  (size_t) UTF8LEX_LITERAL_REGEX_MAX_BYTES);  // max_bytes
      if (error != UTF8LEX_OK)
      {
        UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
        return error;
      }
      break;

    case UTF8LEX_LEX_STATE_MULTI_ID:
    case UTF8LEX_LEX_STATE_MULTI_ID_SPACE:
    case UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID:
    case UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_SPACE:
    case UTF8LEX_LEX_STATE_MULTI_OR_ID:
      definition_type = UTF8LEX_DEFINITION_TYPE_MULTI;

      if (next_lex_state == UTF8LEX_LEX_STATE_MULTI_ID
          || next_lex_state == UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID)
      {
        multi_type = UTF8LEX_MULTI_TYPE_SEQUENCE;
      }
      else if (next_lex_state == UTF8LEX_LEX_STATE_MULTI_OR_ID)
      {
        multi_type = UTF8LEX_MULTI_TYPE_OR;
      }

      if (token.rule->id == lex->id.id)
      {
        // Another reference.
        int rd = lex->db.num_references;
        error = utf8lex_token_copy_string(
                    &token,  // self
                    lex->db.reference_names[rd],  // str
                    (size_t) UTF8LEX_NAME_LENGTH_MAX);  // max_bytes
        if (error != UTF8LEX_OK)
        {
          UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
          return error;
        }
        error = utf8lex_reference_init(
                    &(lex->db.references[rd]),  // self
                    last_reference,  // prev
                    lex->db.reference_names[rd],  // name
                    1,  // min
                    1,  // max
                    &(lex->db.multi_definitions[md]));  // parent
        if (error != UTF8LEX_OK)
        {
          UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
          return error;
        }
        lex->db.num_references ++;
        if (lex->db.num_references >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
        {
          fprintf(stderr, "ERROR 16 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
          return UTF8LEX_ERROR_MAX_LENGTH;
        }
        last_reference = &(lex->db.references[rd]);
      }
      break;

    case UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_STAR:
    case UTF8LEX_LEX_STATE_MULTI_OR_ID_STAR:
      // DEF1 REF1 ... REFN*
      // 0 or more instances of the last defined reference to a definition
      // for the new multi-definition.
      last_reference->min = 0;
      last_reference->max = UTF8LEX_REFERENCES_LENGTH_MAX;  // Unbounded.
      break;

    case UTF8LEX_LEX_STATE_MULTI_SEQUENCE_ID_PLUS:
    case UTF8LEX_LEX_STATE_MULTI_OR_ID_PLUS:
      // DEF1 REF1 ... REFN+
      // 1 or more instances of the last defined reference to a definition
      // for the new multi-definition.
      last_reference->min = 1;
      last_reference->max = UTF8LEX_REFERENCES_LENGTH_MAX;  // Unbounded.
      break;

    case UTF8LEX_LEX_STATE_RULE:
      if (is_rule != true)
      {
        // Can't add rule code to a definition in the definitions section.
        fprintf(stderr, "ERROR 17 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_TOKEN\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
        return UTF8LEX_ERROR_TOKEN;
      }

      if (token.rule->id == lex->rule_open.id)
      {
        num_nested_rules ++;
      }
      else if (token.rule->id == lex->rule_close.id)
      {
        num_nested_rules --;
        if (num_nested_rules == 0)
        {
          next_lex_state = UTF8LEX_LEX_STATE_COMPLETE;
          break;
        }
      }

      // Now add the rule code, unless this is the very first "{" rule_open.
      if (lex_state != UTF8LEX_LEX_STATE_RULE)
      {
        // We just started the rule, so its definition is complete.
        // Do not store the rule_open ("{") character as part of the rule,
        // but do finish anything up (e.g. resolve multi definition,
        // if applicable).
        if (definition_type == UTF8LEX_DEFINITION_TYPE_MULTI)
        {
          error = utf8lex_multi_definition_resolve(
                      &(lex->db.multi_definitions[md]),  // self,
                      lex->db.definitions_db);  // db
          if (error == UTF8LEX_ERROR_NOT_FOUND) {
            utf8lex_reference_t *unresolved_reference =
              lex->db.multi_definitions[md].references;
            for (int infinite_loop_protector = 0;
                 infinite_loop_protector < UTF8LEX_REFERENCES_LENGTH_MAX;
                 infinite_loop_protector ++)
            {
              if (unresolved_reference == NULL) {
                break;
              }
              else if (unresolved_reference->definition_or_null == NULL) {
                // This is the unresolved definition.
                break;
              }
              unresolved_reference = unresolved_reference->next;
            }
            if (unresolved_reference != NULL) {
              fprintf(stderr, "ERROR utf8lex No such definition [%d.%d]: %s\n",
                      state->loc[UTF8LEX_UNIT_LINE].start + 1,
                      state->loc[UTF8LEX_UNIT_CHAR].start,
                      unresolved_reference->definition_name);
              UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
              return error;
            }
          }
          if (error != UTF8LEX_OK)
          {
            UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
            return error;
          }
        }
      }
      else
      {
        int rn = lex->db.num_rules;
        error = utf8lex_token_cat_string(
                    &token,  // self
                    lex->db.rule_code[rn],  // str
                    (size_t) UTF8LEX_RULE_CODE_LENGTH_MAX);  // max_bytes
        if (error != UTF8LEX_OK)
        {
          UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
          return error;
        }
      }

      break;
    }

    prev_lex_state = lex_state;
    lex_state = next_lex_state;

    if (history[0] != prev_lex_state) {
      for (int h = 16 - 1; h > 0; h --) {
        history[h] = history[h - 1];
      }
      history[0] = prev_lex_state;
    }
  }

  if (lex_state == UTF8LEX_LEX_STATE_ERROR)
  {
    char history_str[256];
    size_t num_bytes_written = (size_t) 0;
    for (int h = 0;
         h < 16
           && history[h] != UTF8LEX_LEX_STATE_NONE
           && num_bytes_written < (size_t) 256;
         h ++) {
      num_bytes_written += snprintf(history_str + num_bytes_written,
                                    (size_t) 256 - num_bytes_written,
                                    " <-- %s", utf8lex_lex_state_names[history[h]]);
    }
    fprintf(stderr, "ERROR 18 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_STATE%s\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start, history_str);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
    return UTF8LEX_ERROR_STATE;
  }
  else if (lex_state != UTF8LEX_LEX_STATE_COMPLETE)
  {
    char token_str[32];
    token_str[0] = '\0';
    size_t token_length_bytes = (size_t) 29;
    if (token.length_bytes < (size_t) 32) {
      token_length_bytes = token.length_bytes;
    }
    snprintf(token_str,
             token_length_bytes,
             "%s", token.str->bytes + token.start_byte);
    if (token.length_bytes >= (size_t) 32) {
      snprintf(token.str->bytes + token.start_byte + token_length_bytes,
               (size_t) 3,
               "...");
    }
    fprintf(stderr, "ERROR 19 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_UNRESOLVED_DEFINITION: %s\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start, token_str);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
    return UTF8LEX_ERROR_UNRESOLVED_DEFINITION;
  }

  // If we do not need the multi definition, delete it.
  if (definition_type != UTF8LEX_DEFINITION_TYPE_MULTI)
  {
    error = utf8lex_multi_definition_clear(
                (utf8lex_definition_t *)
                &(lex->db.multi_definitions[md])  // self
                );
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return error;
    }
    lex->db.num_multi_definitions --;
    md --;
  }

  // Now store the definition.
  error = UTF8LEX_OK;
  if (definition_type == UTF8LEX_DEFINITION_TYPE_LITERAL)
  {
    int ld = lex->db.num_literal_definitions;
    error = utf8lex_literal_definition_init(
                &(lex->db.literal_definitions[ld]),  // self
                lex->db.last_definition,  // prev
                lex->db.definition_names[dn],  // name
                lex->db.str[dn]);  // str
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return error;
    }
    lex->db.num_literal_definitions ++;
    if (lex->db.num_literal_definitions >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
    {
      fprintf(stderr, "ERROR 21 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    // If a previous definition with the same name already exists, remove it.
    error = utf8lex_remove_definition(lex,
                                      lex->db.literal_definitions[ld].base.name);
    if (error != UTF8LEX_OK)
    {
      fprintf(stderr, "ERROR 21.5 in utf8lex_generate_definition() [%d.%d]: error %d from ut8flex_remove_definition()\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start, error);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return error;
    }
    // Add the definition to the database.
    // Note that if we removed any definitions, the last definition in the
    // database will have changed, but we still need to point to the
    // same definition_names[dn].  So we do not decrement dn.
    lex->db.last_definition = (utf8lex_definition_t *)
      &(lex->db.literal_definitions[ld]);
    lex->db.num_definition_names ++;
    lex->db.num_definitions ++;
  }
  else if (definition_type == UTF8LEX_DEFINITION_TYPE_REGEX)
  {
    int rd = lex->db.num_regex_definitions;
    error = utf8lex_regex_definition_init(
                &(lex->db.regex_definitions[rd]),  // self
                lex->db.last_definition,  // prev
                lex->db.definition_names[dn],  // name
                lex->db.str[dn]);  // pattern
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return error;
    }
    lex->db.num_regex_definitions ++;
    if (lex->db.num_regex_definitions >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
    {
      fprintf(stderr, "ERROR 22 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    // If a previous definition with the same name already exists, remove it.
    error = utf8lex_remove_definition(lex,
                                      lex->db.regex_definitions[rd].base.name);
    if (error != UTF8LEX_OK)
    {
      fprintf(stderr, "ERROR 22.5 in utf8lex_generate_definition() [%d.%d]: error %d from ut8flex_remove_definition()\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start, error);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return error;
    }
    // Add the definition to the database.
    // Note that if we removed any definitions, the last definition in the
    // database will have changed, but we still need to point to the
    // same definition_names[dn].  So we do not decrement dn.
    lex->db.last_definition = (utf8lex_definition_t *)
      &(lex->db.regex_definitions[rd]);
    lex->db.num_definition_names ++;
    lex->db.num_definitions ++;
  }
  else if (definition_type == UTF8LEX_DEFINITION_TYPE_MULTI)
  {
    lex->db.multi_definitions[md].multi_type = multi_type;

    // If a previous definition with the same name already exists, remove it.
    // In this case, we'll inadvertently also remove our own multi-definition
    // that we added in advance.  So we'll have to add it back again.
    error = utf8lex_remove_definition(lex,
                                      lex->db.multi_definitions[md].base.name);
    if (error != UTF8LEX_OK)
    {
      fprintf(stderr, "ERROR 20.5 in utf8lex_generate_definition() [%d.%d]: error %d from ut8flex_remove_definition()\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start, error);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
      return error;
    }
    // Add the definition to the database.
    // Note that if we removed any definitions, the last definition in the
    // database will have changed, but we still need to point to the
    // same definition_names[dn].  So we do not decrement dn.
    lex->db.last_definition = (utf8lex_definition_t *)
      &(lex->db.multi_definitions[md]);
    lex->db.num_definition_names ++;
    lex->db.num_definitions ++;
  }
  else
  {
    // Incomplete parse or something.
    fprintf(stderr, "ERROR 23 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_STATE\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
    return UTF8LEX_ERROR_STATE;
  }

  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
    return error;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_generate_definition()");
  return UTF8LEX_OK;
}
