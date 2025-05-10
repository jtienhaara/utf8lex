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
#include <fcntl.h>  // For open(), unlink()
#include <inttypes.h>  // For uint32_t
#include <stdbool.h>  // For bool, true, false
#include <string.h>  // For strlen(), strcpy(), strcat, strncpy
#include <unistd.h>  // For write()

#include "utf8lex.h"


typedef struct _STRUCT_utf8lex_db               utf8lex_db_t;
typedef struct _STRUCT_utf8lex_generate_lexicon utf8lex_generate_lexicon_t;
typedef enum _ENUM_utf8lex_lex_state            utf8lex_lex_state_t;
typedef struct _STRUCT_utf8lex_lex_transition   utf8lex_lex_transition_t;


// Arbitrary maximum number of lines in a .l file, used for infinite loop
// prevention, so that we don't read forever and ever and ever and...
// (That's a lot of lines of lex code.  I'm sure someone has done it, though...)
#define UTF8LEX_LEX_FILE_NUM_LINES_MAX 65536

// Arbitrary maximum definition / rule name length, so that we don't
// have to malloc:
// WARNING Setting this too high leads to segfaults at static function start.
#define UTF8LEX_NAME_LENGTH_MAX 64

// Arbitrary maximum rule code length, so that we don't have to malloc:
// WARNING Setting this too high leads to segfaults at static function start.
#define UTF8LEX_RULE_CODE_LENGTH_MAX 1024

// Maximum length of a literal string or regex pattern.
#define UTF8LEX_LITERAL_REGEX_MAX_BYTES 256

struct _STRUCT_utf8lex_db
{
  // Pre-defined definitions (which can be overridden in the .l file):
  // Literals matching the category names
  // LETTER, LETTER_LOWER, NUM, HSPACE, VSPACE, and so on:
  unsigned char cat_names[UTF8LEX_NUM_CATEGORIES][UTF8LEX_CAT_FORMAT_MAX_LENGTH];
  utf8lex_cat_definition_t cat_definitions[UTF8LEX_NUM_CATEGORIES];
  utf8lex_rule_t cats[UTF8LEX_NUM_CATEGORIES];

  // Definitions from the .l file (NOT in any order -- use definitions_db).
  // Note that the first UTF8LEX_NUM_CATEGORIES definitions are pre-defined,
  // not from the .l file.
  uint32_t num_definitions;
  unsigned char definition_names[UTF8LEX_DEFINITIONS_DB_LENGTH_MAX][UTF8LEX_NAME_LENGTH_MAX];
  // For literals and regexes:
  unsigned char str[UTF8LEX_DEFINITIONS_DB_LENGTH_MAX][UTF8LEX_LITERAL_REGEX_MAX_BYTES];

  uint32_t num_literal_definitions;
  utf8lex_literal_definition_t literal_definitions[UTF8LEX_DEFINITIONS_DB_LENGTH_MAX];
  uint32_t num_regex_definitions;
  utf8lex_regex_definition_t regex_definitions[UTF8LEX_DEFINITIONS_DB_LENGTH_MAX];
  uint32_t num_multi_definitions;
  utf8lex_multi_definition_t multi_definitions[UTF8LEX_DEFINITIONS_DB_LENGTH_MAX];
  uint32_t num_references;
  unsigned char reference_names[UTF8LEX_DEFINITIONS_DB_LENGTH_MAX][UTF8LEX_NAME_LENGTH_MAX];
  utf8lex_reference_t references[UTF8LEX_DEFINITIONS_DB_LENGTH_MAX];

  // Rules from the .l file:
  uint32_t num_rules;
  unsigned char rule_names[UTF8LEX_RULES_DB_LENGTH_MAX][UTF8LEX_NAME_LENGTH_MAX];
  unsigned char rule_code[UTF8LEX_RULES_DB_LENGTH_MAX][UTF8LEX_RULE_CODE_LENGTH_MAX];
  utf8lex_rule_t rules[UTF8LEX_RULES_DB_LENGTH_MAX];

  // The databases of definitions and rules:
  utf8lex_definition_t *definitions_db;  // All pre-defined and .l definitions.
  utf8lex_rule_t *rules_db;  // All .l file rules.  (Points to rules[0].)

  // Where to append new definitions and rules (possibly also deleting
  // existing ones, if the names overlap!).
  utf8lex_definition_t *last_definition;
  utf8lex_rule_t *last_rule;
};

struct _STRUCT_utf8lex_generate_lexicon
{
  utf8lex_definition_t *lex_definitions;
  utf8lex_rule_t *lex_rules;

  // Newline
  utf8lex_cat_definition_t newline_definition;
  utf8lex_rule_t newline;

  // %%
  utf8lex_literal_definition_t section_divider_definition;
  utf8lex_rule_t section_divider;
  // %{
  utf8lex_literal_definition_t enclosed_open_definition;
  utf8lex_rule_t enclosed_open;
  // %}
  utf8lex_literal_definition_t enclosed_close_definition;
  utf8lex_rule_t enclosed_close;
  // "
  utf8lex_literal_definition_t quote_definition;
  utf8lex_rule_t quote;
  // |
  utf8lex_literal_definition_t or_definition;
  utf8lex_rule_t or;
  // {
  utf8lex_literal_definition_t rule_open_definition;
  utf8lex_rule_t rule_open;
  // }
  utf8lex_literal_definition_t rule_close_definition;
  utf8lex_rule_t rule_close;
  // *
  utf8lex_literal_definition_t star_definition;
  utf8lex_rule_t star;
  // +
  utf8lex_literal_definition_t plus_definition;
  utf8lex_rule_t plus;
  // (backslash)
  utf8lex_literal_definition_t backslash_definition;
  utf8lex_rule_t backslash;

  // ID
  utf8lex_regex_definition_t id_definition;
  utf8lex_rule_t id;

  // horizontal whitespace
  utf8lex_regex_definition_t space_definition;
  utf8lex_rule_t space;

  // anything EXCEPT backslash.
  utf8lex_regex_definition_t not_backslash_definition;
  utf8lex_rule_t not_backslash;

  // Any (regex .)
  utf8lex_regex_definition_t any_definition;
  utf8lex_rule_t any;

  // code
  utf8lex_regex_definition_t code_definition;
  utf8lex_rule_t code;

  // read to end of line
  utf8lex_cat_definition_t to_eol_definition;
  utf8lex_rule_t to_eol;

  // Database of definitions and rules in a specific .l file,
  // which comes with pre-defined definitions for the categories:
  utf8lex_db_t db;
};


static utf8lex_error_t utf8lex_generate_setup_db(
        utf8lex_db_t *db
        )
{
  if (db == NULL)
  {
    fprintf(stderr, "ERROR 1 in utf8lex_generate_setup_db() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
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
    if (error != UTF8LEX_OK) { return error; }

    db->last_definition = (utf8lex_definition_t *)
      &(db->cat_definitions[c]);
    if (db->definitions_db == NULL)
    {
      // First definition in the database.
      db->definitions_db = db->last_definition;
    }
    db->num_definitions ++;
  }

  return UTF8LEX_OK;
}

static utf8lex_error_t utf8lex_generate_setup(
        utf8lex_generate_lexicon_t *lex
        )
{
  if (lex == NULL)
  {
    fprintf(stderr, "ERROR 2 in utf8lex_generate_setup() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
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
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->newline_definition);
  lex->lex_definitions = prev_definition;
  error = utf8lex_rule_init(&(lex->newline),
                            prev,
                            "newline",  // name
                            (utf8lex_definition_t *)
                            &(lex->newline_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->newline);
  lex->lex_rules = prev;

  // %%
  error = utf8lex_literal_definition_init(&(lex->section_divider_definition),
                                          prev_definition,  // prev
                                          "SECTION_DIVIDER",  // name
                                          "%%");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->section_divider_definition);
  error = utf8lex_rule_init(&(lex->section_divider),
                            prev,
                            "section_divider",  // name
                            (utf8lex_definition_t *)  // definition
                            &(lex->section_divider_definition),
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->section_divider);
  // %{
  error = utf8lex_literal_definition_init(&(lex->enclosed_open_definition),
                                          prev_definition,  // prev
                                          "ENCLOSED_OPEN",  // name
                                          "%{");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->enclosed_open_definition);
  error = utf8lex_rule_init(&(lex->enclosed_open),
                            prev,
                            "enclosed_open",  // name
                            (utf8lex_definition_t *)  // definition
                            &(lex->enclosed_open_definition),
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->enclosed_open);
  // %<}
  error = utf8lex_literal_definition_init(&(lex->enclosed_close_definition),
                                          prev_definition,  // prev
                                          "ENCLOSED_CLOSE",  // name
                                          "%}");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->enclosed_close_definition);
  error = utf8lex_rule_init(&(lex->enclosed_close),
                            prev,
                            "enclosed_close",  // name
                            (utf8lex_definition_t *)  // definition
                            &(lex->enclosed_close_definition),
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->enclosed_close);
  // "
  error = utf8lex_literal_definition_init(&(lex->quote_definition),
                                          prev_definition,  // prev
                                          "QUOTE",  // name
                                          "\"");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->quote_definition);
  error = utf8lex_rule_init(&(lex->quote),
                            prev,
                            "quote",  // name
                            (utf8lex_definition_t *)
                            &(lex->quote_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->quote);
  // |
  error = utf8lex_literal_definition_init(&(lex->or_definition),
                                          prev_definition,  // prev
                                          "OR",  // name
                                          "|");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->or_definition);
  error = utf8lex_rule_init(&(lex->or),
                            prev,
                            "or",  // name
                            (utf8lex_definition_t *)
                            &(lex->or_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->or);
  // {
  error = utf8lex_literal_definition_init(&(lex->rule_open_definition),
                                          prev_definition,  // prev
                                          "RULE_OPEN",  // name
                                          "{");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->rule_open_definition);
  error = utf8lex_rule_init(&(lex->rule_open),
                            prev,
                            "rule_open",  // name
                            (utf8lex_definition_t *)
                            &(lex->rule_open_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->rule_open);
  // }
  error = utf8lex_literal_definition_init(&(lex->rule_close_definition),
                                          prev_definition,  // prev
                                          "RULE_CLOSE",  // name
                                          "}");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->rule_close_definition);
  error = utf8lex_rule_init(&(lex->rule_close),
                            prev,
                            "rule_close",  // name
                            (utf8lex_definition_t *)
                            &(lex->rule_close_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->rule_close);
  // *
  error = utf8lex_literal_definition_init(&(lex->star_definition),
                                          prev_definition,  // prev
                                          "STAR",  // name
                                          "*");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->star_definition);
  error = utf8lex_rule_init(&(lex->star),
                            prev,
                            "star",  // name
                            (utf8lex_definition_t *)
                            &(lex->star_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->star);
  // +
  error = utf8lex_literal_definition_init(&(lex->plus_definition),
                                          prev_definition,  // prev
                                          "PLUS",  // name
                                          "+");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->plus_definition);
  error = utf8lex_rule_init(&(lex->plus),
                            prev,
                            "plus",  // name
                            (utf8lex_definition_t *)
                            &(lex->plus_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->plus);
  // (backslash)
  error = utf8lex_literal_definition_init(&(lex->backslash_definition),
                                          prev_definition,  // prev
                                          "BACKSLASH",  // name
                                          "\\");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->backslash_definition);
  error = utf8lex_rule_init(&(lex->backslash),
                            prev,
                            "backslash",  // name
                            (utf8lex_definition_t *)
                            &(lex->backslash_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->backslash);

  // definition ID
  error = utf8lex_regex_definition_init(&(lex->id_definition),
                                        prev_definition,  // prev
                                        "ID",  // name
                                        "[_\\p{L}][_\\p{L}\\p{N}]*");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->id_definition);
  error = utf8lex_rule_init(&(lex->id),
                            prev,
                            "id",  // name
                            (utf8lex_definition_t *)
                            &(lex->id_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->id);

  // horizontal whitespace
  error = utf8lex_regex_definition_init(&(lex->space_definition),
                                        prev_definition,  // prev
                                        "SPACE",  // name
                                        "[\\h]+");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->space_definition);
  error = utf8lex_rule_init(&(lex->space),
                            prev,
                            "space",  // name
                            (utf8lex_definition_t *)
                            &(lex->space_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->space);

  // anything EXCEPT backslash.
  error = utf8lex_regex_definition_init(&(lex->not_backslash_definition),
                                        prev_definition,  // prev
                                        "NOT_BACKSLASH",  // name
                                        "[^\\\\]");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->not_backslash_definition);
  error = utf8lex_rule_init(&(lex->not_backslash),
                            prev,
                            "not_backslash",  // name
                            (utf8lex_definition_t *)
                            &(lex->not_backslash_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->not_backslash);

  // Any (regex .)
  error = utf8lex_regex_definition_init(&(lex->any_definition),
                                        prev_definition,  // prev
                                        "ANY",  // name
                                        ".");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->any_definition);
  error = utf8lex_rule_init(&(lex->any),
                            prev,
                            "any",  // name
                            (utf8lex_definition_t *)
                            &(lex->any_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->any);


  // Everything to the end of the line
  error = utf8lex_cat_definition_init(&(lex->to_eol_definition),
                                      prev_definition,  // prev
                                      "TO_EOL",  // name
                                      UTF8LEX_GROUP_NOT_VSPACE,  // cat
                                      1,  // min
                                      -1);  // max
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->to_eol_definition);
  error = utf8lex_rule_init(&(lex->to_eol),
                            NULL,  // Do not link to other rules.
                            "to_eol",  // name
                            (utf8lex_definition_t *)
                            &(lex->to_eol_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_generate_setup_db(&(lex->db));
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_fill_some_of_remaining_buffer(
        unsigned char *some_of_remaining_buffer,
        utf8lex_state_t *state,
        size_t buffer_bytes,
        size_t max_bytes
        )
{
  if (some_of_remaining_buffer == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->loc == NULL)
  {
    fprintf(stderr, "ERROR 3 in utf8lex_fill_some_of_remaining_buffer() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (buffer_bytes <= (size_t) 0
           || max_bytes <= (size_t) 0)
  {
    fprintf(stderr, "ERROR 4 in utf8lex_fill_some_of_remaining_buffer() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
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

  return UTF8LEX_OK;
}

static utf8lex_error_t utf8lex_generate_token_error(
        utf8lex_state_t *state,
        utf8lex_token_t *token,
        unsigned char *message
        )
{
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
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  unsigned char some_of_remaining_buffer[32];
  utf8lex_fill_some_of_remaining_buffer(some_of_remaining_buffer,
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
  return UTF8LEX_ERROR_TOKEN;
}

static utf8lex_error_t utf8lex_generate_read_to_eol(
        utf8lex_generate_lexicon_t *lex,
        utf8lex_state_t *state,
        utf8lex_token_t *line_token_pointer,
        utf8lex_token_t *newline_token_pointer
        )
{
  bool is_empty_to_eol = false;
  utf8lex_error_t error = utf8lex_lex(&(lex->to_eol),
                                      state,
                                      line_token_pointer);
  if (error == UTF8LEX_EOF)
  {
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
    utf8lex_fill_some_of_remaining_buffer(
        some_of_remaining_buffer,
        state,
        (size_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].length,
        (size_t) 32);
    fprintf(stderr,
            "ERROR utf8lex Failed to read to EOL [%d.%d]: \"%s\"\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start, some_of_remaining_buffer);
    return error;
  }

  // Read in the newline.
  error = utf8lex_lex(&(lex->newline),
                      state,
                      newline_token_pointer);
  if (error == UTF8LEX_EOF)
  {
    return error;
  }
  else if (error != UTF8LEX_OK)
  {
    // Error.
    unsigned char some_of_remaining_buffer[32];
    utf8lex_fill_some_of_remaining_buffer(
        some_of_remaining_buffer,
        state,
        (size_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].length,
        (size_t) 32);
    fprintf(stderr,
            "ERROR utf8lex Failed to read newline [%d.%d]: \"%s\"\n",
            state->loc[UTF8LEX_UNIT_LINE].start + 1,
            state->loc[UTF8LEX_UNIT_CHAR].start,
            some_of_remaining_buffer);
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

  return UTF8LEX_OK;
}

static utf8lex_error_t utf8lex_generate_write_line(
        int fd_out,
        utf8lex_generate_lexicon_t *lex,
        utf8lex_state_t *state
        )
{
  if (lex == NULL
      || state == NULL)
  {
    fprintf(stderr, "ERROR 7 in utf8lex_generate_write_line() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    fprintf(stderr, "ERROR 8 in utf8lex_generate_write_line() [%d.%d]: UTF8LEX_ERROR_FILE_DESCRIPTOR\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
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
    return error;
  }

  // Write out the rest of the line:
  bytes_written = write(fd_out,
                        &(line_token.str->bytes[line_token.start_byte]),
                        line_token.length_bytes);
  if (bytes_written != line_token.length_bytes)
  {
    fprintf(stderr, "ERROR 9 in utf8lex_generate_write_line() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  // Write out the newline:
  bytes_written = write(fd_out,
                        &(newline_token.str->bytes[newline_token.start_byte]),
                        newline_token.length_bytes);
  if (bytes_written != newline_token.length_bytes)
  {
    fprintf(stderr, "ERROR 10 in utf8lex_generate_write_line() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  return UTF8LEX_OK;
}

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

static utf8lex_error_t utf8lex_generate_definition(
        utf8lex_generate_lexicon_t *lex,  // .l file lexicon, this file's db.
        utf8lex_state_t *state,  // .l file current state.
        unsigned char *name,  // Name of (definition or rule).
        bool is_rule  // true = rules section; false = definitions section.
        )
{
  if (lex == NULL
      || state == NULL
      || name == NULL)
  {
    fprintf(stderr, "ERROR 11 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
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

  // Store the definition's / rule's name.
  int dn = lex->db.num_definitions;
  strcpy(lex->db.definition_names[dn],
         name);
  lex->db.num_definitions ++;
  if (lex->db.num_definitions >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
  {
    fprintf(stderr, "ERROR 12 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
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
  if (error != UTF8LEX_OK) { return error; }
  lex->db.num_multi_definitions ++;
  if (lex->db.num_multi_definitions >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
  {
    fprintf(stderr, "ERROR 13 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    return UTF8LEX_ERROR_MAX_LENGTH;
  }
  lex->db.last_definition = (utf8lex_definition_t *)
    &(lex->db.multi_definitions[md]);

  // Now parse the definition.
  // DEF1 ...defintiion... (space first).
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
  lex->db.str[dn][0] = 0;
  unsigned char regex_space[256];
  regex_space[0] = 0;
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
      utf8lex_fill_some_of_remaining_buffer(
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
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    error = utf8lex_lex(lex->lex_rules,
                        state,
                        &token);
    if (error != UTF8LEX_OK)
    {
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
        if (error != UTF8LEX_OK) { return error; }
      }
      break;

    case UTF8LEX_LEX_STATE_REGEX_SPACE:
      // Transitioning to either more REGEX or RULE or COMPLETE.
      // Store this token's text in case we continue with the REGEX.
      error = utf8lex_token_copy_string(
                  &token,  // self
                  regex_space,  // str
                  (size_t) 256);  // max_bytes
      if (error != UTF8LEX_OK) { return error; }
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
      if (error != UTF8LEX_OK) { return error; }
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
        if (error != UTF8LEX_OK) { return error; }
        error = utf8lex_reference_init(
                    &(lex->db.references[rd]),  // self
                    last_reference,  // prev
                    lex->db.reference_names[rd],  // name
                    1,  // min
                    1,  // max
                    &(lex->db.multi_definitions[md]));  // parent
        if (error != UTF8LEX_OK) { return error; }
        lex->db.num_references ++;
        if (lex->db.num_references >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
        {
          fprintf(stderr, "ERROR 16 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
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
              fprintf(stderr, "ERROR utf8lex No such defintiion [%d.%d]: %s\n",
                      state->loc[UTF8LEX_UNIT_LINE].start + 1,
                      state->loc[UTF8LEX_UNIT_CHAR].start,
                      unresolved_reference->definition_name);
              return error;
            }
          }
          if (error != UTF8LEX_OK) { return error; }
        }
      }
      else
      {
        int rn = lex->db.num_rules;
        error = utf8lex_token_cat_string(
                    &token,  // self
                    lex->db.rule_code[rn],  // str
                    (size_t) UTF8LEX_RULE_CODE_LENGTH_MAX);  // max_bytes
        if (error != UTF8LEX_OK) { return error; }
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
    return UTF8LEX_ERROR_UNRESOLVED_DEFINITION;
  }

  // If we do not need the multi definition, delete it.
  if (definition_type != UTF8LEX_DEFINITION_TYPE_MULTI)
  {
    if (lex->db.last_definition != (utf8lex_definition_t *)
        &(lex->db.multi_definitions[md]))
    {
      fprintf(stderr, "ERROR 20 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_STATE\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
      return UTF8LEX_ERROR_STATE;
    }
    lex->db.last_definition = lex->db.last_definition->prev;
    error = utf8lex_multi_definition_clear(
                (utf8lex_definition_t *)
                &(lex->db.multi_definitions[md])  // self
                );
    if (error != UTF8LEX_OK) { return error; }
    lex->db.num_multi_definitions --;
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
    if (error != UTF8LEX_OK) { return error; }
    lex->db.num_literal_definitions ++;
    if (lex->db.num_literal_definitions >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
    {
      fprintf(stderr, "ERROR 21 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    lex->db.last_definition = (utf8lex_definition_t *)
      &(lex->db.literal_definitions[ld]);
  }
  else if (definition_type == UTF8LEX_DEFINITION_TYPE_REGEX)
  {
    int rd = lex->db.num_regex_definitions;
    error = utf8lex_regex_definition_init(
                &(lex->db.regex_definitions[rd]),  // self
                lex->db.last_definition,  // prev
                lex->db.definition_names[dn],  // name
                lex->db.str[dn]);  // pattern
    if (error != UTF8LEX_OK) { return error; }
    lex->db.num_regex_definitions ++;
    if (lex->db.num_regex_definitions >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
    {
      fprintf(stderr, "ERROR 22 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_MAX_LENGTH\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    lex->db.last_definition = (utf8lex_definition_t *)
      &(lex->db.regex_definitions[rd]);
  }
  else if (definition_type == UTF8LEX_DEFINITION_TYPE_MULTI)
  {
    lex->db.multi_definitions[md].multi_type = multi_type;
  }
  else
  {
    // Incomplete parse or something.
    fprintf(stderr, "ERROR 23 in utf8lex_generate_definition() [%d.%d]: UTF8LEX_ERROR_STATE\n", state->loc[UTF8LEX_UNIT_LINE].start + 1, state->loc[UTF8LEX_UNIT_CHAR].start);
    return UTF8LEX_ERROR_STATE;
  }

  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_generate_write_rules(
        int fd_out,
        utf8lex_db_t *db
        )
{
  if (db == NULL)
  {
    fprintf(stderr, "ERROR 24 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    fprintf(stderr, "ERROR 25 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_DESCRIPTOR\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  unsigned char line[4096];
  size_t max_bytes = 4096;
  size_t line_bytes;
  size_t bytes_written;

  unsigned char newline[] = "\n";
  size_t newline_bytes = strlen(newline);

  bool is_first;

  utf8lex_error_t error;

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_cat_definition_t YY_CAT_DEFINITIONS[%d];\n",
                        UTF8LEX_NUM_CATEGORIES);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 26 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 27 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_literal_definition_t YY_LITERAL_DEFINITIONS[%d];\n",
                        db->num_literal_definitions);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 28 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 29 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_multi_definition_t YY_MULTI_DEFINITIONS[%d];\n",
                        db->num_multi_definitions);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 30 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 31 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_reference_t YY_REFERENCES[%d];\n",
                        db->num_references);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 32 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 33 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_regex_definition_t YY_REGEX_DEFINITIONS[%d];\n",
                        db->num_regex_definitions);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 34 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 35 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 36 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_rule_t YY_RULES[%d];\n",
                        db->num_rules);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 37 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 38 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 39 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 40 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_error_t yy_rules_init()\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 41 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 42 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "{\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 43 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 44 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    utf8lex_error_t error;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 45 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 46 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    utf8lex_definition_t *rule_definition;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 47 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 48 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  int cd = 0;  // cat definition #
  int ld = 0;  // literal definition #
  int rd = 0;  // regex definition #
  int md = 0;  // multi definition #
  int ref = 0;  // reference #

  unsigned char printable_str[2 * UTF8LEX_LITERAL_REGEX_MAX_BYTES];

  // Previous definition, reference, rule, etc:
  unsigned char previous[UTF8LEX_NAME_LENGTH_MAX + 16];


  // Definitions:
  // ====================
  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 49 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // Definitions:\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 50 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 51 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // =================================================================\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 52 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 53 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  utf8lex_definition_t *definition = db->definitions_db;
  strcpy(previous, "NULL");
  is_first = true;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < db->num_definitions;
       infinite_loop_protector ++)
  {
    if (definition == NULL)
    {
      fprintf(stderr, "ERROR 54 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    bytes_written = write(fd_out, newline, newline_bytes);
    if (bytes_written != newline_bytes) {
      fprintf(stderr, "ERROR 55 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    if (definition->definition_type == UTF8LEX_DEFINITION_TYPE_CAT)
    {
      // Start cat
      // --------------------
      if (cd >= UTF8LEX_NUM_CATEGORIES)
      {
        fprintf(stderr, "ERROR 56 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_STATE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    // Definition # %d: %s (cat)\n",
                            infinite_loop_protector,
                            db->cat_definitions[cd].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 57 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 58 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    error = utf8lex_cat_definition_init(\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 59 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 60 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                &(YY_CAT_DEFINITIONS[%d]),  // self\n",
                            cd);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 6 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 62 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_definition_t *) %s,  // prev\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 63 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 64 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\",  // name\n",
                            db->cat_definitions[cd].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 65 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 66 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      // TODO create a cat enum to full cat enum string function
      // TDOO e.g. UTF8LEX_CAT_LETTER_LOWER -> "UTF8LEX_CAT_LETTER_LOWER"
      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_cat_t) %d,  // cat\n",
                            db->cat_definitions[cd].cat);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 67 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 68 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                %d,  // min\n",
                            db->cat_definitions[cd].min);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 69 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 70 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                %d);  // max\n",
                            db->cat_definitions[cd].max);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 71 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 72 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    if (error != UTF8LEX_OK) { return error; }\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 73 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 74 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                            "&(YY_CAT_DEFINITIONS[%d])",
                            cd);
      if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
        fprintf(stderr, "ERROR 75 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }

      cd ++;
      // --------------------
      // End cat
    }
    else if (definition->definition_type == UTF8LEX_DEFINITION_TYPE_LITERAL)
    {
      // Start literal
      // --------------------
      if (ld >= db->num_literal_definitions)
      {
        fprintf(stderr, "ERROR 76 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_STATE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    // Definition # %d: %s (literal)\n",
                            infinite_loop_protector,
                            db->literal_definitions[ld].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 77 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 78 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    error = utf8lex_literal_definition_init(\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 79 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 80 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                &(YY_LITERAL_DEFINITIONS[%d]),  // self\n",
                            ld);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 81 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 82 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_definition_t *) %s,  // prev\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 83 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 84 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\",  // name\n",
                            db->literal_definitions[ld].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 85 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 86 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      error = utf8lex_printable_str(printable_str,
                                    2 * UTF8LEX_LITERAL_REGEX_MAX_BYTES,
                                    db->literal_definitions[ld].str,
                                    UTF8LEX_PRINTABLE_ALL);
      if (error != UTF8LEX_OK) { return error; }
      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\");  // str\n",
                            printable_str);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 87 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 88 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    if (error != UTF8LEX_OK) { return error; }\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 89 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 90 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                            "&(YY_LITERAL_DEFINITIONS[%d])",
                            ld);
      if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
        fprintf(stderr, "ERROR 91 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }

      ld ++;
      // --------------------
      // End literal
    }
    else if (definition->definition_type == UTF8LEX_DEFINITION_TYPE_MULTI)
    {
      // Start multi
      // --------------------
      if (md >= db->num_multi_definitions)
      {
        fprintf(stderr, "ERROR 92 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_STATE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    // Definition # %d: %s (multi)\n",
                            infinite_loop_protector,
                            db->multi_definitions[md].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 93 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 94 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    error = utf8lex_multi_definition_init(\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 95 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 96 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                &(YY_MULTI_DEFINITIONS[%d]),  // self\n",
                            md);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 97 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 98 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_definition_t *) %s,  // prev\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 99 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 100 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\",  // name\n",
                            db->multi_definitions[md].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 101 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 102 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      // TODO allow nested multi definitions e.g. A B (X | D) etc.
      if (db->multi_definitions[md].parent != NULL)
      {
        fprintf(stderr, "ERROR 103 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_NOT_IMPLEMENTED\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_NOT_IMPLEMENTED;
      }
      line_bytes = snprintf(line, max_bytes,
                            "                NULL,  // parent\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 104 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 105 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      if (db->multi_definitions[md].multi_type == UTF8LEX_MULTI_TYPE_SEQUENCE)
      {
        line_bytes = snprintf(line, max_bytes,
                              "                UTF8LEX_MULTI_TYPE_SEQUENCE);  // multi_type\n");
      }
      else if (db->multi_definitions[md].multi_type == UTF8LEX_MULTI_TYPE_OR)
      {
        line_bytes = snprintf(line, max_bytes,
                              "                UTF8LEX_MULTI_TYPE_OR);  // multi_type\n");
      }
      else
      {
        fprintf(stderr, "ERROR 106 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_MULTI_TYPE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_MULTI_TYPE;
      }
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 107 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 108 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    if (error != UTF8LEX_OK) { return error; }\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 109 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 110 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      // Now the references:
      bytes_written = write(fd_out, newline, newline_bytes);
      if (bytes_written != newline_bytes) {
        fprintf(stderr, "ERROR 111 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      strcpy(previous, "NULL");
      utf8lex_reference_t *reference = db->multi_definitions[md].references;
      for (int ref_infinite = 0;
           ref_infinite < UTF8LEX_REFERENCES_LENGTH_MAX;
           ref_infinite ++)
      {
        if (reference == NULL)
        {
          break;
        }
        else if (md >= db->num_references)
        {
          fprintf(stderr, "ERROR 112 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_STATE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "    error = utf8lex_reference_init(\n");
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 113 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 114 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                &(YY_REFERENCES[%d]),  // self\n",
                              ref);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 115 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 116 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                %s,  // prev\n",
                              previous);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 117 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 118 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                \"%s\",  // name\n",
                              reference->definition_name);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 119 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 120 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                %d,  // min\n",
                              reference->min);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 121 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 122 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                %d,  // max\n",
                              reference->max);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 123 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 124 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                &(YY_MULTI_DEFINITIONS[%d]));  // parent\n",
                              md);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 125 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 126 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "    if (error != UTF8LEX_OK) { return error; }\n");
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 127 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 128 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                              "&(YY_REFERENCES[%d])",
                              ref);
        if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
          fprintf(stderr, "ERROR 129 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          return UTF8LEX_ERROR_BAD_LENGTH;
        }

        reference = reference->next;
        ref ++;
      }

      if (reference != NULL)
       {
         fprintf(stderr, "ERROR 130 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_INFINITE_LOOP\n", /* line */ 0, /* char */ 0);
         return UTF8LEX_ERROR_INFINITE_LOOP;
       }

      line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                            "&(YY_MULTI_DEFINITIONS[%d])",
                            md);
      if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
        fprintf(stderr, "ERROR 131 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }

      md ++;
      // --------------------
      // End multi
    }
    else if (definition->definition_type == UTF8LEX_DEFINITION_TYPE_REGEX)
    {
      // Start regex
      // --------------------
      if (rd >= db->num_regex_definitions)
      {
        fprintf(stderr, "ERROR 132 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_STATE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    // Definition # %d: %s (regex)\n",
                            infinite_loop_protector,
                            db->regex_definitions[rd].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 133 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 134 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    error = utf8lex_regex_definition_init(\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 135 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 136 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                &(YY_REGEX_DEFINITIONS[%d]),  // self\n",
                            rd);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 137 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 138 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_definition_t *) %s,  // prev\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 139 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 140 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\",  // name\n",
                            db->regex_definitions[rd].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 141 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 142 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      error = utf8lex_printable_str(printable_str,
                                    2 * UTF8LEX_LITERAL_REGEX_MAX_BYTES,
                                    db->regex_definitions[rd].pattern,
                                    UTF8LEX_PRINTABLE_ALL);
      if (error != UTF8LEX_OK) { return error; }
      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\");  // pattern\n",
                            printable_str);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 143 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 144 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }


      line_bytes = snprintf(line, max_bytes,
                            "    if (error != UTF8LEX_OK) { return error; }\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 145 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 146 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                            "&(YY_REGEX_DEFINITIONS[%d])",
                            rd);
      if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
        fprintf(stderr, "ERROR 147 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }

      rd ++;
      // --------------------
      // End regex
    }
    else
    {
      fprintf(stderr, "ERROR 148 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_DEFINITION_TYPE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_DEFINITION_TYPE;
    }

    if (is_first == true)
    {
      line_bytes = snprintf(line, max_bytes,
                            "    YY_FIRST_DEFINITION = (utf8lex_definition_t *) %s;\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 149 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 150 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      is_first = false;
    }

    definition = definition->next;
  }

  if (definition != NULL)
  {
    // The definitions db has a loop in it somewhere, somehow.
    fprintf(stderr, "ERROR 151 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_INFINITE_LOOP\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (cd != UTF8LEX_NUM_CATEGORIES
           || ld != db->num_literal_definitions
           || md != db->num_multi_definitions
           || rd != db->num_regex_definitions)
  {
    fprintf(stderr, "ERROR 152 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_STATE;
  }


  // Resolve multi-definitions:
  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 153 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // Resolve multi-definitions:\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 154 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 155 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  for (int md = 0; md < db->num_multi_definitions; md ++)
  {
    line_bytes = snprintf(line, max_bytes,
                          "    // # %d %s:\n",
                          md,
                          db->multi_definitions[md].base.name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 156 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 157 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    error = utf8lex_multi_definition_resolve(\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 158 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 159 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "        &(YY_MULTI_DEFINITIONS[%d]),  // self\n",
                          md);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 160 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 161 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "        YY_FIRST_DEFINITION);  // db\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 162 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 163 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    if (error != UTF8LEX_OK) { return error; }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 164 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }
  }


  // Rules:
  // ====================
  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 166 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // Rules:\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 167 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 168 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // =================================================================\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 169 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 170 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  utf8lex_rule_t *rule = db->rules_db;
  int rn = 0;
  strcpy(previous, "NULL");
  is_first = true;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < db->num_rules;
       infinite_loop_protector ++)
  {
    if (rule == NULL)
    {
      fprintf(stderr, "ERROR 171 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    bytes_written = write(fd_out, newline, newline_bytes);
    if (bytes_written != newline_bytes) {
      fprintf(stderr, "ERROR 172 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    // Rule # %d: %s\n",
                          rn,
                          rule->name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 173 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 174 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    error = utf8lex_definition_find_by_id(\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 175 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 176 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                YY_FIRST_DEFINITION,  // first_definition\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 177 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 178 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                (uint32_t) %u,  // id (\"%s\")\n",
                          (unsigned int) rule->definition->id,
                          rule->definition->name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 179 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 180 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                &rule_definition);  // found_pointer\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 181 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 182 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    if (error != UTF8LEX_OK) { return error; }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 183 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 184 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    error = utf8lex_rule_init(\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 185 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 186 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                &(YY_RULES[%d]),  // self\n",
                          rn);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 187 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 188 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                %s,  // prev\n",
                          previous);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 189 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 190 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                \"%s\",  // name\n",
                          rule->name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 191 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 192 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                rule_definition,  // definition\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 193 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 194 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                \"\",  // code\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 195 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 196 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                (size_t) 0);  // code_length_bytes\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 197 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 198 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    if (error != UTF8LEX_OK) { return error; }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 199 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 200 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                          "&(YY_RULES[%d])",
                          rn);
    if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
      fprintf(stderr, "ERROR 201 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }

    if (is_first == true)
    {
      line_bytes = snprintf(line, max_bytes,
                            "    YY_FIRST_RULE = %s;\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 202 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 203 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      is_first = false;
    }

    rule = rule->next;
    rn ++;
  }

  if (rule != NULL)
  {
    // The rules db has a loop in it somewhere, somehow.
    fprintf(stderr, "ERROR 204 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_INFINITE_LOOP\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (rn != db->num_rules)
  {
    fprintf(stderr, "ERROR 205 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_STATE;
  }
  else if (is_first == true)
  {
    fprintf(stderr, "ERROR generating yy_rules_init() [%d.%d]: no rules in the 2nd section\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_STATE;
  }


  // Close yy_rules_init() function declaration:
  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 206 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    return UTF8LEX_OK;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 207 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 208 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "}\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 209 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 210 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_generate_write_rule_callbacks(
        int fd_out,
        utf8lex_db_t *db
        )
{
  if (db == NULL)
  {
    fprintf(stderr, "ERROR 211 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    fprintf(stderr, "ERROR 212 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_DESCRIPTOR\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  unsigned char line[4096];
  size_t max_bytes = 4096;
  size_t line_bytes;
  size_t bytes_written;

  unsigned char newline[] = "\n";
  size_t newline_bytes = strlen(newline);

  bool is_first;

  utf8lex_error_t error;

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 213 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static int yy_rule_callback(\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 214 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 215 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        utf8lex_token_t *token\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 216 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 217 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        )\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 218 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 219 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "{\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 220 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 221 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    if (token == NULL\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 222 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 223 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        || token->rule == NULL\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 224 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 225 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        || token->rule->code == NULL)\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 226 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 227 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    {\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 228 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 229 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        return YYerror;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 230 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 231 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    }\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 232 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 233 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 234 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    switch (token->rule->id)\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 235 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 236 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    {\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 237 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 238 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  utf8lex_rule_t *rule = db->rules_db;
  int rn = 0;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < db->num_rules;
       infinite_loop_protector ++)
  {
    if (rule == NULL)
    {
      fprintf(stderr, "ERROR 239 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    line_bytes = snprintf(line, max_bytes,
                          "        case (uint32_t) %d:  // # %d %s\n",
                          rule->id,
                          rn,
                          rule->name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 240 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 241 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    // TODO indent the rule's code
    bytes_written = write(fd_out, rule->code, rule->code_length_bytes);
    if (bytes_written != rule->code_length_bytes) {
      fprintf(stderr, "ERROR 242 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "            break;\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 243 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 244 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    rule = rule->next;
    rn ++;
  }

  if (rule != NULL)
  {
    // The rules db has a loop in it somewhere, somehow.
    fprintf(stderr, "ERROR 245 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_INFINITE_LOOP\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (rn != db->num_rules)
  {
    fprintf(stderr, "ERROR 246 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_STATE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        default:\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 247 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 248 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "            return YYerror;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 249 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 250 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    }\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 251 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 252 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    return (int) token->rule->id;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 253 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 254 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "}\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 255 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 256 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_generate_parse(
        const utf8lex_target_language_t *target_language,
        utf8lex_state_t *state_pointer,
        utf8lex_buffer_t *lex_file,
        int fd_out
        )
{
  int i = 5;
  if (target_language == NULL
      || state_pointer == NULL)
  {
    fprintf(stderr, "ERROR 257 in utf8lex_generate_parse() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    fprintf(stderr, "ERROR 258 in utf8lex_generate_parse() [%d.%d]: UTF8LEX_ERROR_FILE_DESCRIPTOR\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  utf8lex_error_t error;

  // lex: the tokens that we parse in every .l file, as well as
  // lex.db, the database of definitions that this particular
  // .l file generates (we pre-define all the character
  // categories as builtin definitions which can be referenced
  // or overridden in the .l file).
  utf8lex_generate_lexicon_t lex;
  error = utf8lex_generate_setup(&lex);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Get the .l file lexer state set up:
  error = utf8lex_state_init(state_pointer, lex_file);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // This horrid approach will have to do for now.
  // After trying to make the lex grammar context-free, and running
  // out of Christmas break in which to battle that windmill,
  // it's time for a hack.

  // -------------------------------------------------------------------
  // Definitions section:
  // -------------------------------------------------------------------
  bool is_enclosed = false;
  bool is_end_of_section = false;
  int infinite_loop_protector = 0;
  while (true)
  {
    infinite_loop_protector ++;
    if (infinite_loop_protector > UTF8LEX_LEX_FILE_NUM_LINES_MAX)
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex Aborting, possible infinite loop [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    // Read a token in from the beginning of the line:
    utf8lex_token_t token;
    error = utf8lex_lex(lex.lex_rules,
                        state_pointer,
                        &token);
    if (is_enclosed == true
        && error != UTF8LEX_OK
        && error != UTF8LEX_EOF)
    {
      // Not a token, but we're inside the enclosed code section,
      // so just write it out.
      utf8lex_generate_write_line(fd_out,
                                  &lex,
                                  state_pointer);
      error = UTF8LEX_OK;
      continue;
    }
    else if (error != UTF8LEX_OK)
    {
      // Error.
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return error;
    }

    // Acceptable tokens in the definitions section:
    //   (newline)    Blank line, ignored.
    //   %{           Start enclosed code section.  (Write contents to fd_out.)
    //   %}           End enclosed code section.
    //   (space) ...  Indented code line.
    //   (id) ...     Definition.
    //   %%           Section divider, move on to next section.
    error = UTF8LEX_ERROR_TOKEN;  // Default to invalid token.
    if (lex.newline.id == token.rule->id)
    {
      // (newline)
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_open.id == token.rule->id
             && is_enclosed == false)
    {
      // ${
      is_enclosed = true;
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_close.id == token.rule->id
             && is_enclosed == true)
    {
      // %}
      is_enclosed = false;
      error = UTF8LEX_OK;
    }
    else if (is_enclosed == true)
    {
      // Inside %{ ... %}
      // Write out the token, and the rest of the line, to fd_out.
      size_t bytes_written = write(fd_out,
                                   &(token.str->bytes[token.start_byte]),
                                   token.length_bytes);
      if (bytes_written != token.length_bytes)
      {
        error = UTF8LEX_ERROR_FILE_WRITE;
      }

      // Now write out the rest of the line to fd_out.
      error = utf8lex_generate_write_line(fd_out,
                                          &lex,
                                          state_pointer);
    }
    else if (lex.space.id == token.rule->id)
    {
      // (space) ...
      utf8lex_generate_write_line(fd_out,
                                  &lex,
                                  state_pointer);
      error = UTF8LEX_OK;
    }
    else if (lex.id.id == token.rule->id)
    {
      // (id) ...
      uint32_t d = lex.db.num_definitions;
      error = utf8lex_token_copy_string(&token,  // self
                                        lex.db.definition_names[d],  // str
                                        UTF8LEX_NAME_LENGTH_MAX);  // max_bytes
      if (error != UTF8LEX_OK)
      {
        return error;
      }
      error = utf8lex_generate_definition(
                  &lex,  // lex
                  state_pointer,  // state
                  lex.db.definition_names[d],  // name
                  false);  // is_rule -- we're not in the rules section.
      if (error != UTF8LEX_OK)
      {
        return error;
      }

      error = UTF8LEX_OK;
    }
    else if (lex.section_divider.id == token.rule->id)
    {
      // %%
      error = utf8lex_lex(lex.lex_rules,
                          state_pointer,
                          &token);
      if (error != UTF8LEX_OK)
      {
        // Error.
        unsigned char some_of_remaining_buffer[32];
        utf8lex_fill_some_of_remaining_buffer(
            some_of_remaining_buffer,
            state_pointer,
            (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
            (size_t) 32);
        fprintf(stderr,
                "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
                some_of_remaining_buffer);
        return error;
      }
      else if (lex.newline.id != token.rule->id)
      {
        error = utf8lex_generate_token_error(
            state_pointer,
            &token,
            "Expected newline after %% definitions/rules section divider");
        return error;
      }

      // Move on to the rules section.
      error = UTF8LEX_OK;
      is_end_of_section = true;
      break;
    }
    else
    {
      error = UTF8LEX_ERROR_TOKEN;
    }

    if (error != UTF8LEX_OK)
    {
      error = utf8lex_generate_token_error(
          state_pointer,
          &token,
          "Unexpected token in definitions section");
      return error;
    }

    if (is_end_of_section == true)
    {
      break;
    }
  }

  if (error != UTF8LEX_OK)
  {
    return error;
  }


  // Resolve all the multi-definitions from the definitions section
  // (if any).
  int num_multi_definitions = lex.db.num_multi_definitions;
  for (int md = 0; md < num_multi_definitions; md ++)
  {
    error = utf8lex_multi_definition_resolve(
                &(lex.db.multi_definitions[md]),  // self,
                lex.db.definitions_db);  // db
    if (error == UTF8LEX_ERROR_NOT_FOUND) {
      utf8lex_reference_t *unresolved_reference =
        lex.db.multi_definitions[md].references;
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
        fprintf(stderr, "ERROR utf8lex No such defintiion [%d.%d]: %s\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
                unresolved_reference->definition_name);
        return error;
      }
    }
    if (error != UTF8LEX_OK) {
      return error;
    }
  }


  // -------------------------------------------------------------------
  // Rules section:
  // -------------------------------------------------------------------
  is_enclosed = false;
  is_end_of_section = false;
  infinite_loop_protector = 0;
  while (true)
  {
    infinite_loop_protector ++;
    if (infinite_loop_protector > UTF8LEX_LEX_FILE_NUM_LINES_MAX)
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex Aborting, possible infinite loop [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    // Read a token in from the beginning of the line:
    utf8lex_token_t token;
    error = utf8lex_lex(lex.lex_rules,
                        state_pointer,
                        &token);
    if (is_enclosed == true
        && error != UTF8LEX_OK
        && error != UTF8LEX_EOF)
    {
      // Not a token, but we're inside the enclosed code section,
      // so just write it out.
      utf8lex_generate_write_line(fd_out,
                                  &lex,
                                  state_pointer);
      error = UTF8LEX_OK;
      continue;
    }
    else if (error != UTF8LEX_OK)
    {
      // Error.
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return error;
    }

    // Acceptable tokens in the rules section:
    //   (newline)                 Blank line, ignored.
    //   %{                        Start enclosed code section.
    //   %}                        End enclosed code section.
    //   (space)                   Indented code line.
    //   (definition) { ...code... }  Rule.
    error = UTF8LEX_ERROR_TOKEN;  // Default to invalid token.
    if (lex.newline.id == token.rule->id)
    {
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_open.id == token.rule->id
             && is_enclosed == false)
    {
      is_enclosed = true;
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_close.id == token.rule->id
             && is_enclosed == true)
    {
      is_enclosed = false;
      error = UTF8LEX_OK;
    }
    else if (is_enclosed == true)
    {
      // Write out the token, and the rest of the line, to fd_out.
      size_t bytes_written = write(fd_out,
                                   &(token.str->bytes[token.start_byte]),
                                   token.length_bytes);
      if (bytes_written != token.length_bytes)
      {
        error = UTF8LEX_ERROR_FILE_WRITE;
      }

      // Now write out the rest of the line to fd_out.
      error = utf8lex_generate_write_line(fd_out,
                                          &lex,
                                          state_pointer);
    }
    else if (lex.space.id == token.rule->id)
    {
      utf8lex_generate_write_line(fd_out,
                                  &lex,
                                  state_pointer);
      error = UTF8LEX_OK;
    }
    else if (lex.section_divider.id == token.rule->id)
    {
      error = utf8lex_lex(lex.lex_rules,
                          state_pointer,
                          &token);
      if (error != UTF8LEX_OK)
      {
        // Error.
        unsigned char some_of_remaining_buffer[32];
        utf8lex_fill_some_of_remaining_buffer(
            some_of_remaining_buffer,
            state_pointer,
            (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
            (size_t) 32);
        fprintf(stderr,
                "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
                some_of_remaining_buffer);
        return error;
      }
      else if (lex.newline.id != token.rule->id)
      {
        error = utf8lex_generate_token_error(
            state_pointer,
            &token,
            "Expected newline after %% rules/user code section divider");
        return error;
      }

      // Move on to the user code section.
      error = UTF8LEX_OK;
      is_end_of_section = true;
      break;
    }
    else
    {
      // rule: (definition-without-id) {rule} or just (definition-without-id).
      // Note that the rule code could sprawl across multiple lines
      // (not yet handled).
      // Backup the buffer and state pointers to where the token pointer is:
      for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
           unit < UTF8LEX_UNIT_MAX;
           unit ++)
      {
        if (token.loc[unit].after == -1)
        {
          int length_units = token.loc[unit].length;
          state_pointer->buffer->loc[unit].start -= token.loc[unit].length;
          state_pointer->loc[unit].start -= token.loc[unit].length;
        }
        else
        {
          state_pointer->buffer->loc[unit].start = token.loc[unit].start;
          state_pointer->loc[unit].start = token.loc[unit].start;
        }
      }

      uint32_t r = lex.db.num_rules;
      int printed = snprintf(lex.db.rule_names[r],  // str
                             (size_t) UTF8LEX_NAME_LENGTH_MAX,  // size
                             "rule_%d",  // format
                             (int) r + 1);
      if (printed < 0)
      {
        fprintf(stderr, "ERROR 259 in utf8lex_generate_parse() [%d.%d]: UTF8LEX_ERROR_TOKEN\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start);
        return UTF8LEX_ERROR_TOKEN;
      }

      error = utf8lex_generate_definition(
                  &lex,  // lex
                  state_pointer,  // state
                  lex.db.rule_names[r],  // name
                  true);  // is_rule -- we're in the rules section.
      if (error != UTF8LEX_OK)
      {
        return error;
      }

      error = utf8lex_rule_init(
                  &(lex.db.rules[r]),  // self
                  lex.db.last_rule,  // prev
                  lex.db.rule_names[r],  // name
                  lex.db.last_definition,  // definition
                  lex.db.rule_code[r],  // code
                  strlen(lex.db.rule_code[r]));  // code_length_bytes
      if (error != UTF8LEX_OK)
      {
        return error;
      }

      if (r == 0)
      {
        lex.db.rules_db = &(lex.db.rules[r]);
      }
      lex.db.last_rule = &(lex.db.rules[r]);
      lex.db.num_rules ++;

      error = UTF8LEX_OK;
    }

    if (error != UTF8LEX_OK)
    {
      error = utf8lex_generate_token_error(
          state_pointer,
          &token,
          "Unexpected token in rules section");
      return error;
    }

    if (is_end_of_section == true)
    {
      break;
    }
  }

  if (error != UTF8LEX_OK)
  {
    return error;
  }


  // Now write out the definitions and rules to fd_out:
  error = utf8lex_generate_write_rules(fd_out,
                                       &(lex.db));
  if (error != UTF8LEX_OK) {
    return error;
  }

  error = utf8lex_generate_write_rule_callbacks(fd_out,
                                                &(lex.db));
  if (error != UTF8LEX_OK) {
    return error;
  }


  // -------------------------------------------------------------------
  // User code section:
  // -------------------------------------------------------------------
  infinite_loop_protector = 0;
  while (true)
  {
    infinite_loop_protector ++;
    if (infinite_loop_protector > UTF8LEX_LEX_FILE_NUM_LINES_MAX)
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex Aborting, possible infinite loop [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    // Write out the whole line to fd_out.
    error = utf8lex_generate_write_line(fd_out,
                                        &lex,
                                        state_pointer);
    if (error == UTF8LEX_EOF)
    {
      break;
    }
    else if (error != UTF8LEX_OK)
    {
      // Error.
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return error;
    }
  }

  if (error != UTF8LEX_EOF)
  {
    return error;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_generate(
        const utf8lex_target_language_t *target_language,
        unsigned char *lex_file_path,
        unsigned char *template_dir_path,
        unsigned char *generated_file_path,
        utf8lex_state_t *state_pointer  // Will be initialized.
        )
{
  if (target_language == NULL
      || target_language->extension == NULL
      || lex_file_path == NULL
      || template_dir_path == NULL
      || generated_file_path == NULL
      || state_pointer == NULL)
  {
    fprintf(stderr, "ERROR 260 in utf8lex_generate() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;

  // First, read in the lex .l file:
  // mmap the file to be lexed:
  utf8lex_string_t lex_file_str;
  lex_file_str.max_length_bytes = -1;
  lex_file_str.length_bytes = -1;
  lex_file_str.bytes = NULL;
  utf8lex_buffer_t lex_file_buffer;
  lex_file_buffer.next = NULL;
  lex_file_buffer.prev = NULL;
  lex_file_buffer.str = &lex_file_str;
  error = utf8lex_buffer_mmap(&lex_file_buffer,
                              lex_file_path);  // path
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Now, read in the "head" and "tail" template files
  // for the target language.
  size_t template_dir_path_length = strlen(template_dir_path);
  unsigned char *path_sep = "/";  // TODO OS independent
  size_t path_sep_length = strlen(path_sep);
  char *extension = target_language->extension;
  size_t extension_length = strlen(extension);
  size_t template_length =
    template_dir_path_length
    + path_sep_length
    // Your name goes here (e.g. "head", "tail").
    + extension_length;

  char head_path[template_length + strlen("head") + 1];
  strcpy(head_path, template_dir_path);
  strcat(head_path, path_sep);
  strcat(head_path, "head");
  strcat(head_path, extension);
  utf8lex_string_t head_str;
  head_str.max_length_bytes = -1;
  head_str.length_bytes = -1;
  head_str.bytes = NULL;
  utf8lex_buffer_t head_buffer;
  head_buffer.next = NULL;
  head_buffer.prev = NULL;
  head_buffer.str = &head_str;
  error = utf8lex_buffer_mmap(&head_buffer,
                              head_path);  // path
  if (error != UTF8LEX_OK)
  {
    utf8lex_buffer_munmap(&lex_file_buffer);
    return error;
  }

  char tail_path[template_length + strlen("tail") + 1];
  strcpy(tail_path, template_dir_path);
  strcat(tail_path, path_sep);
  strcat(tail_path, "tail");
  strcat(tail_path, extension);
  utf8lex_string_t tail_str;
  tail_str.max_length_bytes = -1;
  tail_str.length_bytes = -1;
  tail_str.bytes = NULL;
  utf8lex_buffer_t tail_buffer;
  tail_buffer.next = NULL;
  tail_buffer.prev = NULL;
  tail_buffer.str = &tail_str;
  error = utf8lex_buffer_mmap(&tail_buffer,
                              tail_path);  // path
  if (error != UTF8LEX_OK)
  {
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    return error;
  }

  // Generate and write out the target language source code file:
  int fd_out = open(generated_file_path,
                    O_CREAT | O_WRONLY | O_TRUNC,
                    S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH);
  if (fd_out < 0)
  {
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    utf8lex_buffer_munmap(&tail_buffer);
    fprintf(stderr, "ERROR 261 in utf8lex_generate() [%d.%d]: UTF8LEX_ERROR_FILE_OPEN\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_OPEN;
  }

  size_t bytes_written = (size_t) 0;

  bytes_written = write(fd_out,
                        head_buffer.str->bytes,
                        head_buffer.str->length_bytes);
  if (bytes_written != head_buffer.str->length_bytes)
  {
    close(fd_out);
    unlink(generated_file_path);  // Delete the failed output file.
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    utf8lex_buffer_munmap(&tail_buffer);
    fprintf(stderr, "ERROR 262 in utf8lex_generate() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  error = utf8lex_generate_parse(target_language,
                                 state_pointer,  // Will be initialized.
                                 &lex_file_buffer,
                                 fd_out);
  if (error != UTF8LEX_OK)
  {
    close(fd_out);
    unlink(generated_file_path);  // Delete the failed output file.
    state_pointer->buffer = NULL;
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    utf8lex_buffer_munmap(&tail_buffer);
    return error;
  }

  bytes_written = write(fd_out,
                        tail_buffer.str->bytes,
                        tail_buffer.str->length_bytes);
  if (bytes_written != tail_buffer.str->length_bytes)
  {
    close(fd_out);
    unlink(generated_file_path);  // Delete the failed output file.
    state_pointer->buffer = NULL;
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    utf8lex_buffer_munmap(&tail_buffer);
    fprintf(stderr, "ERROR 263 in utf8lex_generate() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n",
            state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
            state_pointer->loc[UTF8LEX_UNIT_CHAR].start);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  close(fd_out);
  state_pointer->buffer = NULL;
  utf8lex_buffer_munmap(&lex_file_buffer);
  utf8lex_buffer_munmap(&head_buffer);
  utf8lex_buffer_munmap(&tail_buffer);

  return UTF8LEX_OK;
}
