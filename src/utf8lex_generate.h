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
#ifndef UTF8LEX_GENERATE_H_INCLUDED
#define UTF8LEX_GENERATE_H_INCLUDED

#include <inttypes.h>  // For uint32_t.

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
  uint32_t num_cat_definitions;
  utf8lex_cat_definition_t cat_definitions[UTF8LEX_NUM_CATEGORIES];
  utf8lex_rule_t cats[UTF8LEX_NUM_CATEGORIES];

  // Definitions from the .l file (NOT in any order -- use definitions_db).
  // Note that the first UTF8LEX_NUM_CATEGORIES definitions are pre-defined,
  // not from the .l file.
  // A definition with name "X" will be deleted if another definition
  // with name "X" is defined in the .l file.  But the name index remains.
  uint32_t num_definition_names;
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

extern utf8lex_error_t utf8lex_generate_init(
        utf8lex_generate_lexicon_t *lex
        );

extern utf8lex_error_t utf8lex_generate_clear(
        utf8lex_generate_lexicon_t *lex
        );

extern utf8lex_error_t utf8lex_generate_token_error(
        utf8lex_state_t *state,
        utf8lex_token_t *token,
        unsigned char *message
        );

extern utf8lex_error_t utf8lex_generate_fill_some_of_remaining_buffer(
        unsigned char *some_of_remaining_buffer,
        utf8lex_state_t *state,
        size_t buffer_bytes,
        size_t max_bytes
        );

extern utf8lex_error_t utf8lex_generate_write_line(
        int fd_out,
        utf8lex_generate_lexicon_t *lex,
        utf8lex_state_t *state
        );

extern utf8lex_error_t utf8lex_generate_definition(
        utf8lex_generate_lexicon_t *lex,  // .l file lexicon, this file's db.
        utf8lex_state_t *state,  // .l file current state.
        unsigned char *name,  // Name of (definition or rule).
        bool is_rule  // true = rules section; false = definitions section.
        );

#endif  // UTF8LEX_GENERATE_H_INCLUDED
