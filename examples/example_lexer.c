/*
 * Uses (whichever file is compiled and linked, e.g. programming_tokens.l)
 * to lex an input file.
 *
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
#include <unistd.h>  // For execl(), fork(), getcwd()
#include <sys/wait.h>  // For waitpid()

#include "utf8lex.h"

extern utf8lex_error_t yylex_start(
        unsigned char *path
        );
extern int yyutf8lex(
        utf8lex_token_t *token_or_null,
        utf8lex_lloc_t *location_or_null
        );
extern utf8lex_error_t yylex_end();

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s (input_file)\n",
            argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "(input_file):\n");
    fprintf(stderr, "    A text file to analyze with the linked lexer.\n");
    fprintf(stderr, "\n");
    fflush(stdout);
    fflush(stderr);
    return 1;
  }

  char *input_file_path = argv[1];

  utf8lex_error_t error;

  unsigned char token_str[4096];
  unsigned char printable_str[4096];

  error = yylex_start(input_file_path);
  if (error != UTF8LEX_OK)
  {
    unsigned char error_name[256];
    error_name[0] = '\0';
    utf8lex_string_t error_string;
    utf8lex_error_t string_error = utf8lex_string(&error_string, 256, error_name);
    string_error = utf8lex_error_string(&error_string, error);
    fprintf(stderr, "ERROR Failed yylex_start(\"%s\"): %d %s\n",
            input_file_path,
            (int) error,
            error_name);
    fflush(stdout);
    fflush(stderr);
    return (int) error;
  }

  utf8lex_token_t token;
  utf8lex_lloc_t location;
  int lex_result = 0;
  while (lex_result >= 0)
  {
    lex_result = yyutf8lex(&token, &location);
    if (lex_result == YYEOF)
    {
      printf("EOF\n");
    }
    else if (lex_result == YYerror)
    {
      fprintf(stderr, "ERROR %d\n",
              lex_result);
    }
    else if (lex_result < 0)
    {
      fprintf(stderr, "UNKNOWN %d\n",
              lex_result);
    }
    else
    {
      error = utf8lex_token_copy_string(&token,  // self
                                        token_str,  // str
                                        (size_t) 4096);  // max_bytes
      if (error != UTF8LEX_OK)
      {
        fflush(stdout);
        fflush(stderr);
        return (int) error;
      }
      error = utf8lex_printable_str(printable_str,  // printable_str
                                    (size_t) 4096,  // max_bytes
                                    token_str,  // str
                                    UTF8LEX_PRINTABLE_ALL);  // flags
      if (error != UTF8LEX_OK)
      {
        fflush(stdout);
        fflush(stderr);
        return (int) error;
      }

      printf("TOKEN: %s \"%s\"\n",
              token.rule->name,
              printable_str);
    }
  }

  error = yylex_end();
  if (error != UTF8LEX_OK)
  {
    fprintf(stderr, "ERROR Failed yylex_end(): %d\n",
            (int) error);
    fflush(stdout);
    fflush(stderr);
    return (int) error;
  }

  fflush(stdout);
  fflush(stderr);

  return 0;
}
