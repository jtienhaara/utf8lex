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
#include <string.h>  // For strcmp(), strncat

#include "utf8lex.h"


utf8lex_error_t test_utf8lex_printable_str()
{
  unsigned char *input;
  unsigned char *expected;
  unsigned char actual[4096];
  size_t max_bytes = 4096;

  utf8lex_error_t error;

  // 1 Empty string:
  input = "";
  expected = "";
  printf("  1 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 2 No conversion to be done:
  input = "Hello, world!";
  expected = "Hello, world!";
  printf("  2 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 3 Backslash
  input = "Hello, \\ world!";
  expected = "Hello, \\\\ world!";
  printf("  3 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 4 Alert
  input = "Hello, \a world!";
  expected = "Hello, \\a world!";
  printf("  4 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 5 Backspace
  input = "Hello, \b world!";
  expected = "Hello, \\b world!";
  printf("  5 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 6 Form feed
  input = "Hello, \f world!";
  expected = "Hello, \\f world!";
  printf("  6 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 7 Newline
  input = "Hello, \n world!";
  expected = "Hello, \\n world!";
  printf("  7 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 8 Carriage return
  input = "Hello, \r world!";
  expected = "Hello, \\r world!";
  printf("  8 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 9 Tab
  input = "Hello, \t world!";
  expected = "Hello, \\t world!";
  printf("  9 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 10 Vertical tab
  input = "Hello, \v world!";
  expected = "Hello, \\v world!";
  printf("  10 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 11 Quote
  input = "Hello, \" world!";
  expected = "Hello, \\\" world!";
  printf("  11 Testing utf8lex_printable_str(\"%s\"):", expected);
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error != UTF8LEX_OK)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0)
  {
    printf(" FAILED expected '%s' but actual '%s'\n",
            expected,
            actual);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  // 12 Too long for 4096 bytes.
  // 101 input bytes (102 with \0) -> requires 112 bytes including \0
  // too big for 111 bytes.
  // So we expect all but the last \v to be converted,
  // i.e. 109 bytes plus \0 -> 110 bytes.
  max_bytes = 111;
  input = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()-_=+[{]}\\|;:'\",<.>/?\a\b\f\n\r\t\v";
  expected = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()-_=+[{]}\\\\|;:'\\\",<.>/?\\a\\b\\f\\n\\r\\t";
  printf("  12 Testing utf8lex_printable_str(...big input...):");
  fflush(stdout);
  error = utf8lex_printable_str(actual, max_bytes, input,
                                UTF8LEX_PRINTABLE_ALL);
  if (error == UTF8LEX_OK)
  {
    printf(" FAILED expected UTF8LEX_MORE, but returned UTF8LEX_OK\n");
    return UTF8LEX_ERROR_STATE;
  }
  else if (error != UTF8LEX_MORE)
  {
    printf(" FAILED with errror\n");
    return error;
  }
  else if (strcmp(expected, actual) != 0
           || strlen(expected) != strlen(actual))
  {
    unsigned char *diff_str = "";
    for (int c = 0; c < strlen(expected) && c < strlen(actual); c ++)
    {
      if (expected[c] != actual[c])
      {
        diff_str = &(actual[c]);
        break;
      }
    }
    printf(" FAILED expected %d bytes but actual %d bytes\n      expected:\n        %s\n      but actual:\n        %s\n      diff:\n        %s",
           strlen(expected),
           strlen(actual),
           expected,
           actual,
           diff_str);
    fflush(stdout);
    return UTF8LEX_NO_MATCH;
  }
  else
  {
    printf(" OK\n");
    fflush(stdout);
  }

  return UTF8LEX_OK;
}


int main(
        int argc,
        char *argv[]
        )
{
  utf8lex_error_t error = test_utf8lex_printable_str();
  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS testing utf8lex_printable_str.\n");
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
    fprintf(stderr, "FAILED testing utf8lex_printable_str: error %d %s\n",
            error,
            error_string.bytes);
      fflush(stderr);
  }

  fflush(stdout);
  fflush(stderr);

  return (int) error;
}
