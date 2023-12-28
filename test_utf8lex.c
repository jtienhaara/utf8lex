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
#include <unistd.h>  // For read(), STDIN_FILENO

#include "utf8lex.h"

utf8lex_error_t test_utf8lex(
        int fd,
        utf8lex_state_t *state
        )
{
  utf8lex_error_t error = UTF8LEX_OK;
  printf("  test_utf8lex: begin test...\n");

  //
  // Note: I'm not sure if pcre2 has bugs, or what, but the ID
  // regular expression matches the first byte (and only the first byte)
  // of the 3-byte UTF number '⅒' (1/10, 0xe2 0x85 0x92), leaving
  // two useless bytes (0x85 0x92) by themselves to be unmatched
  // by all token types.  So until I can figure out why pcre2 is behaving
  // this way, and make it stop:
  //
  //     ALL lexical cat patterns MUST appear before ANY regular expression
  //     patterns.
  //
  printf("    test_utf8lex: Create number cat pattern\n");
  utf8lex_cat_pattern_t number_pattern;
  error = utf8lex_cat_pattern_init(&number_pattern,
                                   UTF8LEX_GROUP_NUM,  // cat
                                   1,  // min
                                   -1);  // max
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  printf("    test_utf8lex: Create token_type NUMBER\n");
  utf8lex_token_type_t NUMBER;
  error = utf8lex_token_type_init(
      &NUMBER, NULL, "NUMBER",
      (utf8lex_abstract_pattern_t *) &number_pattern,  // pattern
      "// Empty code");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("    test_utf8lex: Create id regex pattern\n");
  utf8lex_regex_pattern_t id_pattern;
  error = utf8lex_regex_pattern_init(&id_pattern,
                                     "[_\\p{L}][_\\p{L}\\p{N}]*");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  printf("    test_utf8lex: Create token_type ID\n");
  utf8lex_token_type_t ID;
  error = utf8lex_token_type_init(
      &ID, &NUMBER, "ID",
      (utf8lex_abstract_pattern_t *) &id_pattern,  // pattern
      "// Empty code");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("    test_utf8lex: Create equals3 literal pattern\n");
  utf8lex_literal_pattern_t equals3_pattern;
  error = utf8lex_literal_pattern_init(&equals3_pattern,
                                       "===");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  printf("    test_utf8lex: Create token_type EQUALS3\n");
  utf8lex_token_type_t EQUALS3;
  error = utf8lex_token_type_init(
      &EQUALS3, &ID, "EQUALS3",
      (utf8lex_abstract_pattern_t *) &equals3_pattern,  // pattern
      "// Empty code");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("    test_utf8lex: Create equals literal pattern\n");
  utf8lex_literal_pattern_t equals_pattern;
  error = utf8lex_literal_pattern_init(&equals_pattern,
                                       "=");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  printf("    test_utf8lex: Create token_type EQUALS\n");
  utf8lex_token_type_t EQUALS;
  error = utf8lex_token_type_init(
      &EQUALS, &EQUALS3, "EQUALS",
      (utf8lex_abstract_pattern_t *) &equals_pattern,  // pattern
      "// Empty code");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("    test_utf8lex: Create plus literal pattern\n");
  utf8lex_literal_pattern_t plus_pattern;
  error = utf8lex_literal_pattern_init(&plus_pattern,
                                       "+");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  printf("    test_utf8lex: Create token_type PLUS\n");
  utf8lex_token_type_t PLUS;
  error = utf8lex_token_type_init(
      &PLUS, &EQUALS, "PLUS",
      (utf8lex_abstract_pattern_t *) &plus_pattern,  // pattern
      "// Empty code");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("    test_utf8lex: Create minus literal pattern\n");
  utf8lex_literal_pattern_t minus_pattern;
  error = utf8lex_literal_pattern_init(&minus_pattern,
                                     "-");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  printf("    test_utf8lex: Create token_type MINUS\n");
  utf8lex_token_type_t MINUS;
  error = utf8lex_token_type_init(
      &MINUS, &PLUS, "MINUS",
      (utf8lex_abstract_pattern_t *) &minus_pattern,  // pattern
      "// Empty code");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("    test_utf8lex: Create space regex pattern\n");
  utf8lex_regex_pattern_t space_pattern;
  error = utf8lex_regex_pattern_init(&space_pattern,
                                     "[\\s]+");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }
  printf("    test_utf8lex: Create token_type SPACE\n");
  utf8lex_token_type_t SPACE;
  error = utf8lex_token_type_init(
      &SPACE, &MINUS, "SPACE",
      (utf8lex_abstract_pattern_t *) &space_pattern,  // pattern
      "// Empty code");
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  // Now read in some bytes:
  printf("    test_utf8lex: Read bytes\n");
  unsigned char utf8_bytes[16384];
  size_t length_bytes = read(fd, utf8_bytes, (size_t) 16384);

  printf("    test_utf8lex: Create string\n");
  utf8lex_string_t str;
  error = utf8lex_string_init(&str,
                              16384,  // max_length_bytes
                              length_bytes,  // length_bytes
                              &utf8_bytes[0]);
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("    test_utf8lex: Create buffer\n");
  utf8lex_buffer_t buffer;
  error = utf8lex_buffer_init(&buffer,
                              NULL,  // prev
                              &str,
                              true);  // is_eof
  if (error != UTF8LEX_OK)
  {
    printf("  test_utf8lex: FAILED\n");
    return error;
  }

  printf("    test_utf8lex: Initialize state\n");
  error = utf8lex_state_init(state, &buffer);
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
    error = utf8lex_lex(&NUMBER,  // first_token_type
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
    printf("        test_utf8lex: %s (%s) \"%s\" @(%d/%d/%d/%d)[%d/%d/%d/%d]\n",
           token.token_type->name,
           token.token_type->pattern->pattern_type->name,
           token_bytes,
           token.loc[UTF8LEX_UNIT_BYTE].start,
           token.loc[UTF8LEX_UNIT_CHAR].start,
           token.loc[UTF8LEX_UNIT_GRAPHEME].start,
           token.loc[UTF8LEX_UNIT_LINE].start,
           token.loc[UTF8LEX_UNIT_BYTE].length,
           token.loc[UTF8LEX_UNIT_CHAR].length,
           token.loc[UTF8LEX_UNIT_GRAPHEME].length,
           token.loc[UTF8LEX_UNIT_LINE].length
           );
  }

  printf("    test_utf8lex: End lexing\n");

  printf("  test_utf8lex: SUCCESS\n");
  return UTF8LEX_OK;
}


int main(
        int argc,
        char *argv[]
        )
{
  utf8lex_state_t state;
  utf8lex_error_t error = test_utf8lex(STDIN_FILENO, &state);
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
              "ERROR utf8lex %s: Failed with error code: %d %s: \"%s\"\n",
              state_string.bytes,
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
