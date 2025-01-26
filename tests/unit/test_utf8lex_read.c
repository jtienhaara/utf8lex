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
#include <inttypes.h>  // For int32_t.
#include <string.h>  // For strcpy()

#include "utf8lex.h"


//
// Creates a utf8lex_state_t (including buffer, str) out of the specified
// bytes.  For example:
//
//     utf8lex_state_t state;
//     utf8lex_buffer_t buffer;
//     utf8lex_string_t str;
//     test_utf8lex_init_state(&state, &buffer, &str, "Hello, world!");
//
static utf8lex_error_t test_utf8lex_init_state(
        utf8lex_state_t *state,
        utf8lex_buffer_t *buffer,
        utf8lex_string_t *str,
        unsigned char *bytes
        )
{
  utf8lex_error_t error = UTF8LEX_OK;

  size_t length_bytes = strlen(bytes);
  error = utf8lex_string_init(str,  // self
                              length_bytes,  // max_length_bytes
                              length_bytes,  // length_bytes
                              bytes);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_buffer_init(buffer,  // self
                              NULL,  // prev
                              str,  // str
                              true);  // is_eof
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_state_init(state,  // self
                             buffer);  // buffer
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


//
// Cleans up the specified utf8lex_state_t so it can be reused.For example:
//
//     utf8lex_state_t state;
//     utf8lex_buffer_t buffer;
//     utf8lex_string_t str;
//     test_utf8lex_init_state(&state, &buffer, &str, "Hello, world!");
//     ...
//     test_utf8lex_clear_state(&state);
//     test_utf8lex_init_state(&state, &buffer, &str, "Another test!");
//
static utf8lex_error_t test_utf8lex_clear_state(
        utf8lex_state_t *state
        )
{
  if (state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_buffer_t *buffer = state->buffer;
  utf8lex_string_t *str = state->buffer->str;

  error = utf8lex_state_clear(state);  // self
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_buffer_clear(buffer);  // self
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_string_clear(str);  // self
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


//
// Sets up a utf8lex_location_t with byte, char, grapheme and line
// length, after and hash, with one compact function call.
//
static utf8lex_error_t test_setup_loc(
        utf8lex_location_t loc_pointer[UTF8LEX_UNIT_MAX], // Mutable
        int byte_length, // # of bytes of a token.
        int byte_after,  // Either -1, or reset the start, if >= 0.
        unsigned long byte_hash,  // The sum of bytes.
        int char_length, // # of chars of a token.
        int char_after,  // Either -1, or reset the start, if >= 0.
        unsigned long char_hash,  // The sum of chars.
        int grapheme_length, // # of graphemes of a token.
        int grapheme_after,  // Either -1, or reset the start, if >= 0.
        unsigned long grapheme_hash,  // The sum of graphemes.
        int line_length, // # of lines of a token.
        int line_after,  // Either -1, or reset the start, if >= 0.
        unsigned long line_hash  // The sum of lines.
        )
{
  loc_pointer[UTF8LEX_UNIT_BYTE].length = byte_length;
  loc_pointer[UTF8LEX_UNIT_BYTE].after = byte_after;
  loc_pointer[UTF8LEX_UNIT_BYTE].hash = byte_hash;

  loc_pointer[UTF8LEX_UNIT_CHAR].length = char_length;
  loc_pointer[UTF8LEX_UNIT_CHAR].after = char_after;
  loc_pointer[UTF8LEX_UNIT_CHAR].hash = char_hash;

  loc_pointer[UTF8LEX_UNIT_GRAPHEME].length = grapheme_length;
  loc_pointer[UTF8LEX_UNIT_GRAPHEME].after = grapheme_after;
  loc_pointer[UTF8LEX_UNIT_GRAPHEME].hash = grapheme_hash;

  loc_pointer[UTF8LEX_UNIT_LINE].length = line_length;
  loc_pointer[UTF8LEX_UNIT_LINE].after = line_after;
  loc_pointer[UTF8LEX_UNIT_LINE].hash = line_hash;

  return UTF8LEX_OK;
}


static char * unit_strings[UTF8LEX_UNIT_MAX] = {
  "byte",
  "char",
  "grapheme",
  "line"
};


static utf8lex_error_t test_utf8lex_read_grapheme_ascii()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_state_t state;
  utf8lex_buffer_t buffer;
  utf8lex_string_t str;
  unsigned char *to_read;

  char *grapheme;
  off_t offset;
  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];
  int32_t codepoint;
  utf8lex_cat_t cat;

  utf8lex_location_t expected_loc[UTF8LEX_UNIT_MAX];

  // ===================================================================
  to_read = "Hello, world\nGoodbye\r\nend";
  offset = (off_t) 0;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    loc[unit].start = 0;
    loc[unit].length = 0;
    loc[unit].after = 0;
    loc[unit].hash = (unsigned long) 0;
  }
  printf("  Reading graphemes from '%s', and checking\n", to_read);
  printf("  (start, length, after, hash) (byte, char, grapheme, line):\n");

  // -------------------------------------------------------------------
  grapheme = "H";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 72,  // byte
                         1, -1, (unsigned long) 72,  // char
                         1, -1, (unsigned long) 72,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "e";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 101,  // byte
                         1, -1, (unsigned long) 101,  // char
                         1, -1, (unsigned long) 101,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "l";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 108,  // byte
                         1, -1, (unsigned long) 108,  // char
                         1, -1, (unsigned long) 108,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "l";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 108,  // byte
                         1, -1, (unsigned long) 108,  // char
                         1, -1, (unsigned long) 108,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "o";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 111,  // byte
                         1, -1, (unsigned long) 111,  // char
                         1, -1, (unsigned long) 111,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = ",";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 44,  // byte
                         1, -1, (unsigned long) 44,  // char
                         1, -1, (unsigned long) 44,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = " ";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 32,  // byte
                         1, -1, (unsigned long) 32,  // char
                         1, -1, (unsigned long) 32,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "w";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 119,  // byte
                         1, -1, (unsigned long) 119,  // char
                         1, -1, (unsigned long) 119,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "o";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 111,  // byte
                         1, -1, (unsigned long) 111,  // char
                         1, -1, (unsigned long) 111,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "r";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 114,  // byte
                         1, -1, (unsigned long) 114,  // char
                         1, -1, (unsigned long) 114,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "l";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 108,  // byte
                         1, -1, (unsigned long) 108,  // char
                         1, -1, (unsigned long) 108,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "d";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 100,  // byte
                         1, -1, (unsigned long) 100,  // char
                         1, -1, (unsigned long) 100,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "\\n";  // \n
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 10,  // byte
                         1, 0, (unsigned long) 10,  // char
                         1, 0, (unsigned long) 10,  // grapheme
                         1, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "G";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 71,  // byte
                         1, -1, (unsigned long) 71,  // char
                         1, -1, (unsigned long) 71,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "o";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 111,  // byte
                         1, -1, (unsigned long) 111,  // char
                         1, -1, (unsigned long) 111,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "o";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 111,  // byte
                         1, -1, (unsigned long) 111,  // char
                         1, -1, (unsigned long) 111,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "d";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 100,  // byte
                         1, -1, (unsigned long) 100,  // char
                         1, -1, (unsigned long) 100,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "b";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 98,  // byte
                         1, -1, (unsigned long) 98,  // char
                         1, -1, (unsigned long) 98,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "y";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 121,  // byte
                         1, -1, (unsigned long) 121,  // char
                         1, -1, (unsigned long) 121,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "e";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 101,  // byte
                         1, -1, (unsigned long) 101,  // char
                         1, -1, (unsigned long) 101,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "\\r\\n";  // \r\n
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         2, -1, (unsigned long) 3338,  // byte
                         2, 0, (unsigned long) 3338,  // char
                         1, 0, (unsigned long) 3338,  // grapheme
                         1, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "e";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 101,  // byte
                         1, -1, (unsigned long) 101,  // char
                         1, -1, (unsigned long) 101,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "n";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 110,  // byte
                         1, -1, (unsigned long) 110,  // char
                         1, -1, (unsigned long) 110,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "d";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         1, -1, (unsigned long) 100,  // byte
                         1, -1, (unsigned long) 100,  // char
                         1, -1, (unsigned long) 100,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------

  if (offset != (off_t) strlen(to_read))
  {
    printf("  ERROR Expected offset %d, but found: %d\n",
           (int) strlen(to_read), (int) offset);
    return UTF8LEX_ERROR_BAD_OFFSET;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t test_utf8lex_read_grapheme_unicode()
{
  utf8lex_error_t error = UTF8LEX_OK;

  utf8lex_state_t state;
  utf8lex_buffer_t buffer;
  utf8lex_string_t str;
  unsigned char *to_read;

  char *grapheme;
  off_t offset;
  utf8lex_location_t loc[UTF8LEX_UNIT_MAX];
  int32_t codepoint;
  utf8lex_cat_t cat;

  utf8lex_location_t expected_loc[UTF8LEX_UNIT_MAX];

  // ===================================================================
  to_read = "¾¢÷Æ";  // 0xC2BE 0xC2A2 0xC3B7 0xC386  (49854 49826 50103 50054)
  offset = (off_t) 0;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    loc[unit].start = 0;
    loc[unit].length = 0;
    loc[unit].after = 0;
    loc[unit].hash = (unsigned long) 0;
  }
  printf("  Reading graphemes from '%s', and checking\n", to_read);
  printf("  (start, length, after, hash) (byte, char, grapheme, line):\n");

  // -------------------------------------------------------------------
  grapheme = "¾";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         2, -1, (unsigned long) 49854,  // byte
                         1, -1, (unsigned long) 49854,  // char
                         1, -1, (unsigned long) 49854,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "¢";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         2, -1, (unsigned long) 49826,  // byte
                         1, -1, (unsigned long) 49826,  // char
                         1, -1, (unsigned long) 49826,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "÷";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         2, -1, (unsigned long) 50103,  // byte
                         1, -1, (unsigned long) 50103,  // char
                         1, -1, (unsigned long) 50103,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "Æ";
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         2, -1, (unsigned long) 50054,  // byte
                         1, -1, (unsigned long) 50054,  // char
                         1, -1, (unsigned long) 50054,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------

  if (offset != (off_t) strlen(to_read))
  {
    printf("  ERROR Expected offset %d, but found: %d\n",
           (int) strlen(to_read), (int) offset);
    return UTF8LEX_ERROR_BAD_OFFSET;
  }


  // ===================================================================
  to_read = "ࢀ𐄈הַֽ";
  // 0xE0A280 (14721664) 3 bytes 1 char 1 grapheme
  // 0xF0908488 (4036002952) 4 bytes 1 char 1 grapheme
  // 0xd794d6b7d6bd (237034257503933) 6 bytes 3 chars 1 grapheme
  offset = (off_t) 0;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    loc[unit].start = 0;
    loc[unit].length = 0;
    loc[unit].after = 0;
    loc[unit].hash = (unsigned long) 0;
  }
  printf("  Reading graphemes from '%s', and checking\n", to_read);
  printf("  (start, length, after, hash) (byte, char, grapheme, line):\n");

  // -------------------------------------------------------------------
  grapheme = "ࢀ";
  // 0xE0A280 (14721664) 3 bytes 1 char 1 grapheme
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         3, -1, (unsigned long) 14721664,  // byte
                         1, -1, (unsigned long) 14721664,  // char
                         1, -1, (unsigned long) 14721664,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  grapheme = "𐄈";  // 0xF0908488 (4036002952) 4 bytes 1 char 1 grapheme
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         4, -1, (unsigned long) 4036002952,  // byte
                         1, -1, (unsigned long) 4036002952,  // char
                         1, -1, (unsigned long) 4036002952,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------
  // !!! printf("!!!%c%c%c%c!!!\n", (char) 0x09, (char) 0x05, (char) 0x03, (char) 0x10); fflush(stdout);
  // !!! error = UTF8LEX_ERROR_BAD_LENGTH;
  // !!! if (error != UTF8LEX_OK) { return error; }
  grapheme = "הַֽ";
  // 0xD794D6B7D6BD (237034257503933) 6 bytes 3 chars 1 grapheme
  // ה + ַ + ֽ = 
  // 0x05D4 + 0x05B7 + 0x05BD =
  // 0x05D405B705BD
  // Don't ask me how we get from that to
  // 0xD794D6B7D6BD
  loc[UTF8LEX_UNIT_BYTE].start += loc[UTF8LEX_UNIT_BYTE].length;
  loc[UTF8LEX_UNIT_CHAR].start += loc[UTF8LEX_UNIT_CHAR].length;
  loc[UTF8LEX_UNIT_GRAPHEME].start += loc[UTF8LEX_UNIT_GRAPHEME].length;
  loc[UTF8LEX_UNIT_LINE].start += loc[UTF8LEX_UNIT_LINE].length;
  expected_loc[UTF8LEX_UNIT_BYTE].start = loc[UTF8LEX_UNIT_BYTE].start;
  expected_loc[UTF8LEX_UNIT_CHAR].start = loc[UTF8LEX_UNIT_CHAR].start;
  expected_loc[UTF8LEX_UNIT_GRAPHEME].start = loc[UTF8LEX_UNIT_GRAPHEME].start;
  expected_loc[UTF8LEX_UNIT_LINE].start = loc[UTF8LEX_UNIT_LINE].start;
  offset = (off_t) loc[UTF8LEX_UNIT_BYTE].start;
  error = test_setup_loc(loc,
                         0, -1, (unsigned long) 0,  // byte
                         0, -1, (unsigned long) 0,  // char
                         0, -1, (unsigned long) 0,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }
  error = test_setup_loc(expected_loc,
                         6, -1, (unsigned long) 237034257503933,  // byte
                         3, -1, (unsigned long) 237034257503933,  // char
                         1, -1, (unsigned long) 237034257503933,  // grapheme
                         0, -1, (unsigned long) 0);  // line
  if (error != UTF8LEX_OK) { return error; }

  printf("    Reading grapheme # %d '%s':\n",
         loc[UTF8LEX_UNIT_GRAPHEME].start, grapheme, to_read);
  fflush(stdout);
  error == test_utf8lex_init_state(&state,  // state
                                   &buffer,  // buffer
                                   &str,  // str
                                   to_read);
  if (error != UTF8LEX_OK) { return error; }

  error = utf8lex_read_grapheme(
                                &state,  // state
                                &offset,  // offset_pointer, mutable
                                loc,  // loc_pointer, mutable
                                &codepoint,  // codepoint_pointer, mutable
                                &cat);  // cat_pointer, mutable
  if (error != UTF8LEX_OK) { return error; }

  // Check that the start, length, after and hash of bytes, chars, graphemes
  // and lines are all correct:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (loc[unit].start != expected_loc[unit].start)
    {
      printf("      ERROR Incorrect %s.start: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].start, loc[unit].start);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_START; }
    }
    if (loc[unit].length != expected_loc[unit].length)
    {
      printf("      ERROR Incorrect %s.length: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].length, loc[unit].length);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_LENGTH; }
    }
    if (loc[unit].after != expected_loc[unit].after)
    {
      printf("      ERROR Incorrect %s.after: expected %d but found: %d\n",
             unit_strings[unit], expected_loc[unit].after, loc[unit].after);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_AFTER; }
    }
    if (loc[unit].hash != expected_loc[unit].hash)
    {
      printf("      ERROR Incorrect %s.hash: expected %lu but found: %lu\n",
             unit_strings[unit], expected_loc[unit].hash, loc[unit].hash);
      fflush(stdout);
      if (error == UTF8LEX_OK) { error = UTF8LEX_ERROR_BAD_HASH; }
    }
  }

  if (error == UTF8LEX_OK) {
    printf("    OK\n");
    fflush(stdout);
  }
  else
  {
    printf("    FAILED\n");
    fflush(stdout);
    return error;
  }

  error = test_utf8lex_clear_state(&state);  // state
  if (error != UTF8LEX_OK) { return error; }

  // -------------------------------------------------------------------

  if (offset != (off_t) strlen(to_read))
  {
    printf("  ERROR Expected offset %d, but found: %d\n",
           (int) strlen(to_read), (int) offset);
    return UTF8LEX_ERROR_BAD_OFFSET;
  }

  return UTF8LEX_OK;
}


int main(
        int argc,
        char *argv[]
        )
{
  printf("Testing utf8lex_read...\n");  fflush(stdout);

  // Test reading in an ASCii string, with \r\n fake single character:
  utf8lex_error_t error = test_utf8lex_read_grapheme_ascii();

  // Test reading in a Unicode string, with multibyte chars and graphemes:
  if (error == UTF8LEX_OK)
  {
    error = test_utf8lex_read_grapheme_unicode();
  } // else if (error != UTF8LEX_OK) then fall through, below.

  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS Testing utf8lex_read.\n");  fflush(stdout);
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
            "ERROR test_utf8lex_read: Failed with error code: %d %s\n",
            (int) error,
            error_string.bytes);

    fflush(stdout);
    fflush(stderr);
    return (int) error;
  }
}
