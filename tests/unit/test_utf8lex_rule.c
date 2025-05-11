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

#include "utf8lex.h"


utf8lex_error_t test_utf8lex_rule_found(
        utf8lex_rule_t *db,
        utf8lex_rule_t *search_for
        )
{
  if (db == NULL
      || search_for == NULL
      || search_for->name == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_rule_t *found_by_name = NULL;
  utf8lex_error_t error_by_name = UTF8LEX_OK;

  printf("    Find by name '%s':", search_for->name);  fflush(stdout);
  error_by_name = utf8lex_rule_find(db,  // first_rule
                                          search_for->name,  // name
                                          &found_by_name);
  if (error_by_name != UTF8LEX_OK)
  {
    printf(" NOT FOUND!\n");  fflush(stdout);
  }
  else if (found_by_name == NULL)
  {
    printf(" NULL!\n");  fflush(stdout);
    error_by_name = UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (found_by_name != search_for)
  {
    printf(" Found wrong rule '%s'!\n", found_by_name->name);  fflush(stdout);
    error_by_name =  UTF8LEX_ERROR_STATE;
  }
  else
  {
    // UTF8LEX_OK, and found correct rule.
    printf(" OK\n");  fflush(stdout);
  }

  utf8lex_rule_t *found_by_id = NULL;
  utf8lex_error_t error_by_id = UTF8LEX_OK;

  printf("    Find by id %u:", (unsigned int) search_for->id);  fflush(stdout);
  error_by_id = utf8lex_rule_find_by_id(db,  // first_rule
                                              search_for->id,  // id
                                              &found_by_id);
  if (error_by_id != UTF8LEX_OK)
  {
    printf(" NOT FOUND!\n");  fflush(stdout);
  }
  else if (found_by_id == NULL)
  {
    printf(" NULL!\n");  fflush(stdout);
    error_by_id = UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (found_by_id != search_for)
  {
    printf(" Found wrong rule %u!\n", (unsigned int) found_by_id->id);  fflush(stdout);
    error_by_id = UTF8LEX_ERROR_STATE;
  }
  else
  {
    // UTF8LEX_OK, and found correct rule.
    printf(" OK\n");  fflush(stdout);
  }

  if (error_by_name != UTF8LEX_OK)
  {
    return error_by_name;
  }
  else if (error_by_id != UTF8LEX_OK)
  {
    return error_by_id;
  }
  else
  {
    return UTF8LEX_OK;
  }
}

utf8lex_error_t test_utf8lex_rule_find()
{
  utf8lex_rule_t *prev = NULL;
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_cat_definition_t definition_1;
  error = utf8lex_cat_definition_init(&definition_1,  // self
                                      NULL,  // prev
                                      "definition_1",  // name
                                      UTF8LEX_GROUP_NUM,  // cat
                                      1,  // min
                                      -1);  // max
  if (error != UTF8LEX_OK) { return error; }
  utf8lex_rule_t rule_1;
  error = utf8lex_rule_init(&rule_1,  // self
                            prev,  // prev
                            "rule_1",  // name
                            (utf8lex_definition_t *)
                            &definition_1,  // definition
                            "printf(\"rule_1\\n\");\n",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = (utf8lex_rule_t *) &rule_1;

  utf8lex_literal_definition_t definition_2;
  error = utf8lex_literal_definition_init(&definition_2,  // self
                                          NULL,  // prev
                                          "definition_2",  // name
                                          "class");  // str
  if (error != UTF8LEX_OK) { return error; }
  utf8lex_rule_t rule_2;
  error = utf8lex_rule_init(&rule_2,  // self
                            prev,  // prev
                            "rule_2",  // name
                            (utf8lex_definition_t *)
                            &definition_2,  // definition
                            "printf(\"rule_2\\n\");\n",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = (utf8lex_rule_t *) &rule_2;

  utf8lex_regex_definition_t definition_3;
  error = utf8lex_regex_definition_init(&definition_3,  // self
                                        NULL,  // prev
                                        "definition_3",  // name
                                        "^[a-zA-Z_][a-zA-Z0-9_]*$");  // pattern
  if (error != UTF8LEX_OK) { return error; }
  utf8lex_rule_t rule_3;
  error = utf8lex_rule_init(&rule_3,  // self
                            prev,  // prev
                            "rule_3",  // name
                            (utf8lex_definition_t *)
                            &definition_3,  // definition
                            "printf(\"rule_3\\n\");\n",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = (utf8lex_rule_t *) &rule_3;

  utf8lex_cat_definition_t definition_4;
  error = utf8lex_cat_definition_init(&definition_4,  // self
                                      NULL,  // prev
                                      "definition_4",  // name
                                      UTF8LEX_GROUP_LETTER,  // cat
                                      1,  // min
                                      1);  // max
  if (error != UTF8LEX_OK) { return error; }
  utf8lex_rule_t rule_4;
  error = utf8lex_rule_init(&rule_4,  // self
                            prev,  // prev
                            "rule_4",  // name
                            (utf8lex_definition_t *)
                            &definition_4,  // definition
                            "printf(\"rule_4\\n\");\n",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = (utf8lex_rule_t *) &rule_4;

  utf8lex_literal_definition_t definition_5;
  error = utf8lex_literal_definition_init(&definition_5,  // self
                                          NULL,  // prev
                                          "definition_5",  // name
                                          "int");  // str
  if (error != UTF8LEX_OK) { return error; }
  utf8lex_rule_t rule_5;
  error = utf8lex_rule_init(&rule_5,  // self
                            prev,  // prev
                            "rule_5",  // name
                            (utf8lex_definition_t *)
                            &definition_5,  // definition
                            "printf(\"rule_5\\n\");\n",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = (utf8lex_rule_t *) &rule_5;

  utf8lex_regex_definition_t definition_6;
  error = utf8lex_regex_definition_init(&definition_6,  // self
                                        NULL,  // prev
                                        "definition_6",  // name
                                        "^http[s]?://([a-z][-a-z0-9]*\\.)*[a-z][-a-z0-9]*$");  // pattern
  if (error != UTF8LEX_OK) { return error; }
  utf8lex_rule_t rule_6;
  error = utf8lex_rule_init(&rule_6,  // self
                            prev,  // prev
                            "rule_6",  // name
                            (utf8lex_definition_t *)
                            &definition_6,  // definition
                            "printf(\"rule_6\\n\");\n",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = (utf8lex_rule_t *) &rule_6;

  utf8lex_rule_t *db = &rule_1;

  error = test_utf8lex_rule_found(db,  // db
                                  (utf8lex_rule_t *)
                                  &rule_1);  // search_for
  if (error != UTF8LEX_OK) { return error; }
  error = test_utf8lex_rule_found(db,  // db
                                  (utf8lex_rule_t *)
                                  &rule_2);  // search_for
  if (error != UTF8LEX_OK) { return error; }
  error = test_utf8lex_rule_found(db,  // db
                                  (utf8lex_rule_t *)
                                  &rule_3);  // search_for
  if (error != UTF8LEX_OK) { return error; }
  error = test_utf8lex_rule_found(db,  // db
                                  (utf8lex_rule_t *)
                                  &rule_4);  // search_for
  if (error != UTF8LEX_OK) { return error; }
  error = test_utf8lex_rule_found(db,  // db
                                  (utf8lex_rule_t *)
                                  &rule_5);  // search_for
  if (error != UTF8LEX_OK) { return error; }
  error = test_utf8lex_rule_found(db,  // db
                                  (utf8lex_rule_t *)
                                  &rule_6);  // search_for
  if (error != UTF8LEX_OK) { return error; }

  utf8lex_literal_definition_t definition_not_in_db;
  error = utf8lex_literal_definition_init(&definition_not_in_db,  // self
                                          NULL,  // prev
                                          "definition_not_in_db",  // name
                                          "string");  // str
  if (error != UTF8LEX_OK) { return error; }
  utf8lex_rule_t rule_not_in_db;
  error = utf8lex_rule_init(&rule_not_in_db,  // self
                            NULL,  // prev
                            "rule_not_in_db",  // name
                            (utf8lex_definition_t *)
                            &definition_not_in_db,  // definition
                            "printf(\"rule_not_in_db\\n\");\n",  // code
                            (size_t) -1);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }

  error = test_utf8lex_rule_found(db,  // db
                                  (utf8lex_rule_t *)
                                  &rule_not_in_db);  // search_for
  if (error == UTF8LEX_OK)
  {
    return UTF8LEX_ERROR_STATE;
  }
  else if (error != UTF8LEX_ERROR_NOT_FOUND)
  {
    return error;
  }

  // Now clean up the definitions and rules:
  error = utf8lex_cat_definition_clear(&(definition_1.base));
  if (error != UTF8LEX_OK) { return error; }
  error = utf8lex_rule_clear(&rule_1);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_literal_definition_clear(&(definition_2.base));
  if (error != UTF8LEX_OK) { return error; }
  error = utf8lex_rule_clear(&rule_2);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_regex_definition_clear(&(definition_3.base));
  if (error != UTF8LEX_OK) { return error; }
  error = utf8lex_rule_clear(&rule_3);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_cat_definition_clear(&(definition_4.base));
  if (error != UTF8LEX_OK) { return error; }
  error = utf8lex_rule_clear(&rule_4);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_literal_definition_clear(&(definition_5.base));
  if (error != UTF8LEX_OK) { return error; }
  error = utf8lex_rule_clear(&rule_5);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_regex_definition_clear(&(definition_6.base));
  if (error != UTF8LEX_OK) { return error; }
  error = utf8lex_rule_clear(&rule_6);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_literal_definition_clear(&(definition_not_in_db.base));
  if (error != UTF8LEX_OK) { return error; }
  error = utf8lex_rule_clear(&rule_not_in_db);
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}

utf8lex_error_t test_utf8lex_rule()
{
  utf8lex_error_t error = UTF8LEX_OK;

  printf("  Testing utf8lex_error_find():\n");  fflush(stdout);
  error = test_utf8lex_rule_find();
  if (error != UTF8LEX_OK)
  {
    printf("    FAILED\n");  fflush(stdout);
    return error;
  }
  printf("    OK\n");  fflush(stdout);

  return UTF8LEX_OK;
}


int main(
        int argc,
        char *argv[]
        )
{
  printf("Testing utf8lex_rule_t...\n");  fflush(stdout);
  utf8lex_error_t error = test_utf8lex_rule();
  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS testing utf8lex_rule_t.\n");  fflush(stdout);
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
    fprintf(stderr, "FAILED testing utf8lex_rule: error %d %s\n",
            error,
            error_string.bytes);
      fflush(stderr);
  }

  fflush(stdout);
  fflush(stderr);

  return (int) error;
}
