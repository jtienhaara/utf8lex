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

#include <inttypes.h>  // For uint32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strcmp()

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                        utf8lex_definition_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_definition_find(
        utf8lex_definition_t *first_definition,  // Database to search.
        unsigned char *name,  // Name of definition to search for.
        utf8lex_definition_t ** found_pointer  // Gets set when found.
        )
{
  if (first_definition == NULL
      || name == NULL
      || found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_definition_t *definition = first_definition;
  uint32_t infinite_loop = UTF8LEX_DEFINITIONS_DB_LENGTH_MAX;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  for (uint32_t d = 0; d < infinite_loop; d ++)
  {
    if (strcmp(definition->name, name) == 0)
    {
      *found_pointer = definition;
      is_infinite_loop = false;
      break;
    }

    if (definition->next == NULL)
    {
      *found_pointer = NULL;
      is_infinite_loop = false;
      break;
    }

    definition = definition->next;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (*found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NOT_FOUND;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_definition_find_by_id(
        utf8lex_definition_t *first_definition,  // Database to search.
        uint32_t id,  // The id of the definition to search for.
        utf8lex_definition_t ** found_pointer  // Gets set when found.
        )
{
  if (first_definition == NULL
      || found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }


  utf8lex_definition_t *definition = first_definition;
  uint32_t infinite_loop = UTF8LEX_DEFINITIONS_DB_LENGTH_MAX;
  bool is_infinite_loop = true;
  *found_pointer = NULL;
  for (uint32_t d = 0; d < infinite_loop; d ++)
  {
    if (definition->id == id)
    {
      *found_pointer = definition;
      is_infinite_loop = false;
      break;
    }

    if (definition->next == NULL)
    {
      *found_pointer = NULL;
      is_infinite_loop = false;
      break;
    }

    definition = definition->next;
  }

  if (is_infinite_loop == true)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (*found_pointer == NULL)
  {
    return UTF8LEX_ERROR_NOT_FOUND;
  }

  return UTF8LEX_OK;
}


// Makes a UTF-8 string printable by converting certain characters
// such as backslash (\) and newline (\n) into C escape sequences
// such as "\\", "\n", and so on.
// Useful, for example, to recreate the C string for a literal
// (literal_definition->str) or regular expression (regex_definition->pattern).
// Returns UTF8LEX_MORE if max_bytes is not big enough to
// contain the printable version of the specified str (in which case
// as many bytes as possible have been written to the target).
// Conversions done:
// \\, \a, \b, \f, \n, \r, \t, \v, \"
// (from https://pubs.opengroup.org/onlinepubs/7908799/xbd/notation.html
// plus quote)
// Each can be turned on/off with the flags, e.g. to only convert
// backslash (\) and quote ("):
//     utf8lex_printable_str(..., UTF8LEX_PRINTABLE_BACKSLASH
//                                | UTF8LEX_PRINTABLE_QUOTE);
utf8lex_error_t utf8lex_printable_str(
        unsigned char *printable_str,  // Target printable version of string.
        size_t max_bytes,  // Max bytes to write to printable_str including \0.
        unsigned char *str,  // Source string to convert.  Must be 0-terminated.
        utf8lex_printable_flag_t flags  // Which char(s) to convert.  Default ALL.
        )
{
  if (printable_str == NULL
      || str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (max_bytes <= (size_t) 0)
  {
    return UTF8LEX_ERROR_BAD_MAX;
  }

  char backslash[] = "\\\\";
  size_t backslash_bytes = strlen(backslash);
  char alert[] = "\\a";
  size_t alert_bytes = strlen(alert);
  char backspace[] = "\\b";
  size_t backspace_bytes = strlen(backspace);
  char form_feed[] = "\\f";
  size_t form_feed_bytes = strlen(form_feed);
  char newline[] = "\\n";
  size_t newline_bytes = strlen(newline);
  char carriage_return[] = "\\r";
  size_t carriage_return_bytes = strlen(carriage_return);
  char tab[] = "\\t";
  size_t tab_bytes = strlen(tab);
  char vertical_tab[] = "\\v";
  size_t vertical_tab_bytes = strlen(vertical_tab);

  char quote[] = "\\\"";
  size_t quote_bytes = strlen(quote);

  size_t source_bytes = strlen(str);
  printf("!!! source_bytes = %d for %s\n", (int) source_bytes, str);
  off_t target_offset = (off_t) 0;
  size_t target_bytes = (size_t) 0;
  for (int c = 0; c < source_bytes; c ++)
  {
    unsigned char *printable_char_str;
    size_t printable_char_bytes;

    unsigned char *straight_from_source = &(str[c]);
    size_t straight_from_source_bytes = sizeof(char);

    switch (str[c])
    {
    case '\\':
      if (flags & UTF8LEX_PRINTABLE_BACKSLASH)
      {
        printable_char_str = backslash;
        printable_char_bytes = backslash_bytes;
      }
      else
      {
        printable_char_str = straight_from_source;
        printable_char_bytes = straight_from_source_bytes;
      }
      break;
    case '\a':
      if (flags & UTF8LEX_PRINTABLE_ALERT)
      {
        printable_char_str = alert;
        printable_char_bytes = alert_bytes;
      }
      else
      {
        printable_char_str = straight_from_source;
        printable_char_bytes = straight_from_source_bytes;
      }
      break;
    case '\b':
      if (flags & UTF8LEX_PRINTABLE_BACKSPACE)
      {
        printable_char_str = backspace;
        printable_char_bytes = backspace_bytes;
      }
      else
      {
        printable_char_str = straight_from_source;
        printable_char_bytes = straight_from_source_bytes;
      }
      break;
    case '\f':
      if (flags & UTF8LEX_PRINTABLE_FORM_FEED)
      {
        printable_char_str = form_feed;
        printable_char_bytes = form_feed_bytes;
      }
      else
      {
        printable_char_str = straight_from_source;
        printable_char_bytes = straight_from_source_bytes;
      }
      break;
    case '\n':
      if (flags & UTF8LEX_PRINTABLE_NEWLINE)
      {
        printable_char_str = newline;
        printable_char_bytes = newline_bytes;
      }
      else
      {
        printable_char_str = straight_from_source;
        printable_char_bytes = straight_from_source_bytes;
      }
      break;
    case '\r':
      if (flags & UTF8LEX_PRINTABLE_CARRIAGE_RETURN)
      {
        printable_char_str = carriage_return;
        printable_char_bytes = carriage_return_bytes;
      }
      else
      {
        printable_char_str = straight_from_source;
        printable_char_bytes = straight_from_source_bytes;
      }
      break;
    case '\t':
      if (flags & UTF8LEX_PRINTABLE_TAB)
      {
        printable_char_str = tab;
        printable_char_bytes = tab_bytes;
      }
      else
      {
        printable_char_str = straight_from_source;
        printable_char_bytes = straight_from_source_bytes;
      }
      break;
    case '\v':
      if (flags & UTF8LEX_PRINTABLE_VERTICAL_TAB)
      {
        printable_char_str = vertical_tab;
        printable_char_bytes = vertical_tab_bytes;
      }
      else
      {
        printable_char_str = straight_from_source;
        printable_char_bytes = straight_from_source_bytes;
      }
      break;
    case '"':
      if (flags & UTF8LEX_PRINTABLE_QUOTE)
      {
        printable_char_str = quote;
        printable_char_bytes = quote_bytes;
      }
      else
      {
        printable_char_str = straight_from_source;
        printable_char_bytes = straight_from_source_bytes;
      }
      break;
    default:
      // Just the 1 char.
      printable_char_str = straight_from_source;
      printable_char_bytes = straight_from_source_bytes;
    }

    // Add the printable character only if there's enough room
    // to add \0 at the end:
    size_t new_target_bytes =
      target_bytes + printable_char_bytes + sizeof(char);
    if (new_target_bytes >= max_bytes)
    {
      // Can't fit the printable characters plus \0 into the target.
      printable_str[target_offset] = '\0';
      return UTF8LEX_MORE;
    }

    for (off_t pcb = (off_t) 0; pcb < printable_char_bytes; pcb ++)
    {
      printable_str[target_offset] = printable_char_str[pcb];
      target_offset ++;
      target_bytes ++;
    }
    printf("!!! added %d bytes: %s from %c\n", (int) printable_char_bytes, printable_char_str, straight_from_source[0]);
  }

  printable_str[target_offset] = '\0';
  printf("!!! printable_str length should be %d, is %d: %s\n", (int) target_offset, (int) strlen(printable_str), printable_str);
  target_offset ++;

  return UTF8LEX_OK;
}
