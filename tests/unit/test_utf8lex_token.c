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
#include <string.h>  // For strcpy()

#include "utf8lex.h"

typedef struct _STRUCT_utf8lex_test_db
{
  utf8lex_definition_t *definitions_db;
  utf8lex_rule_t *rules_db;

  utf8lex_definition_t *last_definition;
  utf8lex_rule_t *last_rule;

  // NUMBER
  utf8lex_cat_definition_t number_definition;
  utf8lex_rule_t number_rule;

  // ID
  utf8lex_regex_definition_t id_definition;
  utf8lex_rule_t id_rule;

  // EQUALS3
  utf8lex_literal_definition_t equals3_definition;
  utf8lex_rule_t equals3_rule;

  // EQUALS
  utf8lex_literal_definition_t equals_definition;
  utf8lex_rule_t equals_rule;

  // PLUS
  utf8lex_literal_definition_t plus_definition;
  utf8lex_rule_t plus_rule;

  // MINUS
  utf8lex_literal_definition_t minus_definition;
  utf8lex_rule_t minus_rule;

  // SPACE
  utf8lex_regex_definition_t space_definition;
  utf8lex_rule_t space_rule;
} utf8lex_test_db_t;

typedef struct _STRUCT_utf8lex_test_operator
{
  // OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
  utf8lex_multi_definition_t operator_definition;

  utf8lex_reference_t ref_equals3;
  utf8lex_reference_t ref_equals;
  utf8lex_reference_t ref_plus;
  utf8lex_reference_t ref_minus;
} utf8lex_test_operator_t;

typedef struct _STRUCT_utf8lex_test_declaration
{
  // DECLARATION = ID SPACE ID
  utf8lex_multi_definition_t declaration_definition;

  utf8lex_reference_t ref_id1;
  utf8lex_reference_t ref_space;
  utf8lex_reference_t ref_id2;
} utf8lex_test_declaration_t;

typedef struct _STRUCT_utf8lex_test_operand
{
  // OPERAND = NUMBER | ID
  utf8lex_multi_definition_t operand_definition;

  utf8lex_reference_t ref_number;
  utf8lex_reference_t ref_id;
} utf8lex_test_operand_t;

typedef struct _STRUCT_utf8lex_test_expression
{
  // EXPRESSION = DECLARATION OPERATOR OPERAND
  //     = ( ID SPACE ID ) ( EQUALS3 | EQUALS | PLUS | MINUS ) ( NUMBER | ID )
  utf8lex_multi_definition_t expression_definition;

  utf8lex_test_operator_t declaration;
  utf8lex_test_operator_t operator;
  utf8lex_test_operand_t operand;

  utf8lex_reference_t ref_declaration;
  utf8lex_reference_t ref_operator;
  utf8lex_reference_t ref_operand;
} utf8lex_test_expression_t;


// NUMBER, ID, EQUALS3, EQUALS, PLUS, MINUS, SPACE:
static utf8lex_error_t db_init(
        utf8lex_test_db_t *db
        )
{
  if (db == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_definition_t *prev_definition = NULL;
  utf8lex_rule_t *prev_rule = NULL;

  error = utf8lex_cat_definition_init(&(db->number_definition),
                                      prev_definition,  // prev
                                      "NUM",  // name
                                      UTF8LEX_GROUP_NUM,  // cat
                                      1,  // min
                                      -1);  // max
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(db->number_definition);
  error = utf8lex_rule_init(
                            &(db->number_rule), prev_rule, "NUMBER",
                            (utf8lex_definition_t *) &(db->number_definition),  // definition
                            "// Empty code",
                            (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_rule = &(db->number_rule);

  error = utf8lex_regex_definition_init(&(db->id_definition),
                                        prev_definition,  // prev
                                        "ID",  // name
                                        "[_\\p{L}][_\\p{L}\\p{N}]*");
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(db->id_definition);
  error = utf8lex_rule_init(
                            &(db->id_rule), prev_rule, "ID",
                            (utf8lex_definition_t *) &(db->id_definition),  // definition
                            "// Empty code",
                            (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_rule = &(db->id_rule);

  error = utf8lex_literal_definition_init(&(db->equals3_definition),
                                          prev_definition,  // prev
                                          "EQUALS3",  // name
                                          "===");
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(db->equals3_definition);
  error = utf8lex_rule_init(
      &(db->equals3_rule), prev_rule, "EQUALS3",
      (utf8lex_definition_t *) &(db->equals3_definition),  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_rule = &(db->equals3_rule);

  error = utf8lex_literal_definition_init(&(db->equals_definition),
                                          prev_definition,  // prev
                                          "EQUALS",  // name
                                          "=");
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(db->equals_definition);
  error = utf8lex_rule_init(
      &(db->equals_rule), prev_rule, "EQUALS",
      (utf8lex_definition_t *) &(db->equals_definition),  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_rule = &(db->equals_rule);

  error = utf8lex_literal_definition_init(&(db->plus_definition),
                                          prev_definition,  // prev
                                          "PLUS",  // name
                                          "+");
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(db->plus_definition);
  error = utf8lex_rule_init(
      &(db->plus_rule), prev_rule, "PLUS",
      (utf8lex_definition_t *) &(db->plus_definition),  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_rule = &(db->plus_rule);

  error = utf8lex_literal_definition_init(&(db->minus_definition),
                                          prev_definition,  // prev
                                          "MINUS",  // name
                                          "-");
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(db->minus_definition);
  error = utf8lex_rule_init(
      &(db->minus_rule), prev_rule, "MINUS",
      (utf8lex_definition_t *) &(db->minus_definition),  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_rule = &(db->minus_rule);

  error = utf8lex_regex_definition_init(&(db->space_definition),
                                        prev_definition,  // prev
                                        "SPACE",  // name
                                        "[\\s]+");
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &(db->space_definition);
  error = utf8lex_rule_init(
      &(db->space_rule), prev_rule, "SPACE",
      (utf8lex_definition_t *) &(db->space_definition),  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  prev_rule = &(db->space_rule);

  db->definitions_db = (utf8lex_definition_t *) &(db->number_definition);
  db->rules_db = &(db->number_rule);

  db->last_definition = prev_definition;
  db->last_rule = prev_rule;

  return UTF8LEX_OK;
}

static utf8lex_error_t db_clear(
        utf8lex_test_db_t *db
        )
{
  if (db == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;

  error = utf8lex_cat_definition_clear(&(db->number_definition.base));
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = utf8lex_rule_clear(&(db->number_rule));
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  error = utf8lex_regex_definition_clear(&(db->id_definition.base));
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = utf8lex_rule_clear(&(db->id_rule));
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  error = utf8lex_literal_definition_clear(&(db->equals3_definition.base));
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = utf8lex_rule_clear(&(db->equals3_rule));
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  error = utf8lex_literal_definition_clear(&(db->equals_definition.base));
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = utf8lex_rule_clear(&(db->equals_rule));
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  error = utf8lex_literal_definition_clear(&(db->plus_definition.base));
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = utf8lex_rule_clear(&(db->plus_rule));
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  error = utf8lex_literal_definition_clear(&(db->minus_definition.base));
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = utf8lex_rule_clear(&(db->minus_rule));
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  error = utf8lex_regex_definition_clear(&(db->space_definition.base));
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = utf8lex_rule_clear(&(db->space_rule));
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  db->definitions_db = NULL;
  db->rules_db = NULL;

  db->last_definition = NULL;
  db->last_rule = NULL;

  return UTF8LEX_OK;
}


static utf8lex_error_t find_prev(
        utf8lex_test_db_t *db,
        utf8lex_multi_definition_t *parent,
        utf8lex_definition_t **prev_definition_pointer
        )
{
  if (db == NULL
      || prev_definition_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (parent == NULL)
  {
    // Add to the toplevel definitions db.
    *prev_definition_pointer = db->last_definition;
  }
  else if (parent->db == NULL)
  {
    *prev_definition_pointer = NULL;
  }
  else
  {
    // Add to the parent multi-definition's own db.
    *prev_definition_pointer = parent->db;
    bool is_infinite_loop = true;
    for (int infinite_loop = 0;
         infinite_loop < UTF8LEX_DEFINITIONS_DB_LENGTH_MAX;
         infinite_loop ++)
    {
      if ((*prev_definition_pointer)->next == NULL)
      {
        is_infinite_loop = false;
        break;
      }

      *prev_definition_pointer = (*prev_definition_pointer)->next;
    }

    if (is_infinite_loop == true)
    {
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }
  }

  return UTF8LEX_OK;
}


// OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
static utf8lex_error_t create_operator(
        utf8lex_test_db_t *db,
        utf8lex_multi_definition_t *parent,  // Or NULL for toplevel definition.
        utf8lex_test_operator_t *operator
        )
{
  if (db == NULL
      || operator == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_definition_t *prev_definition = NULL;
  utf8lex_error_t error = UTF8LEX_OK;

  error = find_prev(db, parent, &prev_definition);
  if (error != UTF8LEX_OK) { return error; }

  printf("  Building multi-definition 'operator':\n");  fflush(stdout);
  error = utf8lex_multi_definition_init(
              &(operator->operator_definition),  // self
              prev_definition,  // prev
              "operator",  // name
              parent,  // parent
              UTF8LEX_MULTI_TYPE_OR);  // multi_type
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *)
    &(operator->operator_definition);
  if (parent == NULL)
  {
    db->last_definition = prev_definition;
  }

  printf("    Adding EQUALS3 | EQUALS | PLUS | MINUS:\n");  fflush(stdout);
  utf8lex_reference_t *prev_ref = NULL;
  error = utf8lex_reference_init(
              &(operator->ref_equals3),  // self
              prev_ref,  // prev
              db->equals3_definition.base.name,  // name
              1,  // min
              1,  // max
              &(operator->operator_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(operator->ref_equals3);
  error = utf8lex_reference_init(
              &(operator->ref_equals),  // self
              prev_ref,  // prev
              db->equals_definition.base.name,  // name
              1,  // min
              1,  // max
              &(operator->operator_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(operator->ref_equals);
  error = utf8lex_reference_init(
              &(operator->ref_plus),  // self
              prev_ref,  // prev
              db->plus_definition.base.name,  // name
              1,  // min
              1,  // max
              &(operator->operator_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(operator->ref_plus);
  error = utf8lex_reference_init(
              &(operator->ref_minus),  // self
              prev_ref,  // prev
              db->minus_definition.base.name,  // name
              1,  // min
              1,  // max
              &(operator->operator_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(operator->ref_minus);

  // Just test resolving one of the references, to detect
  // any bugs in reference-resolving before resolving
  // the whole multi-definition:
  printf("  Resolving reference 'EQUALS3':\n");  fflush(stdout);
  error = utf8lex_reference_resolve(
              &(operator->ref_equals3),  // self
              db->definitions_db);  // db
  if (error != UTF8LEX_OK) { return error; }

  printf("  Resolving multi-definition 'operator':\n");  fflush(stdout);
  error = utf8lex_multi_definition_resolve(
              &(operator->operator_definition),  // self
              db->definitions_db);  // db
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


// DECLARATION = ID SPACE ID
static utf8lex_error_t create_declaration(
        utf8lex_test_db_t *db,
        utf8lex_multi_definition_t *parent,  // Or NULL for toplevel definition.
        utf8lex_test_declaration_t *declaration
        )
{
  if (db == NULL
      || declaration == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_definition_t *prev_definition = NULL;
  utf8lex_error_t error = UTF8LEX_OK;

  error = find_prev(db, parent, &prev_definition);
  if (error != UTF8LEX_OK) { return error; }

  printf("  Building multi-definition 'declaration':\n");  fflush(stdout);
  error = utf8lex_multi_definition_init(
              &(declaration->declaration_definition),  // self
              prev_definition,  // prev
              "declaration",  // name
              parent,  // parent
              UTF8LEX_MULTI_TYPE_SEQUENCE);  // multi_type
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *)
    &(declaration->declaration_definition);
  if (parent == NULL)
  {
    db->last_definition = prev_definition;
  }

  printf("    Adding ID SPACE ID:\n");  fflush(stdout);
  utf8lex_reference_t *prev_ref = NULL;
  error = utf8lex_reference_init(
              &(declaration->ref_id1),  // self
              prev_ref,  // prev
              db->id_definition.base.name,  // name
              1,  // min
              1,  // max
              &(declaration->declaration_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(declaration->ref_id1);
  error = utf8lex_reference_init(
              &(declaration->ref_space),  // self
              prev_ref,  // prev
              db->space_definition.base.name,  // name
              1,  // min
              1,  // max
              &(declaration->declaration_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(declaration->ref_space);
  error = utf8lex_reference_init(
              &(declaration->ref_id2),  // self
              prev_ref,  // prev
              db->id_definition.base.name,  // name
              1,  // min
              1,  // max
              &(declaration->declaration_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(declaration->ref_id2);

  // Just test resolving one of the references, to detect
  // any bugs in reference-resolving before resolving
  // the whole multi-definition:
  printf("  Resolving reference 'ID':\n");  fflush(stdout);
  error = utf8lex_reference_resolve(
              &(declaration->ref_id1),  // self
              db->definitions_db);  // db
  if (error != UTF8LEX_OK) { return error; }

  printf("  Resolving multi-definition 'declaration':\n");  fflush(stdout);
  error = utf8lex_multi_definition_resolve(
              &(declaration->declaration_definition),  // self
              db->definitions_db);  // db
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


// OPERAND = NUMBER | ID
static utf8lex_error_t test_utf8lex_create_operand(
        utf8lex_test_db_t *db,
        utf8lex_multi_definition_t *parent,  // Or NULL for toplevel definition.
        utf8lex_test_operand_t *operand
        )
{
  if (db == NULL
      || operand == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_definition_t *prev_definition = NULL;
  utf8lex_error_t error = UTF8LEX_OK;

  error = find_prev(db, parent, &prev_definition);
  if (error != UTF8LEX_OK) { return error; }

  printf("  Building multi-definition 'operand':\n");  fflush(stdout);
  error = utf8lex_multi_definition_init(
              &(operand->operand_definition),  // self
              prev_definition,  // prev
              "operand",  // name
              parent,  // parent
              UTF8LEX_MULTI_TYPE_OR);  // multi_type
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *)
    &(operand->operand_definition);
  if (parent == NULL)
  {
    db->last_definition = prev_definition;
  }

  printf("    Adding NUMBER | ID:\n");  fflush(stdout);
  utf8lex_reference_t *prev_ref = NULL;
  error = utf8lex_reference_init(
              &(operand->ref_number),  // self
              prev_ref,  // prev
              db->number_definition.base.name,  // name
              1,  // min
              1,  // max
              &(operand->operand_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(operand->ref_number);
  error = utf8lex_reference_init(
              &(operand->ref_id),  // self
              prev_ref,  // prev
              db->id_definition.base.name,  // name
              1,  // min
              1,  // max
              &(operand->operand_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(operand->ref_id);

  // Just test resolving one of the references, to detect
  // any bugs in reference-resolving before resolving
  // the whole multi-definition:
  printf("  Resolving reference 'NUMBER':\n");  fflush(stdout);
  error = utf8lex_reference_resolve(
              &(operand->ref_number),  // self
              db->definitions_db);  // db
  if (error != UTF8LEX_OK) { return error; }

  printf("  Resolving multi-definition 'operand':\n");  fflush(stdout);
  error = utf8lex_multi_definition_resolve(
              &(operand->operand_definition),  // self
              db->definitions_db);  // db
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


static utf8lex_error_t init_state(
        utf8lex_state_t *state,
        utf8lex_buffer_t *buffer,
        utf8lex_string_t *str,
        unsigned char *bytes
        )
{
  utf8lex_error_t error = UTF8LEX_OK;

  size_t length_bytes = strlen(bytes);
  error = utf8lex_string_init(str,  // self
                              length_bytes,  // max_length_bytes
                              length_bytes,  // length_bytes
                              bytes);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_buffer_init(buffer,  // self
                              NULL,  // prev
                              str,  // str
                              true);  // is_eof
  if (error != UTF8LEX_OK) { return error; }

  utf8lex_settings_t settings;
  error = utf8lex_state_init(state,    // self
                             NULL,     // settings
                             buffer,   // buffer
                             0);       // stack_depth
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


static utf8lex_error_t clear_state(
        utf8lex_state_t *state
        )
{
  if (state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_buffer_t *buffer = state->buffer;
  utf8lex_string_t *str = state->buffer->str;

  error = utf8lex_state_clear(state);  // self
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_buffer_clear(buffer);  // self
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_string_clear(str);  // self
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}

static utf8lex_error_t expect_token(
        unsigned char *test_name,
        int recursion_depth,
        int sub_token_index,
        utf8lex_rule_t *expected_rule,
        utf8lex_definition_t *expected_definition,
        unsigned char *expected_content,
        int expected_start_byte,
        int expected_start_char,
        int expected_start_grapheme,
        int expected_start_line,
        int expected_length_byte,
        int expected_length_char,
        int expected_length_grapheme,
        int expected_length_line,
        utf8lex_token_t *actual_token
        )
{
  if (expected_rule == NULL
      || expected_definition == NULL
      || (recursion_depth == 0 && expected_content == NULL)
      || actual_token == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  int num_errors = 0;
  char expected_name[256];
  char actual_name[256];

  // Check rule:
  if (expected_rule != actual_token->rule)
  {
    if (expected_rule == NULL) { strcpy(expected_name, "NULL"); }
    else { strcpy(expected_name, expected_rule->name); }
    if (actual_token->rule == NULL) { strcpy(actual_name, "NULL"); }
    else { strcpy(actual_name, actual_token->rule->name); }
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect token at [%d.%d]: expected rule '%s', found rule '%s'\n",
            test_name,
            recursion_depth,
            sub_token_index,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_name,
            actual_name);
    num_errors ++;
  }

  // Check definition:
  if (expected_definition != actual_token->definition)
  {
    if (expected_definition == NULL) { strcpy(expected_name, "NULL"); }
    else { strcpy(expected_name, expected_definition->name); }
    if (actual_token->definition == NULL) { strcpy(actual_name, "NULL"); }
    else { strcpy(actual_name, actual_token->definition->name); }
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect token at [%d.%d]: expected definition '%s', found definition '%s'\n",
            test_name,
            recursion_depth,
            sub_token_index,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_name,
            actual_name);
    num_errors ++;
  }

  // Check location starts:
  if (expected_start_byte != actual_token->loc[UTF8LEX_UNIT_BYTE].start)
  {
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect %s token location at [%d.%d]: expected start byte %d, but found: %d\n",
            test_name,
            recursion_depth,
            sub_token_index,
            expected_definition->name,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_start_byte,
            actual_token->loc[UTF8LEX_UNIT_BYTE].start);
    num_errors ++;
  }
  if (expected_start_char != actual_token->loc[UTF8LEX_UNIT_CHAR].start)
  {
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect %s token location at [%d.%d]: expected start char %d, but found: %d\n",
            test_name,
            recursion_depth,
            sub_token_index,
            expected_definition->name,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_start_char,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start);
    num_errors ++;
  }
  if (expected_start_grapheme != actual_token->loc[UTF8LEX_UNIT_GRAPHEME].start)
  {
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect %s token location at [%d.%d]: expected start grapheme %d, but found: %d\n",
            test_name,
            recursion_depth,
            sub_token_index,
            expected_definition->name,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_start_grapheme,
            actual_token->loc[UTF8LEX_UNIT_GRAPHEME].start);
    num_errors ++;
  }
  if (expected_start_line != actual_token->loc[UTF8LEX_UNIT_LINE].start)
  {
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect %s token location at [%d.%d]: expected start line %d, but found: %d\n",
            test_name,
            recursion_depth,
            sub_token_index,
            expected_definition->name,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_start_line,
            actual_token->loc[UTF8LEX_UNIT_LINE].start);
    num_errors ++;
  }

  // Check location lengths:
  if (expected_length_byte >= 0
      && expected_length_byte != actual_token->loc[UTF8LEX_UNIT_BYTE].length)
  {
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect %s token location at [%d.%d]: expected byte length %d, but found: %d\n",
            test_name,
            recursion_depth,
            sub_token_index,
            expected_definition->name,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_length_byte,
            actual_token->loc[UTF8LEX_UNIT_BYTE].length);
    num_errors ++;
  }
  if (expected_length_char >= 0
      && expected_length_char != actual_token->loc[UTF8LEX_UNIT_CHAR].length)
  {
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect %s token location at [%d.%d]: expected char length %d, but found: %d\n",
            test_name,
            recursion_depth,
            sub_token_index,
            expected_definition->name,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_length_char,
            actual_token->loc[UTF8LEX_UNIT_CHAR].length);
    num_errors ++;
  }
  if (expected_length_grapheme >= 0
      && expected_length_grapheme != actual_token->loc[UTF8LEX_UNIT_GRAPHEME].length)
  {
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect %s token location at [%d.%d]: expected grapheme length %d, but found: %d\n",
            test_name,
            recursion_depth,
            sub_token_index,
            expected_definition->name,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_length_grapheme,
            actual_token->loc[UTF8LEX_UNIT_GRAPHEME].length);
    num_errors ++;
  }
  if (expected_length_line >= 0
      && expected_length_line != actual_token->loc[UTF8LEX_UNIT_LINE].length)
  {
    fprintf(stderr, "ERROR %s [%d][%d]: Incorrect %s token location at [%d.%d]: expected line length %d, but found: %d\n",
            test_name,
            recursion_depth,
            sub_token_index,
            expected_definition->name,
            actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
            actual_token->loc[UTF8LEX_UNIT_CHAR].start,
            expected_length_line,
            actual_token->loc[UTF8LEX_UNIT_LINE].length);
    num_errors ++;
  }

  // Check content:
  if (expected_content != NULL)
  {
    int strcmp;
    if ((expected_content == NULL && actual_token->str != NULL)
        || (expected_content != NULL && actual_token->str == NULL)
        || utf8lex_strcmp(actual_token->str, actual_token->loc,
                          expected_content, &strcmp) != UTF8LEX_OK
        || strcmp != 0)
    {
      if (expected_content == NULL) { strcpy(expected_name, "NULL"); }
      else { strcpy(expected_name, expected_content); }
      utf8lex_error_t error =
        utf8lex_strcpy(actual_token->str, actual_token->loc, actual_name);
      if (error != UTF8LEX_OK)
      {
        return error;
      }
      fprintf(stderr, "ERROR %s [%d][%d]: Incorrect %s token at [%d.%d]: expected content '%s', found content '%s'\n",
              test_name,
              recursion_depth,
              sub_token_index,
              expected_definition->name,
              actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
              actual_token->loc[UTF8LEX_UNIT_CHAR].start,
              expected_name,
              actual_name);
      num_errors ++;
    }
  }

  // If the rule has a sequence multi-definition, then check sub-tokens.
  // (Or multi-definitions do not have sub-tokens.)
  utf8lex_definition_t *top_definition = actual_token->rule->definition;
  if (top_definition->definition_type == UTF8LEX_DEFINITION_TYPE_MULTI
      && ((utf8lex_multi_definition_t *) top_definition)->multi_type == UTF8LEX_MULTI_TYPE_SEQUENCE)
  {
    utf8lex_multi_definition_t *multi = (utf8lex_multi_definition_t *)
      top_definition;
    // Can go into an infinite loop via recursion.  :/
    bool is_infinite_loop = true;
    utf8lex_reference_t *reference = multi->references;
    utf8lex_location_t sub_loc[UTF8LEX_UNIT_MAX] = {
      { .start = 0, .length = 0, .after = 0, .hash = 0 },  // byte
      { .start = 0, .length = 0, .after = 0, .hash = 0 },  // char
      { .start = 0, .length = 0, .after = 0, .hash = 0 },  // grapheme
      { .start = 0, .length = 0, .after = 0, .hash = 0 }  // line
    };
    int num_sub_tokens = 0;
    for (int infinite_loop = 0;
         infinite_loop < UTF8LEX_REFERENCES_LENGTH_MAX;
         infinite_loop ++)
    {
      if (reference == NULL)
      {
        is_infinite_loop = false;
        break;
      }

      int nth = 0;
      utf8lex_reference_t *maybe_same_name = multi->references;
      for (int find_nth = 0; find_nth < infinite_loop; find_nth ++)
      {
        if (strcmp(reference->definition_name,
                   maybe_same_name->definition_name) == 0)
        {
          nth ++;
        }
        maybe_same_name = maybe_same_name->next;
      }

      utf8lex_definition_t *sub_definition = reference->definition_or_null;
      utf8lex_sub_token_t *found_sub_token;
      utf8lex_error_t error = utf8lex_sub_token_find(
          actual_token->sub_tokens,  // first_sub_token
          sub_definition->name,  // name
          nth,  // index
          &found_sub_token);  // found_pointer
      // A "sequence" multi-definition can have 2 or more matched definitions
      // with the same name in the sequence, such as "ID OPERATOR ID".
      // An "or" multi-definition can only have exactly 1 matched definition.
      // After the toplevel recursion, we stop trying to track the name
      // that an "or" resolves to, because we're tired.
      // This test framework is kludgy.  You should fix it!
      if (recursion_depth > 0  // Don't pay attention to "or"s in nested multis
          || multi->multi_type == UTF8LEX_MULTI_TYPE_SEQUENCE  // Find all subs
          || (multi->multi_type == UTF8LEX_MULTI_TYPE_OR  // Only find 1 sub
              && sub_definition == expected_definition))  // Only find 1 sub
      {
        // We should be able to find this sub-token.
        if (error != UTF8LEX_OK)
        {
          fprintf(stderr, "ERROR %s [%d][%d]: Can't find token \"%s\"'s expected \"%s\" sub-token # %d at [%d.%d]: '%s'\n",
                  test_name,
                  recursion_depth,
                  sub_token_index,
                  expected_definition->name,
                  sub_definition->name,
                  nth,
                  actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
                  actual_token->loc[UTF8LEX_UNIT_CHAR].start,
                  sub_definition->name);
          num_errors ++;
          reference = reference->next;
          continue;
        }

        sub_loc[UTF8LEX_UNIT_BYTE].start =
          found_sub_token->token.loc[UTF8LEX_UNIT_BYTE].start
          + found_sub_token->token.loc[UTF8LEX_UNIT_BYTE].length;
        sub_loc[UTF8LEX_UNIT_CHAR].start =
          found_sub_token->token.loc[UTF8LEX_UNIT_CHAR].start
          + found_sub_token->token.loc[UTF8LEX_UNIT_CHAR].length;
        sub_loc[UTF8LEX_UNIT_GRAPHEME].start =
          found_sub_token->token.loc[UTF8LEX_UNIT_GRAPHEME].start
          + found_sub_token->token.loc[UTF8LEX_UNIT_GRAPHEME].length;
        sub_loc[UTF8LEX_UNIT_LINE].start =
          found_sub_token->token.loc[UTF8LEX_UNIT_LINE].start
          + found_sub_token->token.loc[UTF8LEX_UNIT_LINE].length;
        num_sub_tokens ++;
        if (error != UTF8LEX_OK)
        {
          // We've already printed error(s) while recursing, just count.
          num_errors ++;
        }
      }
      else
      {
        // We should not be able to find this sub-token.
        if (error == UTF8LEX_OK)
        {
          fprintf(stderr, "ERROR %s [%d][%d]: found token \"%s\"'s unexpected sub-token at [%d.%d]: '%s'\n",
                  test_name,
                  recursion_depth,
                  sub_token_index,
                  expected_definition->name,
                  found_sub_token->token.loc[UTF8LEX_UNIT_LINE].start + 1,
                  found_sub_token->token.loc[UTF8LEX_UNIT_CHAR].start,
                  sub_definition->name);
          num_errors ++;
        }
      }

      reference = reference->next;
    }

    if (expected_length_byte >= 0
        && (expected_start_byte + expected_length_byte)
        != sub_loc[UTF8LEX_UNIT_BYTE].start)
    {
      fprintf(stderr, "ERROR %s [%d][%d]: total byte length of token \"%s\"'s %d sub-tokens does not match expected byte length of token at [%d.%d]: expected %d but found %d\n",
              test_name,
              recursion_depth,
              sub_token_index,
              expected_definition->name,
              num_sub_tokens,
              actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
              actual_token->loc[UTF8LEX_UNIT_CHAR].start,
              expected_start_byte + expected_length_byte,
              sub_loc[UTF8LEX_UNIT_BYTE].start);
      num_errors ++;
    }
    if (expected_length_char >= 0
        && (expected_start_char + expected_length_char)
        != sub_loc[UTF8LEX_UNIT_CHAR].start)
    {
      fprintf(stderr, "ERROR %s [%d][%d]: total char length of token \"%s\"'s %d sub-tokens does not match expected char length of token at [%d.%d]: expected %d but found %d\n",
              test_name,
              recursion_depth,
              sub_token_index,
              expected_definition->name,
              num_sub_tokens,
              actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
              actual_token->loc[UTF8LEX_UNIT_CHAR].start,
              expected_start_char + expected_length_char,
              sub_loc[UTF8LEX_UNIT_CHAR].start);
      num_errors ++;
    }
    if (expected_length_grapheme >= 0
        && (expected_start_grapheme + expected_length_grapheme)
        != sub_loc[UTF8LEX_UNIT_GRAPHEME].start)
    {
      fprintf(stderr, "ERROR %s [%d][%d]: total grapheme length of token \"%s\"'s %d sub-tokens does not match expected grapheme length of token at [%d.%d]: expected %d but found %d\n",
              test_name,
              recursion_depth,
              sub_token_index,
              expected_definition->name,
              num_sub_tokens,
              actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
              actual_token->loc[UTF8LEX_UNIT_CHAR].start,
              expected_start_grapheme + expected_length_grapheme,
              sub_loc[UTF8LEX_UNIT_GRAPHEME].start);
      num_errors ++;
    }
    if (expected_length_line >= 0
        && (expected_start_line + expected_length_line)
        != sub_loc[UTF8LEX_UNIT_LINE].start)
    {
      fprintf(stderr, "ERROR %s [%d][%d]: total line length of token \"%s\"'s %d sub-tokens does not match expected line length of token at [%d.%d]: expected %d but found %d\n",
              test_name,
              recursion_depth,
              sub_token_index,
              expected_definition->name,
              num_sub_tokens,
              actual_token->loc[UTF8LEX_UNIT_LINE].start + 1,
              actual_token->loc[UTF8LEX_UNIT_CHAR].start,
              expected_start_line + expected_length_line,
              sub_loc[UTF8LEX_UNIT_LINE].start);
      num_errors ++;
    }

    if (is_infinite_loop == true)
    {
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }
  }

  if (num_errors > 0)
  {
    fflush(stderr);
    return UTF8LEX_NO_MATCH;
  }

  return UTF8LEX_OK;
}

// OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
static utf8lex_error_t test_utf8lex_operator()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_test_db_t db;
  error = db_init(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  utf8lex_test_operator_t operator;
  error = create_operator(
              &db,  // db
              NULL,  // parent
              &operator);  // operator
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Now we'll try lexing with the 'operator' definition.
  utf8lex_state_t state;
  utf8lex_buffer_t buffer;
  utf8lex_string_t str;
  unsigned char *to_lex;

  utf8lex_rule_t lex_rule;
  utf8lex_token_t token;

  printf("  Setting up rule for 'operator' definition:\n");
  error = utf8lex_rule_init(&lex_rule,  // self
                            NULL,  // prev
                            "operator",  // name
                            (utf8lex_definition_t *)
                            &(operator.operator_definition),  // definition
                            "return $$;",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "===";  // EQUALS3
  printf("  Lexing '%s' with 'operator' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" OK\n");
    fflush(stdout);
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" FAILED - no match\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  // Check the token and sub-tokens:
  // OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
  // Matched EQUALS3
  {
    error =
      expect_token("test_utf8lex_operator()",  // test_name
                   0,  // recursion_depth
                   0,  // sub_token_index
                   &lex_rule,  // expected_rule
                   operator.ref_equals3.definition_or_null,  // expected_definition
                   to_lex,  // expected_content
                   0, 0, 0, 0,  // expected_start_[byte, char, grapheme, line]
                   3, 3, 3, 0,  // expected_length_[byte, char, grapheme, line]
                   &token);  // actual_token
    if (error != UTF8LEX_OK) { return error; }
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "=";  // EQUALS
  printf("  Lexing '%s' with 'operator' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" OK\n");
    fflush(stdout);
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" FAILED - no match\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  // Check the token and sub-tokens:
  // OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
  // Matched EQUALS
  {
    error =
      expect_token("test_utf8lex_operator()",  // test_name
                   0,  // recursion_depth
                   0,  // sub_token_index
                   &lex_rule,  // expected_rule
                   operator.ref_equals.definition_or_null,  // expected_definition
                   to_lex,  // expected_content
                   0, 0, 0, 0,  // expected_start_[byte, char, grapheme, line]
                   1, 1, 1, 0,  // expected_length_[byte, char, grapheme, line]
                   &token);  // actual_token
    if (error != UTF8LEX_OK) { return error; }
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "+";  // PLUS
  printf("  Lexing '%s' with 'operator' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" OK\n");
    fflush(stdout);
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" FAILED - no match\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  // Check the token and sub-tokens:
  // OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
  // Matched PLUS
  {
    error =
      expect_token("test_utf8lex_operator()",  // test_name
                   0,  // recursion_depth
                   0,  // sub_token_index
                   &lex_rule,  // expected_rule
                   operator.ref_plus.definition_or_null,  // expected_definition
                   to_lex,  // expected_content
                   0, 0, 0, 0,  // expected_start_[byte, char, grapheme, line]
                   1, 1, 1, 0,  // expected_length_[byte, char, grapheme, line]
                   &token);  // actual_token
    if (error != UTF8LEX_OK) { return error; }
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "-";  // MINUS
  printf("  Lexing '%s' with 'operator' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" OK\n");
    fflush(stdout);
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" FAILED - no match\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  // Check the token and sub-tokens:
  // OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
  // Matched MINUS
  {
    error =
      expect_token("test_utf8lex_operator()",  // test_name
                   0,  // recursion_depth
                   0,  // sub_token_index
                   &lex_rule,  // expected_rule
                   operator.ref_minus.definition_or_null,  // expected_definition
                   to_lex,  // expected_content
                   0, 0, 0, 0,  // expected_start_[byte, char, grapheme, line]
                   1, 1, 1, 0,  // expected_length_[byte, char, grapheme, line]
                   &token);  // actual_token
    if (error != UTF8LEX_OK) { return error; }
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "foobar";  // no match.
  printf("  Lexing '%s' with 'operator' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" FAILED - should not match but did\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" OK - unmatched as expected\n");
    fflush(stdout);
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }


  printf("  Tearing down 'operator' rule and definition:\n");
  error = utf8lex_rule_clear(&lex_rule);
  if (error != UTF8LEX_OK) { return error; }

  // Clean up the db:
  error = db_clear(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


// DECLARATION = ID SPACE ID
static utf8lex_error_t test_utf8lex_declaration()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_test_db_t db;
  error = db_init(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  utf8lex_test_declaration_t declaration;
  error = create_declaration(
              &db,  // db
              NULL,  // parent
              &declaration);  // declaration
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Now we'll try lexing with the 'declaration' definition.
  utf8lex_state_t state;
  utf8lex_buffer_t buffer;
  utf8lex_string_t str;
  unsigned char *to_lex;

  utf8lex_rule_t lex_rule;
  utf8lex_token_t token;
  utf8lex_token_t expected_token;

  printf("  Setting up rule for 'declaration' definition:\n");
  error = utf8lex_rule_init(&lex_rule,  // self
                            NULL,  // prev
                            "declaration",  // name
                            (utf8lex_definition_t *)
                            &(declaration.declaration_definition), // definition
                            "return $$;",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "int   count";  // ID SPACE ID
  printf("  Lexing '%s' with 'declaration' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" OK\n");
    fflush(stdout);
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" FAILED - no match\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  // Check the token and sub-tokens:
  // DECLARATION = ID SPACE ID
  // Matched the whole sequence.
  {
    error =
      expect_token("test_utf8lex_declaration()",  // test_name
                   0,  // recursion_depth
                   0,  // sub_token_index
                   &lex_rule,  // expected_rule
                   (utf8lex_definition_t *)
                   &(declaration.declaration_definition),  // expected_definition
                   to_lex,  // expected_content
                   0, 0, 0, 0,  // expected_start_[byte, char, grapheme, line]
                   11, 11, 11, 0,  // expected_length_[byte, char, grapheme, line]
                   &token);  // actual_token
    if (error != UTF8LEX_OK) { return error; }
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "foobar";  // no match.
  printf("  Lexing '%s' with 'declaration' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" FAILED - should not match but did\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" OK - unmatched as expected\n");
    fflush(stdout);
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }


  printf("  Tearing down 'declaration' rule and definition:\n");
  error = utf8lex_rule_clear(&lex_rule);
  if (error != UTF8LEX_OK) { return error; }

  // Clean up the db:
  error = db_clear(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


// OPERAND = NUMBER | ID
static utf8lex_error_t test_utf8lex_operand()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_test_db_t db;
  error = db_init(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  utf8lex_test_operand_t operand;
  error = test_utf8lex_create_operand(
              &db,  // db
              NULL,  // parent
              &operand);  // operand
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Now we'll try lexing with the 'operand' definition.
  utf8lex_state_t state;
  utf8lex_buffer_t buffer;
  utf8lex_string_t str;
  unsigned char *to_lex;

  utf8lex_rule_t lex_rule;
  utf8lex_token_t token;
  utf8lex_token_t expected_token;

  printf("  Setting up rule for 'operand' definition:\n");
  error = utf8lex_rule_init(&lex_rule,  // self
                            NULL,  // prev
                            "operand",  // name
                            (utf8lex_definition_t *)
                            &(operand.operand_definition),  // definition
                            "return $$;",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "1132";  // NUMBER
  printf("  Lexing '%s' with 'operand' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" OK\n");
    fflush(stdout);
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" FAILED - no match\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  // Check the token and sub-tokens:
  // OPERAND = NUMBER | ID
  // Matched NUMBER.
  {
    error =
      expect_token("test_utf8lex_operand()",  // test_name
                   0,  // recursion_depth
                   0,  // sub_token_index
                   &lex_rule,  // expected_rule
                   operand.ref_number.definition_or_null,  // expected_definition
                   to_lex,  // expected_content
                   0, 0, 0, 0,  // expected_start_[byte, char, grapheme, line]
                   4, 4, 4, 0,  // expected_length_[byte, char, grapheme, line]
                   &token);  // actual_token
    if (error != UTF8LEX_OK) { return error; }
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "xyz";  // ID
  printf("  Lexing '%s' with 'operand' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" OK\n");
    fflush(stdout);
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" FAILED - no match\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  // Check the token and sub-tokens:
  // OPERAND = NUMBER | ID
  // Matched ID.
  {
    error =
      expect_token("test_utf8lex_operand()",  // test_name
                   0,  // recursion_depth
                   0,  // sub_token_index
                   &lex_rule,  // expected_rule
                   operand.ref_id.definition_or_null,  // expected_definition
                   to_lex,  // expected_content
                   0, 0, 0, 0,  // expected_start_[byte, char, grapheme, line]
                   3, 3, 3, 0,  // expected_length_[byte, char, grapheme, line]
                   &token);  // actual_token
    if (error != UTF8LEX_OK) { return error; }
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "+";  // no match.
  printf("  Lexing '%s' with 'operand' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" FAILED - should not match but did\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" OK - unmatched as expected\n");
    fflush(stdout);
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }


  printf("  Tearing down 'operand' rule and definition:\n");
  error = utf8lex_rule_clear(&lex_rule);
  if (error != UTF8LEX_OK) { return error; }

  // Clean up the db:
  error = db_clear(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


// EXPRESSION = DECLARATION OPERATOR OPERAND
//     = ( ID SPACE ID ) ( EQUALS3 | EQUALS | PLUS | MINUS ) ( NUMBER | ID )
static utf8lex_error_t test_utf8lex_expression()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_test_db_t db;
  error = db_init(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  utf8lex_test_expression_t expression;

  printf("  Building multi-definition 'expression':\n");  fflush(stdout);
  error = utf8lex_multi_definition_init(
              &(expression.expression_definition),  // self
              db.last_definition,  // prev
              "expression",  // name
              NULL,  // parent
              UTF8LEX_MULTI_TYPE_SEQUENCE);  // multi_type
  if (error != UTF8LEX_OK) { return error; }
  db.last_definition = (utf8lex_definition_t *)
    &(expression.expression_definition);

  // Nest the child multi-definitions inside expression:
  // DECLARATION = ID SPACE ID
  utf8lex_test_declaration_t declaration;
  error = create_declaration(
              &db,  // db
              &(expression.expression_definition),  // parent
              &declaration);  // declaration
  if (error != UTF8LEX_OK) { return error; }
  // OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
  utf8lex_test_operator_t operator;
  error = create_operator(
              &db,  // db
              &(expression.expression_definition),  // parent
              &operator);  // operator
  if (error != UTF8LEX_OK) { return error; }
  // OPERAND = EQUALS3 | EQUALS | PLUS | MINUS
  utf8lex_test_operand_t operand;
  error = test_utf8lex_create_operand(
              &db,  // db
              &(expression.expression_definition),  // parent
              &operand);  // operand
  if (error != UTF8LEX_OK) { return error; }

  // Now refer to the child multi-definitions (DECLARATION, OPERATOR, OPERAND):
  utf8lex_reference_t *prev_ref = NULL;
  printf("    Referring to DECLARATION:\n");  fflush(stdout);
  error = utf8lex_reference_init(
              &(expression.ref_declaration),  // self
              prev_ref,  // prev
              declaration.declaration_definition.base.name,  // name
              1,  // min
              1,  // max
              &(expression.expression_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(expression.ref_declaration);
  printf("    Referring to OPERATOR:\n");  fflush(stdout);
  error = utf8lex_reference_init(
              &(expression.ref_operator),  // self
              prev_ref,  // prev
              operator.operator_definition.base.name,  // name
              1,  // min
              1,  // max
              &(expression.expression_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(expression.ref_operator);
  printf("    Referring to OPERAND:\n");  fflush(stdout);
  error = utf8lex_reference_init(
              &(expression.ref_operand),  // self
              prev_ref,  // prev
              operand.operand_definition.base.name,  // name
              1,  // min
              1,  // max
              &(expression.expression_definition));  // parent
  if (error != UTF8LEX_OK) { return error; }
  prev_ref = &(expression.ref_operand);

  printf("  Resolving multi-definition 'expression':\n");  fflush(stdout);
  error = utf8lex_multi_definition_resolve(
              &(expression.expression_definition),  // self
              db.definitions_db);  // db
  if (error != UTF8LEX_OK) { return error; }

  // Now we'll try lexing with the 'expression' definition.
  utf8lex_state_t state;
  utf8lex_buffer_t buffer;
  utf8lex_string_t str;
  unsigned char *to_lex;

  utf8lex_rule_t lex_rule;
  utf8lex_token_t token;
  utf8lex_token_t expected_token;

  printf("  Setting up rule for 'expression' definition:\n");
  error = utf8lex_rule_init(&lex_rule,  // self
                            NULL,  // prev
                            "expression",  // name
                            (utf8lex_definition_t *)
                            &(expression.expression_definition),  // definition
                            "return $$;",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "string     first_name===42";  // DECLARATION OPERATOR OPERAND
  printf("  Lexing '%s' with 'expression' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" OK\n");
    fflush(stdout);
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" FAILED - no match\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  // Check the token and sub-tokens:
  // EXPRESSION = DECLARATION OPERATOR OPERAND
  //     = ( ID SPACE ID ) ( EQUALS3 | EQUALS | PLUS | MINUS ) ( NUMBER | ID )
  // Matched DECLARATION OPERATOR OPERAND.
  {
    error =
      expect_token("test_utf8lex_expression()",  // test_name
                   0,  // recursion_depth
                   0,  // sub_token_index
                   &lex_rule,  // expected_rule
                   (utf8lex_definition_t *)
                   &(expression.expression_definition),  // expected_definition
                   to_lex,  // expected_content
                   0, 0, 0, 0,  // expected_start_[byte, char, grapheme, line]
                   26, 26, 26, 0,  // expected_length_[byte, char, grapheme, line]
                   &token);  // actual_token
    if (error != UTF8LEX_OK) { return error; }
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "int counter+999";  // DECLARATION OPERATOR OPERAND
  printf("  Lexing '%s' with 'expression' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" OK\n");
    fflush(stdout);
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" FAILED - no match\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  // Check the token and sub-tokens:
  // EXPRESSION = DECLARATION OPERATOR OPERAND
  //     = ( ID SPACE ID ) ( EQUALS3 | EQUALS | PLUS | MINUS ) ( NUMBER | ID )
  // Matched DECLARATION OPERATOR OPERAND.
  {
    error =
      expect_token("test_utf8lex_expression()",  // test_name
                   0,  // recursion_depth
                   0,  // sub_token_index
                   &lex_rule,  // expected_rule
                   (utf8lex_definition_t *)
                   &(expression.expression_definition),  // expected_definition
                   to_lex,  // expected_content
                   0, 0, 0, 0,  // expected_start_[byte, char, grapheme, line]
                   15, 15, 15, 0,  // expected_length_[byte, char, grapheme, line]
                   &token);  // actual_token
    if (error != UTF8LEX_OK) { return error; }
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  to_lex = "foobar +";  // no match.
  printf("  Lexing '%s' with 'expression' definition:\n",
         to_lex);
  error == init_state(&state,  // state
                      &buffer,  // buffer
                      &str,  // str
                      to_lex);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_lex(&lex_rule,  // first_rule
                      &state,  // state
                      &token);
  if (error == UTF8LEX_OK) {
    printf(" FAILED - should not match but did\n");
    fflush(stdout);
    return UTF8LEX_ERROR_STATE;
  }
  else if (error == UTF8LEX_NO_MATCH) {
    printf(" OK - unmatched as expected\n");
    fflush(stdout);
  }
  else
  {
    printf(" FAILED\n");
    fflush(stdout);
    return error;
  }

  error = clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }


  printf("  Tearing down 'expression' rule and definition:\n");
  error = utf8lex_rule_clear(&lex_rule);
  if (error != UTF8LEX_OK) { return error; }

  // Clean up the db:
  error = db_clear(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


int main(
        int argc,
        char *argv[]
        )
{
  printf("Testing utf8lex_token...\n");  fflush(stdout);

  // Test a logical "OR" multi-definition:
  // OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
  utf8lex_error_t error = test_utf8lex_operator();

  // Test a sequence multi-definition:
  // DECLARATION = ID SPACE ID
  if (error == UTF8LEX_OK)
  {
    error = test_utf8lex_declaration();
  } // else if (error != UTF8LEX_OK) then fall through, below.

  // Test another logical 'OR' multi-definition:
  // OPERAND = NUMBER | ID
  if (error == UTF8LEX_OK)
  {
    error = test_utf8lex_operand();
  } // else if (error != UTF8LEX_OK) then fall through, below.

  // Test a sequence of OR'ed sub-multi-definitions:
  // EXPRESSION = DECLARATION OPERATOR OPERAND
  //     = ( ID SPACE ID ) ( EQUALS3 | EQUALS | PLUS | MINUS ) ( NUMBER | ID )
  if (error == UTF8LEX_OK)
  {
    error = test_utf8lex_expression();
  } // else if (error != UTF8LEX_OK) then fall through, below.

  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS Testing utf8lex_token.\n");  fflush(stdout);
    fflush(stdout);
    fflush(stderr);
    return 0;
  }
  else
  {
    char error_bytes[256];
    utf8lex_string_t error_string;
    utf8lex_string_init(&error_string,
                        256,  // max_length_bytes
                        0,  // length_bytes
                        &error_bytes[0]);
    utf8lex_error_string(&error_string,
                         error);

    fprintf(stderr,
            "ERROR test_utf8lex_token: Failed with error code: %d %s [%d]\n",
            (int) error,
            error_string.bytes);

    fflush(stdout);
    fflush(stderr);
    return (int) error;
  }
}
