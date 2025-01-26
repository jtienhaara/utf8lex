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
#include <string.h>  // For strcmp(), strlen()

#include "utf8lex.h"


utf8lex_error_t test_utf8lex_error_string()
{
  char error_bytes[256];
  utf8lex_string_t error_string;
  utf8lex_string_init(&error_string,
                      256,  // max_length_bytes
                      0,  // length_bytes
                      &error_bytes[0]);
  
  for (utf8lex_error_t under_test = UTF8LEX_OK;
       under_test <= UTF8LEX_ERROR_MAX;
       under_test ++)
  {
    utf8lex_error_t error = utf8lex_error_string(&error_string, under_test);
    if (error != UTF8LEX_OK)
    {
      fprintf(stderr,
              "ERROR utf8lex_error_string() failed to turn error %d into a string\n",
              under_test);
      return error;
    }

    printf("  test_utf8lex_error_string: %d -> \"%s\"\n",
           under_test,
           error_string.bytes);
  }

  return UTF8LEX_OK;
}


utf8lex_error_t test_utf8lex_string()
{
  utf8lex_error_t error = UTF8LEX_OK;

  error = test_utf8lex_error_string();
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
  utf8lex_error_t error = test_utf8lex_string();
  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS testing utf8lex_string.\n");
    fflush(stdout);
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
    fprintf(stderr, "FAILED testing utf8lex_string: error %d %s\n",
            error,
            error_string.bytes);
      fflush(stderr);
  }

  fflush(stdout);
  fflush(stderr);

  return (int) error;
}
