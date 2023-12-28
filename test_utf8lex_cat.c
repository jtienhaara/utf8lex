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
#include <string.h>  // For strcmp(), strlen()

#include "utf8lex.h"

utf8lex_error_t test_utf8lex_format_parse_cat(
        utf8lex_cat_t cat
        )
{
  utf8lex_error_t error = UTF8LEX_OK;
  unsigned char categories[UTF8LEX_CAT_FORMAT_MAX_LENGTH];

  // First format the category as a string:
  error = utf8lex_format_cat(cat, categories);
  if (error == UTF8LEX_OK)
  {
    printf("  utf8lex_format_cat(%d) = \"%s\"\n", cat, categories);
    fflush(stdout);
  }
  else
  {
    fprintf(stderr, "  utf8lex_format_cat(%d)\n", cat);
    fflush(stderr);
    return error;
  }

  // Now parse the string back to a category:
  utf8lex_cat_t parsed_cat = UTF8LEX_CAT_NONE;
  error = utf8lex_parse_cat(&parsed_cat, categories);
  if (error == UTF8LEX_OK)
  {
    printf("  utf8lex_parse_cat(\"%s\") = %d\n", categories, parsed_cat);
    fflush(stdout);
    if (parsed_cat != cat)
    {
      fprintf(stderr,
              "ERROR Expected parse(format()) to return cat %d but returned cat %d instead.\n",
              cat,
              parsed_cat);
      fflush(stderr);
      return UTF8LEX_ERROR_CAT;
    }
  }
  else
  {
    fprintf(stderr, "  utf8lex_format_cat(%d)\n", cat);
    fflush(stderr);
    return error;
  }

  return UTF8LEX_OK;
}


utf8lex_error_t test_utf8lex_cat()
{
  utf8lex_error_t error = UTF8LEX_OK;

  // First the plain categories (no groupings):
  for (utf8lex_cat_t cat = UTF8LEX_CAT_NONE + (utf8lex_cat_t) 1;
       cat < UTF8LEX_CAT_MAX;
       cat *= 2)
  {
    error = test_utf8lex_format_parse_cat(cat);
    if (error != UTF8LEX_OK)
    {
      return error;
    }
  }

  // Next the stanard groups:
  error = test_utf8lex_format_parse_cat(UTF8LEX_GROUP_OTHER);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = test_utf8lex_format_parse_cat(UTF8LEX_GROUP_LETTER);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = test_utf8lex_format_parse_cat(UTF8LEX_GROUP_MARK);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = test_utf8lex_format_parse_cat(UTF8LEX_GROUP_NUM);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = test_utf8lex_format_parse_cat(UTF8LEX_GROUP_PUNCT);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = test_utf8lex_format_parse_cat(UTF8LEX_GROUP_SYM);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = test_utf8lex_format_parse_cat(UTF8LEX_GROUP_WHITESPACE);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Now try some oddball combinations:
  error = test_utf8lex_format_parse_cat(
      UTF8LEX_CAT_LETTER_UPPER
      | UTF8LEX_CAT_LETTER_LOWER
      | UTF8LEX_CAT_LETTER_TITLE
      | UTF8LEX_CAT_NUM_LETTER);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  error = test_utf8lex_format_parse_cat(
      UTF8LEX_CAT_MARK_SPACING_COMBINING
      | UTF8LEX_CAT_SEP_SPACE
      | UTF8LEX_CAT_SEP_LINE
      | UTF8LEX_CAT_SEP_PARAGRAPH
      | UTF8LEX_EXT_SEP_LINE);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Now make sure we format to the smallest possible strings:
  utf8lex_cat_t cat;
  unsigned char *expected_str;
  unsigned char actual_str[UTF8LEX_CAT_FORMAT_MAX_LENGTH];

  cat =
    UTF8LEX_CAT_MARK_SPACING_COMBINING
    | UTF8LEX_CAT_SEP_SPACE
    | UTF8LEX_CAT_SEP_LINE
    | UTF8LEX_CAT_SEP_PARAGRAPH
    | UTF8LEX_EXT_SEP_LINE;
  expected_str = "WHITESPACE | MARK_SC";
  error = utf8lex_format_cat(cat, actual_str);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  else if (strcmp(expected_str, actual_str) != 0)
  {
    size_t expected_length = strlen(expected_str);
    size_t actual_length = strlen(actual_str);
    off_t first_diff_index = (off_t) -1;
    for (off_t c = 0; c < expected_length && c < actual_length; c ++)
    {
      if (expected_str[c] != actual_str[c])
      {
        first_diff_index = c;
        break;
      }
    }
    if (first_diff_index < (off_t) 0)
    {
      first_diff_index = (off_t) 0;
    }
    fprintf(stderr,
            "ERROR Expected utf8lex_format_cat(%d) = \"%s\" [%d] but actual = \"%s\" [%d] (first diff index %d '%c' vs. '%c')\n",
            cat,
            expected_str,
            expected_length,
            actual_str,
            actual_length,
            (int) first_diff_index,
            (char) expected_str[first_diff_index],
            (char) actual_str[first_diff_index]);
    fflush(stderr);
    return UTF8LEX_ERROR_CAT;
  }
  else
  {
    printf("  utf8lex_format_cat(UTF8LEX_CAT_MARK_SPACING_COMBINING | UTF8LEX_CAT_SEP_SPACE | UTF8LEX_CAT_SEP_LINE | UTF8LEX_CAT_SEP_PARAGRAPH | UTF8LEX_EXT_SEP_LINE) = \"%s\"\n",
           actual_str);
    fflush(stdout);
  }

  // Now try some sloppy parsing, with varying whitespace,
  // trailing ors, etc.
  char *str;
  utf8lex_cat_t expected_cat;
  utf8lex_cat_t actual_cat;

  str = "   NUM   |   UPPER     |   LOWER    |   TITLE     |    NUM_LETTER   |  PUNCT    |    NEWLINE      |  DECIMAL        ";
  expected_cat =
    UTF8LEX_GROUP_NUM
    | UTF8LEX_CAT_LETTER_UPPER
    | UTF8LEX_CAT_LETTER_LOWER
    | UTF8LEX_CAT_LETTER_TITLE
    | UTF8LEX_CAT_NUM_LETTER
    | UTF8LEX_GROUP_PUNCT
    | UTF8LEX_EXT_SEP_LINE
    | UTF8LEX_CAT_NUM_DECIMAL;
  error = utf8lex_parse_cat(&actual_cat, str);
  if (error != UTF8LEX_OK)
  {
    return error;
  }
  else if (actual_cat != expected_cat)
  {
    fprintf(stderr,
            "ERROR Expected to parse \"%s\" into cat %d, but returned %d\n",
            str,
            expected_cat,
            actual_cat);
    fflush(stderr);
    return UTF8LEX_ERROR_CAT;
  }
  else
  {
    actual_str[0] = 0;
    utf8lex_format_cat(actual_cat, actual_str);
    printf("  utf8lex_parse_cat(\"%s\") = %d -> \"%s\"\n",
           str,
           actual_cat,
           actual_str);
    fflush(stdout);
  }

  str = "       ";
  printf("  Making sure utf8lex_parse_cat(\"%s\") returns error UTF8LEX_ERROR_CAT...\n",
         str);
  fflush(stdout);
  error = utf8lex_parse_cat(&actual_cat, str);
  fflush(stderr);
  if (error != UTF8LEX_ERROR_CAT)
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
            "ERROR utf8lex_parse_cat(\"%s\") expected %d UTF8LEX_ERROR_CAT but returned %d %s\n",
            str,
            (int) UTF8LEX_ERROR_CAT,
            (int) error,
            error_string.bytes);
    fflush(stderr);
    return UTF8LEX_ERROR_CAT;
  }

  str = "WHITESPACE  |";
  printf("  Making sure utf8lex_parse_cat(\"%s\") returns error UTF8LEX_ERROR_CAT...\n",
         str);
  fflush(stdout);
  error = utf8lex_parse_cat(&actual_cat, str);
  fflush(stderr);
  if (error != UTF8LEX_ERROR_CAT)
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
            "ERROR utf8lex_parse_cat(\"%s\") expected %d UTF8LEX_ERROR_CAT but returned %d %s\n",
            str,
            (int) UTF8LEX_ERROR_CAT,
            (int) error,
            error_string.bytes);
    fflush(stderr);
    return UTF8LEX_ERROR_CAT;
  }

  str = "WHITESPACE  |   | LETTER";
  printf("  Making sure utf8lex_parse_cat(\"%s\") returns error UTF8LEX_ERROR_CAT...\n",
         str);
  fflush(stdout);
  error = utf8lex_parse_cat(&actual_cat, str);
  fflush(stderr);
  if (error != UTF8LEX_ERROR_CAT)
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
            "ERROR utf8lex_parse_cat(\"%s\") expected %d UTF8LEX_ERROR_CAT but returned %d %s\n",
            str,
            (int) UTF8LEX_ERROR_CAT,
            (int) error,
            error_string.bytes);
    fflush(stderr);
    return UTF8LEX_ERROR_CAT;
  }

  return UTF8LEX_OK;
}


int main(
        int argc,
        char *argv[]
        )
{
  utf8lex_error_t error = test_utf8lex_cat();
  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS testing utf8lex_cat.\n");
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
    fprintf(stderr, "FAILED testing utf8lex_cat: %d %s\n",
            error,
            error_string.bytes);
      fflush(stderr);
  }

  fflush(stdout);
  fflush(stderr);

  return (int) error;
}
