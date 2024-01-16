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
#include <string.h>  // For strcpy()
#include <unistd.h>  // For read()

#include "utf8lex.h"

typedef struct _STRUCT_utf8lex_test_db
{
  utf8lex_definition_t *definitions_db;
  utf8lex_rule_t *rules_db;

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

typedef struct _STRUCT_utf8lex_test_multi_operator
{
  // OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
  utf8lex_multi_definition_t operator_definition;

  utf8lex_reference_t ref_equals3;
  utf8lex_reference_t ref_equals;
  utf8lex_reference_t ref_plus;
  utf8lex_reference_t ref_minus;
} utf8lex_test_operator_t;


static utf8lex_error_t test_utf8lex_create_db(
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
  db->rules_db = (utf8lex_rule_t *) &(db->number_rule);

  return UTF8LEX_OK;
}


static utf8lex_error_t test_utf8lex_create_operator(
        utf8lex_test_db_t *db,
        utf8lex_test_operator_t *operator
        )
{
  if (db == NULL
      || operator == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;

  printf("  Building multi-definition 'operator':\n");  fflush(stdout);
  error = utf8lex_multi_definition_init(
              &(operator->operator_definition),  // self
              (utf8lex_definition_t *) &(db->space_definition),  // prev
              "operator",  // name
              NULL,  // parent
              UTF8LEX_MULTI_TYPE_OR);  // multi_type
  if (error != UTF8LEX_OK) { return error; }

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


static utf8lex_error_t test_utf8lex_multi_definitions()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_test_db_t db;
  error = test_utf8lex_create_db(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  utf8lex_test_operator_t operator;
  error = test_utf8lex_create_operator(&db, &operator);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  printf("  Clearing multi-definition 'operator':\n");  fflush(stdout);
  error = utf8lex_multi_definition_clear(
              (utf8lex_definition_t *) &(operator.operator_definition));  // self
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


int main(
        int argc,
        char *argv[]
        )
{
  printf("Testing utf8lex_definition_multi...\n");  fflush(stdout);

  utf8lex_error_t error = test_utf8lex_multi_definitions();

  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS Testing utf8lex_definition_multi.\n");  fflush(stdout);
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
            "ERROR test_utf8lex_definition_multi: Failed with error code: %d %s\n",
            (int) error,
            error_string.bytes);

    fflush(stdout);
    fflush(stderr);
    return (int) error;
  }
}
