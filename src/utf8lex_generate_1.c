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
#include <string.h>  // For strlen(), strcpy(), strcat, strncpy
#include <unistd.h>  // For write()

#include "utf8lex.h"

#include "utf8lex_generate.h"


static utf8lex_error_t utf8lex_generate_init_db(
        utf8lex_db_t *db
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_init_db()");

  if (db == NULL)
  {
    fprintf(stderr, "ERROR 1 in utf8lex_generate_init_db() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init_db()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Definitions and rules from the .l file (not including pre-defined
  // cat definitions):
  db->num_definitions = (uint32_t) 0;
  db->num_literal_definitions = (uint32_t) 0;
  db->num_regex_definitions = (uint32_t) 0;
  db->num_multi_definitions = (uint32_t) 0;
  db->num_references = (uint32_t) 0;

  db->num_rules = (uint32_t) 0;

  // Whole database:
  db->definitions_db = NULL;
  db->rules_db = NULL;

  db->last_definition = NULL;
  db->last_rule = NULL;

  // Go through only the explicitly defined categories, don't define
  // new ones here (like X | Y):
  for (int c = 0; c < UTF8LEX_NUM_CATEGORIES; c ++)
  {
    utf8lex_cat_t cat = UTF8LEX_CATEGORIES[c];

    // definition name = "LETTER_LOWER", "NUM", "HSPACE", and so on.
    utf8lex_error_t error = utf8lex_format_cat(cat,
                                               db->cat_names[c]);
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_init_db()");
      return error;
    }

    // LETTER_LOWER or NUM etc definition (can be referenced or overridden):
    error = utf8lex_cat_definition_init(
                &(db->cat_definitions[c]),  // self
                db->last_definition,  // pref
                db->cat_names[c],  // name
                cat,  // cat
                1,  // min
                1);  // max
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_init_db()");
      return error;
    }

    db->last_definition = (utf8lex_definition_t *)
      &(db->cat_definitions[c]);
    if (db->definitions_db == NULL)
    {
      // First definition in the database.
      db->definitions_db = db->last_definition;
    }
    db->num_definitions ++;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_generate_init_db()");
  return UTF8LEX_OK;
}

static utf8lex_error_t utf8lex_generate_clear_db(
        utf8lex_db_t *db
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_clear_db()");

  if (db == NULL)
  {
    fprintf(stderr, "ERROR 1 in utf8lex_generate_clear_db() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear_db()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error;

  // Go through only the explicitly defined categories.
  for (int c = 0; c < UTF8LEX_NUM_CATEGORIES; c ++)
  {
    utf8lex_cat_t cat = UTF8LEX_CATEGORIES[c];
    error = utf8lex_cat_definition_clear(&(db->cat_definitions[c].base));
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_clear_db()");
      return error;
    }
  }

  // Definitions and rules from the .l file (not including pre-defined
  // cat definitions):
  for (uint32_t ld = (uint32_t) 0; ld < db->num_literal_definitions; ld ++)
  {
    error = utf8lex_literal_definition_clear(&(db->literal_definitions[ld].base));
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_clear_db()");
      return error;
    }
  }
  for (uint32_t rd = (uint32_t) 0; rd < db->num_regex_definitions; rd ++)
  {
    error = utf8lex_regex_definition_clear(&(db->regex_definitions[rd].base));
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_clear_db()");
      return error;
    }
  }
  for (uint32_t md = (uint32_t) 0; md < db->num_multi_definitions; md ++)
  {
    error = utf8lex_multi_definition_clear(&(db->multi_definitions[md].base));
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_clear_db()");
      return error;
    }
  }
  for (uint32_t r = (uint32_t) 0; r < db->num_references; r ++)
  {
    error = utf8lex_reference_clear(&(db->references[r]));
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_clear_db()");
      return error;
    }
  }
  for (uint32_t r = (uint32_t) 0; r < db->num_rules; r ++)
  {
    error = utf8lex_rule_clear(&(db->rules[r]));
    if (error != UTF8LEX_OK)
    {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_clear_db()");
      return error;
    }
  }

  db->num_definitions = (uint32_t) 0;
  db->num_literal_definitions = (uint32_t) 0;
  db->num_regex_definitions = (uint32_t) 0;
  db->num_multi_definitions = (uint32_t) 0;
  db->num_references = (uint32_t) 0;

  db->num_rules = (uint32_t) 0;

  // Whole database:
  db->definitions_db = NULL;
  db->rules_db = NULL;

  db->last_definition = NULL;
  db->last_rule = NULL;

  UTF8LEX_DEBUG("EXIT utf8lex_generate_clear_db()");
  return UTF8LEX_OK;
}

// Exported:
utf8lex_error_t utf8lex_generate_init(
        utf8lex_generate_lexicon_t *lex
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_init()");

  if (lex == NULL)
  {
    fprintf(stderr, "ERROR 2 in utf8lex_generate_init() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_definition_t *prev_definition = NULL;
  utf8lex_rule_t *prev = NULL;
  utf8lex_error_t error = UTF8LEX_OK;

  // Newline
  error = utf8lex_cat_definition_init(&(lex->newline_definition),
                                      prev_definition,  // prev
                                      "NEWLINE",  // name
                                      UTF8LEX_CAT_SEP_LINE  // cat
                                      | UTF8LEX_CAT_SEP_PARAGRAPH
                                      | UTF8LEX_EXT_SEP_LINE,
                                      1,  // min
                                      -1);  // max
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->newline_definition);
  lex->lex_definitions = prev_definition;
  error = utf8lex_rule_init(&(lex->newline),
                            prev,
                            "newline",  // name
                            (utf8lex_definition_t *)
                            &(lex->newline_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->newline);
  lex->lex_rules = prev;

  // %%
  error = utf8lex_literal_definition_init(&(lex->section_divider_definition),
                                          prev_definition,  // prev
                                          "SECTION_DIVIDER",  // name
                                          "%%");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->section_divider_definition);
  error = utf8lex_rule_init(&(lex->section_divider),
                            prev,
                            "section_divider",  // name
                            (utf8lex_definition_t *)  // definition
                            &(lex->section_divider_definition),
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->section_divider);
  // %{
  error = utf8lex_literal_definition_init(&(lex->enclosed_open_definition),
                                          prev_definition,  // prev
                                          "ENCLOSED_OPEN",  // name
                                          "%{");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->enclosed_open_definition);
  error = utf8lex_rule_init(&(lex->enclosed_open),
                            prev,
                            "enclosed_open",  // name
                            (utf8lex_definition_t *)  // definition
                            &(lex->enclosed_open_definition),
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->enclosed_open);
  // %<}
  error = utf8lex_literal_definition_init(&(lex->enclosed_close_definition),
                                          prev_definition,  // prev
                                          "ENCLOSED_CLOSE",  // name
                                          "%}");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->enclosed_close_definition);
  error = utf8lex_rule_init(&(lex->enclosed_close),
                            prev,
                            "enclosed_close",  // name
                            (utf8lex_definition_t *)  // definition
                            &(lex->enclosed_close_definition),
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->enclosed_close);
  // "
  error = utf8lex_literal_definition_init(&(lex->quote_definition),
                                          prev_definition,  // prev
                                          "QUOTE",  // name
                                          "\"");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->quote_definition);
  error = utf8lex_rule_init(&(lex->quote),
                            prev,
                            "quote",  // name
                            (utf8lex_definition_t *)
                            &(lex->quote_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->quote);
  // |
  error = utf8lex_literal_definition_init(&(lex->or_definition),
                                          prev_definition,  // prev
                                          "OR",  // name
                                          "|");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->or_definition);
  error = utf8lex_rule_init(&(lex->or),
                            prev,
                            "or",  // name
                            (utf8lex_definition_t *)
                            &(lex->or_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->or);
  // {
  error = utf8lex_literal_definition_init(&(lex->rule_open_definition),
                                          prev_definition,  // prev
                                          "RULE_OPEN",  // name
                                          "{");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->rule_open_definition);
  error = utf8lex_rule_init(&(lex->rule_open),
                            prev,
                            "rule_open",  // name
                            (utf8lex_definition_t *)
                            &(lex->rule_open_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->rule_open);
  // }
  error = utf8lex_literal_definition_init(&(lex->rule_close_definition),
                                          prev_definition,  // prev
                                          "RULE_CLOSE",  // name
                                          "}");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->rule_close_definition);
  error = utf8lex_rule_init(&(lex->rule_close),
                            prev,
                            "rule_close",  // name
                            (utf8lex_definition_t *)
                            &(lex->rule_close_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->rule_close);
  // *
  error = utf8lex_literal_definition_init(&(lex->star_definition),
                                          prev_definition,  // prev
                                          "STAR",  // name
                                          "*");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->star_definition);
  error = utf8lex_rule_init(&(lex->star),
                            prev,
                            "star",  // name
                            (utf8lex_definition_t *)
                            &(lex->star_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->star);
  // +
  error = utf8lex_literal_definition_init(&(lex->plus_definition),
                                          prev_definition,  // prev
                                          "PLUS",  // name
                                          "+");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->plus_definition);
  error = utf8lex_rule_init(&(lex->plus),
                            prev,
                            "plus",  // name
                            (utf8lex_definition_t *)
                            &(lex->plus_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->plus);
  // (backslash)
  error = utf8lex_literal_definition_init(&(lex->backslash_definition),
                                          prev_definition,  // prev
                                          "BACKSLASH",  // name
                                          "\\");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->backslash_definition);
  error = utf8lex_rule_init(&(lex->backslash),
                            prev,
                            "backslash",  // name
                            (utf8lex_definition_t *)
                            &(lex->backslash_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->backslash);

  // definition ID
  error = utf8lex_regex_definition_init(&(lex->id_definition),
                                        prev_definition,  // prev
                                        "ID",  // name
                                        "[_\\p{L}][_\\p{L}\\p{N}]*");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->id_definition);
  error = utf8lex_rule_init(&(lex->id),
                            prev,
                            "id",  // name
                            (utf8lex_definition_t *)
                            &(lex->id_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->id);

  // horizontal whitespace
  error = utf8lex_regex_definition_init(&(lex->space_definition),
                                        prev_definition,  // prev
                                        "SPACE",  // name
                                        "[\\h]+");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->space_definition);
  error = utf8lex_rule_init(&(lex->space),
                            prev,
                            "space",  // name
                            (utf8lex_definition_t *)
                            &(lex->space_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->space);

  // anything EXCEPT backslash.
  error = utf8lex_regex_definition_init(&(lex->not_backslash_definition),
                                        prev_definition,  // prev
                                        "NOT_BACKSLASH",  // name
                                        "[^\\\\]");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->not_backslash_definition);
  error = utf8lex_rule_init(&(lex->not_backslash),
                            prev,
                            "not_backslash",  // name
                            (utf8lex_definition_t *)
                            &(lex->not_backslash_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->not_backslash);

  // Any (regex .)
  error = utf8lex_regex_definition_init(&(lex->any_definition),
                                        prev_definition,  // prev
                                        "ANY",  // name
                                        ".");
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->any_definition);
  error = utf8lex_rule_init(&(lex->any),
                            prev,
                            "any",  // name
                            (utf8lex_definition_t *)
                            &(lex->any_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev = &(lex->any);


  // Everything to the end of the line
  error = utf8lex_cat_definition_init(&(lex->to_eol_definition),
                                      prev_definition,  // prev
                                      "TO_EOL",  // name
                                      UTF8LEX_GROUP_NOT_VSPACE,  // cat
                                      1,  // min
                                      -1);  // max
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(lex->to_eol_definition);
  error = utf8lex_rule_init(&(lex->to_eol),
                            NULL,  // Do not link to other rules.
                            "to_eol",  // name
                            (utf8lex_definition_t *)
                            &(lex->to_eol_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }

  error = utf8lex_generate_init_db(&(lex->db));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
    return error;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_generate_init()");
  return UTF8LEX_OK;
}

// Exported:
utf8lex_error_t utf8lex_generate_clear(
        utf8lex_generate_lexicon_t *lex
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_clear()");

  if (lex == NULL)
  {
    fprintf(stderr, "ERROR in utf8lex_generate_clear() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;

  // Newline
  error = utf8lex_cat_definition_clear(&(lex->newline_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->newline));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // %%
  error = utf8lex_literal_definition_clear(&(lex->section_divider_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->section_divider));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // %{
  error = utf8lex_literal_definition_clear(&(lex->enclosed_open_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->enclosed_open));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // %<}
  error = utf8lex_literal_definition_clear(&(lex->enclosed_close_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->enclosed_close));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // "
  error = utf8lex_literal_definition_clear(&(lex->quote_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->quote));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // |
  error = utf8lex_literal_definition_clear(&(lex->or_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->or));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // {
  error = utf8lex_literal_definition_clear(&(lex->rule_open_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->rule_open));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // }
  error = utf8lex_literal_definition_clear(&(lex->rule_close_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->rule_close));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // *
  error = utf8lex_literal_definition_clear(&(lex->star_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->star));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // +
  error = utf8lex_literal_definition_clear(&(lex->plus_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->plus));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  // (backslash)
  error = utf8lex_literal_definition_clear(&(lex->backslash_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->backslash));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }

  // definition ID
  error = utf8lex_regex_definition_clear(&(lex->id_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->id));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }

  // horizontal whitespace
  error = utf8lex_regex_definition_clear(&(lex->space_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->space));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }

  // anything EXCEPT backslash.
  error = utf8lex_regex_definition_clear(&(lex->not_backslash_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->not_backslash));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }

  // Any (regex .)
  error = utf8lex_regex_definition_clear(&(lex->any_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->any));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }


  // Everything to the end of the line
  error = utf8lex_cat_definition_clear(&(lex->to_eol_definition.base));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }
  error = utf8lex_rule_clear(&(lex->to_eol));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }

  error = utf8lex_generate_clear_db(&(lex->db));
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
    return error;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_generate_clear()");
  return UTF8LEX_OK;
}


// Exported:
utf8lex_error_t utf8lex_generate_fill_some_of_remaining_buffer(
        unsigned char *some_of_remaining_buffer,
        utf8lex_state_t *state,
        size_t buffer_bytes,
        size_t max_bytes
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_fill_some_of_remaining_buffer()");

  if (some_of_remaining_buffer == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->loc == NULL)
  {
    fprintf(stderr, "ERROR 3 in utf8lex_generate_fill_some_of_remaining_buffer() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_fill_some_of_remaining_buffer()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (buffer_bytes <= (size_t) 0
           || max_bytes <= (size_t) 0)
  {
    fprintf(stderr, "ERROR 4 in utf8lex_generate_fill_some_of_remaining_buffer() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_fill_some_of_remaining_buffer()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  off_t start_byte = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;

  size_t num_bytes;
  if (buffer_bytes < max_bytes)
  {
    num_bytes = buffer_bytes;
  }
  else
  {
    num_bytes = max_bytes - 1;
  }
  strncpy(some_of_remaining_buffer,
          &(state->buffer->str->bytes[start_byte]),
          num_bytes);
  some_of_remaining_buffer[num_bytes] = 0;

  bool is_first_eof = true;
  off_t c = (off_t) 0;
  for (c = (off_t) 0; c <= (off_t) (max_bytes - 4); c ++)
  {
    if (some_of_remaining_buffer[c] == 0)
    {
      break;
    }
    else if (some_of_remaining_buffer[c] == '\r'
             || some_of_remaining_buffer[c] == '\n')
    {
      if (is_first_eof == false
          || c >= (max_bytes - 6))
      {
        some_of_remaining_buffer[c] = 0;
      }
      else
      {
        // We have enough room to shift everything,
        // and insert an extra character, so we'll put in
        // "\\r" or "\\n".
        for (int d = num_bytes; d > c; d --)
        {
          some_of_remaining_buffer[d] = some_of_remaining_buffer[d - 1];
        }
        if (some_of_remaining_buffer[c] == '\r')
        {
          some_of_remaining_buffer[c + 1] = 'r';
        }
        else if (some_of_remaining_buffer[c] == '\n')
        {
          some_of_remaining_buffer[c + 1] = 'n';
        }
        else
        {
          some_of_remaining_buffer[c + 1] = '?';
        }
        some_of_remaining_buffer[c] = '\\';
        if ((num_bytes + 1) < max_bytes)
        {
          some_of_remaining_buffer[num_bytes + 1] = 0;
        }
        else
        {
          some_of_remaining_buffer[num_bytes] = 0;
        }
        is_first_eof = false;
        c += 2;
      }
    }
  }
  if (c >= (off_t) max_bytes)
  {
    some_of_remaining_buffer[max_bytes - 4] = '.';
    some_of_remaining_buffer[max_bytes - 3] = '.';
    some_of_remaining_buffer[max_bytes - 2] = '.';
    some_of_remaining_buffer[max_bytes - 1] = 0;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_generate_fill_some_of_remaining_buffer()");
  return UTF8LEX_OK;
}

// Exported:
utf8lex_error_t utf8lex_generate_token_error(
        utf8lex_state_t *state,
        utf8lex_token_t *token,
        unsigned char *message
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_token_error()");

  if (state == NULL
      || state->buffer == NULL
      || state->buffer->loc == NULL
      || state->loc == NULL
      || token == NULL
      || token->rule == NULL
      || token->rule->name == NULL
      || message == NULL)
  {
    fprintf(stderr, "ERROR 5 in utf8lex_generate_token_error() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_token_error()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  unsigned char some_of_remaining_buffer[32];
  utf8lex_generate_fill_some_of_remaining_buffer(some_of_remaining_buffer,
                                        state,
                                        (size_t) token->length_bytes,
                                        (size_t) 32);  // max_bytes
  fprintf(stderr,
          "ERROR utf8lex [%d.%d]: %s %s [#%d] \"%s\"\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start,
          message,
          token->rule->name,
          token->rule->id,
          some_of_remaining_buffer);

  fprintf(stderr, "ERROR 6 in utf8lex_generate_token_error() [%d.%d]: UTF8LEX_ERROR_TOKEN\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
  UTF8LEX_DEBUG("EXIT utf8lex_generate_token_error()");
  return UTF8LEX_ERROR_TOKEN;
}

static utf8lex_error_t utf8lex_generate_read_to_eol(
        utf8lex_generate_lexicon_t *lex,
        utf8lex_state_t *state,
        utf8lex_token_t *line_token_pointer,
        utf8lex_token_t *newline_token_pointer
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_read_to_eol()");

  if (lex == NULL
      || state == NULL
      || line_token_pointer == NULL
      || newline_token_pointer == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_read_to_eol()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Trace pre.
  if (state->settings.is_tracing == true)
  {
    utf8lex_trace_pre("Read to EOL", state);
  }

  bool is_empty_to_eol = false;
  utf8lex_error_t error = utf8lex_lex(&(lex->to_eol),
                                      state,
                                      line_token_pointer);
  if (error == UTF8LEX_EOF)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_read_to_eol()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_post("Read to EOL", state, error);
    }

    return error;
  }
  else if (error == UTF8LEX_NO_MATCH)
  {
    is_empty_to_eol = true;
  }
  else if (error != UTF8LEX_OK)
  {
    // Error.
    unsigned char some_of_remaining_buffer[32];
    utf8lex_generate_fill_some_of_remaining_buffer(
        some_of_remaining_buffer,
        state,
        (size_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].length,
        (size_t) 32);
    fprintf(stderr,
            "ERROR utf8lex Failed to read to EOL [%d.%d]: \"%s\"\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start, some_of_remaining_buffer);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_read_to_eol()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_post("Read to EOL", state, error);
    }

    return error;
  }

  // Read in the newline.
  error = utf8lex_lex(&(lex->newline),
                      state,
                      newline_token_pointer);
  if (error == UTF8LEX_EOF)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_read_to_eol()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_post("Read to EOL", state, error);
    }

    return error;
  }
  else if (error != UTF8LEX_OK)
  {
    // Error.
    unsigned char some_of_remaining_buffer[32];
    utf8lex_generate_fill_some_of_remaining_buffer(
        some_of_remaining_buffer,
        state,
        (size_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].length,
        (size_t) 32);
    fprintf(stderr,
            "ERROR utf8lex Failed to read newline [%d.%d]: \"%s\"\n",
            state->loc[UTF8LEX_UNIT_LINE].start + 1,
            state->loc[UTF8LEX_UNIT_CHAR].start,
            some_of_remaining_buffer);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_read_to_eol()");
    // Trace post.
    if (state->settings.is_tracing == true)
    {
      utf8lex_trace_post("Read to EOL", state, error);
    }

    return error;
  }

  if (is_empty_to_eol == true)
  {
    // Hack Kludge Janky
    // We can't really generate an empty token, so we fake it here.
    line_token_pointer->rule = &(lex->to_eol);
    line_token_pointer->start_byte = newline_token_pointer->start_byte;
    line_token_pointer->length_bytes = (int) 0;
    line_token_pointer->str = newline_token_pointer->str;
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      line_token_pointer->loc[unit].start =
        newline_token_pointer->loc[unit].start;
      line_token_pointer->loc[unit].length = (int) 0;
      line_token_pointer->loc[unit].after = (int) -1;
    }
  }

  UTF8LEX_DEBUG("EXIT utf8lex_generate_read_to_eol()");
  // Trace post.
  if (state->settings.is_tracing == true)
  {
    utf8lex_trace_post("Read to EOL", state, UTF8LEX_OK);
  }

  return UTF8LEX_OK;
}

// Exported:
utf8lex_error_t utf8lex_generate_write_line(
        int fd_out,
        utf8lex_generate_lexicon_t *lex,
        utf8lex_state_t *state
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_write_line()");

  if (lex == NULL
      || state == NULL)
  {
    fprintf(stderr, "ERROR 7 in utf8lex_generate_write_line() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_line()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    fprintf(stderr, "ERROR 8 in utf8lex_generate_write_line() [%d.%d]: UTF8LEX_ERROR_FILE_DESCRIPTOR\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_line()");
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  utf8lex_token_t line_token;
  utf8lex_token_t newline_token;
  utf8lex_error_t error = UTF8LEX_OK;
  size_t bytes_written;

  // Read the rest of the current line as a single token:
  error = utf8lex_generate_read_to_eol(lex,
                                       state,
                                       &line_token,
                                       &newline_token);
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_line()");
    return error;
  }

  // Write out the rest of the line:
  bytes_written = write(fd_out,
                        &(line_token.str->bytes[line_token.start_byte]),
                        line_token.length_bytes);
  if (bytes_written != line_token.length_bytes)
  {
    fprintf(stderr, "ERROR 9 in utf8lex_generate_write_line() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_line()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  // Write out the newline:
  bytes_written = write(fd_out,
                        &(newline_token.str->bytes[newline_token.start_byte]),
                        newline_token.length_bytes);
  if (bytes_written != newline_token.length_bytes)
  {
    fprintf(stderr, "ERROR 10 in utf8lex_generate_write_line() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_line()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_generate_write_line()");
  return UTF8LEX_OK;
}
