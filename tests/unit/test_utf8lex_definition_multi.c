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
} utf8lex_test_expression_t;


// NUMBER, ID, EQUALS3, EQUALS, PLUS, MINUS, SPACE:
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
  db->rules_db = &(db->number_rule);

  db->last_definition = prev_definition;
  db->last_rule = prev_rule;

  return UTF8LEX_OK;
}


static utf8lex_error_t test_utf8lex_find_prev(
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
static utf8lex_error_t test_utf8lex_create_operator(
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

  error = test_utf8lex_find_prev(db, parent, &prev_definition);
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
static utf8lex_error_t test_utf8lex_create_declaration(
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

  error = test_utf8lex_find_prev(db, parent, &prev_definition);
  if (error != UTF8LEX_OK) { return error; }

  printf("  Building multi-definition 'declaration':\n");  fflush(stdout);
  error = utf8lex_multi_definition_init(
              &(declaration->declaration_definition),  // self
              prev_definition,  // prev
              "declaration",  // name
              NULL,  // parent
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

  error = test_utf8lex_find_prev(db, parent, &prev_definition);
  if (error != UTF8LEX_OK) { return error; }

  printf("  Building multi-definition 'operand':\n");  fflush(stdout);
  error = utf8lex_multi_definition_init(
              &(operand->operand_definition),  // self
              prev_definition,  // prev
              "operand",  // name
              NULL,  // parent
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


// OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
static utf8lex_error_t test_utf8lex_operator()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_test_db_t db;
  error = test_utf8lex_create_db(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  utf8lex_test_operator_t operator;
  error = test_utf8lex_create_operator(
              &db,  // db
              NULL,  // parent
              &operator);  // operator
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


// DECLARATION = ID SPACE ID
static utf8lex_error_t test_utf8lex_declaration()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_test_db_t db;
  error = test_utf8lex_create_db(&db);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  utf8lex_test_declaration_t declaration;
  error = test_utf8lex_create_declaration(
              &db,  // db
              NULL,  // parent
              &declaration);  // declaration
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  printf("  Clearing multi-definition 'declaration':\n");  fflush(stdout);
  error = utf8lex_multi_definition_clear(
              (utf8lex_definition_t *) &(declaration.declaration_definition));  // self
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


// OPERAND = NUMBER | ID
static utf8lex_error_t test_utf8lex_operand()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_test_db_t db;
  error = test_utf8lex_create_db(&db);
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

  printf("  Clearing multi-definition 'operand':\n");  fflush(stdout);
  error = utf8lex_multi_definition_clear(
              (utf8lex_definition_t *) &(operand.operand_definition));  // self
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


// EXPRESSION = DECLARATION OPERATOR OPERAND
//     = ( ID SPACE ID ) ( EQUALS3 | EQUALS | PLUS | MINUS ) ( NUMBER | ID )
static utf8lex_error_t test_utf8lex_expression()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_test_db_t db;
  error = test_utf8lex_create_db(&db);
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
  error = test_utf8lex_create_declaration(
              &db,  // db
              &(expression.expression_definition),  // parent
              &declaration);  // declaration
  if (error != UTF8LEX_OK) { return error; }
  // OPERATOR = EQUALS3 | EQUALS | PLUS | MINUS
  utf8lex_test_operator_t operator;
  error = test_utf8lex_create_operator(
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

  printf("  Resolving multi-definition 'expression':\n");  fflush(stdout);
  error = utf8lex_multi_definition_resolve(
              &(expression.expression_definition),  // self
              db.definitions_db);  // db
  if (error != UTF8LEX_OK) { return error; }

  printf("  Clearing multi-definition 'expression':\n");  fflush(stdout);
  error = utf8lex_multi_definition_clear(
              (utf8lex_definition_t *) &(expression.expression_definition));  // self
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


int main(
        int argc,
        char *argv[]
        )
{
  printf("Testing utf8lex_definition_multi...\n");  fflush(stdout);

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
