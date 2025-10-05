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
#include <unistd.h>  // For read()

#include "utf8lex.h"

static utf8lex_error_t test_utf8lex(
        char *file_to_lex_path,
        utf8lex_state_t *state
        )
{
  if (file_to_lex_path == NULL
      || state == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;
  printf("  test_utf8lex: begin test...\n");

  utf8lex_definition_t *prev_definition = NULL;
  utf8lex_rule_t *prev_rule = NULL;

  //
  // Note: I'm not sure if pcre2 has bugs, or what, but the ID
  // regular expression matches the first byte (and only the first byte)
  // of the 3-byte UTF number '⅒' (1/10, 0xe2 0x85 0x92), leaving
  // two useless bytes (0x85 0x92) by themselves to be unmatched
  // by all rules.  So until I can figure out why pcre2 is behaving
  // this way, and make it stop:
  //
  //     ALL lexical cat definitions MUST appear before ANY regular expression
  //     definitions.
  //
  printf("    test_utf8lex: Create number cat definition\n");
  utf8lex_cat_definition_t number_definition;
  error = utf8lex_cat_definition_init(&number_definition,
                                      prev_definition,  // prev
                                      "NUM",  // name
                                      UTF8LEX_GROUP_NUM,  // cat
                                      1,  // min
                                      -1);  // max
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &number_definition;
  printf("    test_utf8lex: Create rule NUMBER\n");
  utf8lex_rule_t NUMBER;
  error = utf8lex_rule_init(
      &NUMBER, prev_rule, "NUMBER",
      (utf8lex_definition_t *) &number_definition,  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_rule = &NUMBER;

  printf("    test_utf8lex: Create id regex definition\n");
  utf8lex_regex_definition_t id_definition;
  error = utf8lex_regex_definition_init(&id_definition,
                                        prev_definition,  // prev
                                        "ID",  // name
                                        "[_\\p{L}][_\\p{L}\\p{N}]*");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &id_definition;
  printf("    test_utf8lex: Create rule ID\n");
  utf8lex_rule_t ID;
  error = utf8lex_rule_init(
      &ID, prev_rule, "ID",
      (utf8lex_definition_t *) &id_definition,  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_rule = &ID;

  printf("    test_utf8lex: Create equals3 literal definition\n");
  utf8lex_literal_definition_t equals3_definition;
  error = utf8lex_literal_definition_init(&equals3_definition,
                                          prev_definition,  // prev
                                          "EQUALS3",  // name
                                          "===");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &equals3_definition;
  printf("    test_utf8lex: Create rule EQUALS3\n");
  utf8lex_rule_t EQUALS3;
  error = utf8lex_rule_init(
      &EQUALS3, prev_rule, "EQUALS3",
      (utf8lex_definition_t *) &equals3_definition,  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_rule = &EQUALS3;

  printf("    test_utf8lex: Create equals literal definition\n");
  utf8lex_literal_definition_t equals_definition;
  error = utf8lex_literal_definition_init(&equals_definition,
                                          prev_definition,  // prev
                                          "EQUALS",  // name
                                          "=");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &equals_definition;
  printf("    test_utf8lex: Create rule EQUALS\n");
  utf8lex_rule_t EQUALS;
  error = utf8lex_rule_init(
      &EQUALS, prev_rule, "EQUALS",
      (utf8lex_definition_t *) &equals_definition,  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_rule = &EQUALS;

  printf("    test_utf8lex: Create plus literal definition\n");
  utf8lex_literal_definition_t plus_definition;
  error = utf8lex_literal_definition_init(&plus_definition,
                                          prev_definition,  // prev
                                          "PLUS",  // name
                                          "+");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &plus_definition;
  printf("    test_utf8lex: Create rule PLUS\n");
  utf8lex_rule_t PLUS;
  error = utf8lex_rule_init(
      &PLUS, prev_rule, "PLUS",
      (utf8lex_definition_t *) &plus_definition,  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_rule = &PLUS;

  printf("    test_utf8lex: Create minus literal definition\n");
  utf8lex_literal_definition_t minus_definition;
  error = utf8lex_literal_definition_init(&minus_definition,
                                          prev_definition,  // prev
                                          "MINUS",  // name
                                          "-");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &minus_definition;
  printf("    test_utf8lex: Create rule MINUS\n");
  utf8lex_rule_t MINUS;
  error = utf8lex_rule_init(
      &MINUS, prev_rule, "MINUS",
      (utf8lex_definition_t *) &minus_definition,  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_rule = &MINUS;

  printf("    test_utf8lex: Create space regex definition\n");
  utf8lex_regex_definition_t space_definition;
  error = utf8lex_regex_definition_init(&space_definition,
                                        prev_definition,  // prev
                                        "SPACE",  // name
                                        "[\\s]+");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_definition = (utf8lex_definition_t *) &space_definition;
  printf("    test_utf8lex: Create rule SPACE\n");
  utf8lex_rule_t SPACE;
  error = utf8lex_rule_init(
      &SPACE, prev_rule, "SPACE",
      (utf8lex_definition_t *) &space_definition,  // definition
      "// Empty code",
      (size_t) 13);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  prev_rule = &SPACE;

  // mmap the file to be lexed:
  printf("    test_utf8lex: MMap file %s\n", file_to_lex_path);
  utf8lex_string_t str;
  str.max_length_bytes = -1;
  str.length_bytes = -1;
  str.bytes = NULL;
  utf8lex_buffer_t buffer;
  buffer.next = NULL;
  buffer.prev = NULL;
  buffer.str = &str;
  error = utf8lex_buffer_mmap(&buffer,
                              file_to_lex_path);  // path
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("    test_utf8lex: Initialize state\n");
  error = utf8lex_state_init(
              state,     // self
              NULL,      // settings
              &buffer,   // buffer
              0);        // stack_depth
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  unsigned char token_bytes[256];
  printf("    test_utf8lex: Begin lexing @(byte/char/grapheme/line)[lengths]\n");
  while (error == UTF8LEX_OK)  // Breaks on UTF8LEX_EOF.
  {
    printf("      test_utf8lex: Lex:\n");
    utf8lex_token_t token;
    error = utf8lex_lex(&NUMBER,  // first_rule
                        state,  // state
                        &token);  // token
    if (error == UTF8LEX_EOF)
    {
      // Nothing more to lex.
      break;
    }
    else if (error == UTF8LEX_MORE)
    {
      // !!!   more_bytes = ...;
      // !!!   more_length = strlen(more_bytes);
      // !!!   ...malloc() new utf8lex_string_t "more_string"
      // !!!     and utf8lex_buffer_t "more_buffer"...
      // !!!     utf8lex_buffer_add(state.buffer, more_buffer);
      printf("  test_utf8lex: MORE -> FAILED\n");
      return error;
    }
    else if (error != UTF8LEX_OK)
    {
      printf("  test_utf8lex: FAILED\n");
      return error;
    }

    utf8lex_token_copy_string(&token,
                              token_bytes,
                              (size_t) 256);
    printf("        test_utf8lex: %s (%s) \"%s\" @(%d/%d/%d/%d)[%d/%d/%d/%d] hash(%ul/%ul/%ul/%ul)\n",
           token.rule->name,
           token.rule->definition->definition_type->name,
           token_bytes,
           token.loc[UTF8LEX_UNIT_BYTE].start,
           token.loc[UTF8LEX_UNIT_CHAR].start,
           token.loc[UTF8LEX_UNIT_GRAPHEME].start,
           token.loc[UTF8LEX_UNIT_LINE].start,
           token.loc[UTF8LEX_UNIT_BYTE].length,
           token.loc[UTF8LEX_UNIT_CHAR].length,
           token.loc[UTF8LEX_UNIT_GRAPHEME].length,
           token.loc[UTF8LEX_UNIT_LINE].hash,
           token.loc[UTF8LEX_UNIT_BYTE].hash,
           token.loc[UTF8LEX_UNIT_CHAR].hash,
           token.loc[UTF8LEX_UNIT_GRAPHEME].hash,
           token.loc[UTF8LEX_UNIT_LINE].hash
           );
  }

  printf("    test_utf8lex: End lexing\n");

  printf("    test_utf8lex: Munmap file %s\n", file_to_lex_path);
  error = utf8lex_buffer_munmap(&buffer);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  // Clean up:
  error = utf8lex_cat_definition_clear(&(number_definition.base));
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  error = utf8lex_rule_clear(&NUMBER);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  error = utf8lex_regex_definition_clear(&(id_definition.base));
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  error = utf8lex_rule_clear(&ID);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  error = utf8lex_literal_definition_clear(&(equals3_definition.base));
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  error = utf8lex_rule_clear(&EQUALS3);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  error = utf8lex_literal_definition_clear(&(equals_definition.base));
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  error = utf8lex_rule_clear(&EQUALS);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  error = utf8lex_literal_definition_clear(&(plus_definition.base));
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  error = utf8lex_rule_clear(&PLUS);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  error = utf8lex_literal_definition_clear(&(minus_definition.base));
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  error = utf8lex_rule_clear(&MINUS);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  error = utf8lex_regex_definition_clear(&(space_definition.base));
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  error = utf8lex_rule_clear(&SPACE);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("  test_utf8lex: SUCCESS\n");
  return UTF8LEX_OK;
}


int main(
        int argc,
        char *argv[]
        )
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s (file-to-lex)\n",
            argv[0]);
    return 1;
  }

  unsigned char *file_to_lex_path = (unsigned char *) argv[1];

  utf8lex_state_t state;
  utf8lex_error_t error = test_utf8lex(file_to_lex_path, &state);
  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS lexing\n");
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

    if (state.buffer == NULL)
    {
      // State has not yet been initialized.
      fprintf(stderr,
              "ERROR utf8lex: Failed with error code: %d %s\n",
              (int) error,
              error_string.bytes);
    }
    else
    {
      // State has been initialized, we might have started lexing.
      char state_bytes[256];
      utf8lex_string_t state_string;
      utf8lex_string_init(&state_string,
                          256,  // max_length_bytes
                          0,  // length_bytes
                          &state_bytes[0]);
      utf8lex_state_string(&state_string,
                           &state);

      off_t offset = (off_t) state.loc[UTF8LEX_UNIT_BYTE].start;
      unsigned char *bad_string = &state.buffer->str->bytes[offset];
      fprintf(stderr,
              "ERROR utf8lex %s: Failed at [%d.%d] with error code: %d %s: \"%s\"\n",
              state_string.bytes,
              state.loc[UTF8LEX_UNIT_LINE].start + 1,
              state.loc[UTF8LEX_UNIT_CHAR].start,
              (int) error,
              error_string.bytes,
              bad_string);
      fflush(stderr);
    }

    fflush(stdout);
    fflush(stderr);
    return (int) error;
  }
}
