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
#include <stdbool.h>  // For bool, true, false.

#include <utf8proc.h>

#include "utf8lex.h"


// Reads to the end of a grapheme, sets the first codepoint of the
// grapheme, and the number of bytes read.
// Note that CR, LF (U+000D, U+000A), often equivalent to '\r\n')
// is treated as a special grapheme cluster (however LF, CR is NOT for now),
// since the 2 characters, combined in sequence, usually represent
// one single line separator.
// The state is used only for the string buffer to read from,
// not for its location info.
// The offset, lengths, codepoint and cat are all set upon
// successfully reading one complete grapheme cluster.
// loc[*].after will be -1 if no newlines were encountered, or 0
// or more if the character / grapheme positions were reset to 0 at newline.
// (Bytes and lines will never have their after locations reset, always -1.)
utf8lex_error_t utf8lex_read_grapheme(
        utf8lex_state_t *state,
        off_t *offset_pointer,  // Mutable.
        utf8lex_location_t loc_pointer[UTF8LEX_UNIT_MAX], // Mutable
        int32_t *codepoint_pointer,  // Mutable.
        utf8lex_cat_t *cat_pointer  // Mutable.
        )
{
  if (state == NULL
      || offset_pointer == NULL
      || loc_pointer == NULL
      || codepoint_pointer == NULL
      || cat_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (*offset_pointer < (off_t) 0
           || *offset_pointer >= state->buffer->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_START;
  }

  utf8lex_error_t error = UTF8LEX_ERROR_BAD_ERROR;
  off_t curr_offset = *offset_pointer;
  utf8proc_int32_t first_codepoint = (utf8proc_int32_t) -1;
  utf8lex_cat_t first_cat = UTF8LEX_CAT_NONE;
  utf8proc_int32_t prev_codepoint = (utf8proc_int32_t) -1;
  utf8proc_int32_t utf8proc_state = (utf8proc_int32_t) 0;;
  size_t total_bytes_read = (size_t) 0;
  size_t total_chars_read = (size_t) 0;
  size_t total_lines_read = (size_t) 0;
  off_t after_char = (off_t) -1;
  off_t after_grapheme = (off_t) -1;
  unsigned long hash = (unsigned long) 0;
  for (int u8c = 0; ; u8c ++)
  {
    unsigned char *str_pointer = (unsigned char *)
      (state->buffer->str->bytes + curr_offset);
    size_t max_bytes = (size_t)
      (state->buffer->str->length_bytes - curr_offset);

    if (max_bytes <= (size_t) 0)
    {
      if (state->buffer->next != NULL)
      {
        // Continue reading from the next buffer in the chain.
        state->buffer = state->buffer->next;
        curr_offset = (off_t) 0;
        u8c --;
        continue;
      }
      else if (state->buffer->is_eof == true)
      {
        // No more bytes can be read in, we're at EOF.
        if (u8c == 0)
        {
          // We haven't read any bytes so far.  How did we get here...?
          error = UTF8LEX_NO_MATCH;  // Should we also return ..._EOF from here?
          break;
        }
        else
        {
          // Finished reading a grapheme with at least 1 valid codepoint.  Done.
          error = UTF8LEX_OK;
          break;
        }
      }
      else
      {
        // Ask for more bytes to be read in, so that we can find
        // a full grapheme, even if it straddles buffer boundaries.
        error = UTF8LEX_MORE;
      }
      break;
    }

    utf8proc_int32_t utf8proc_codepoint;
    utf8proc_ssize_t utf8proc_num_bytes_read = utf8proc_iterate(
        (utf8proc_uint8_t *) str_pointer,
        (utf8proc_ssize_t) max_bytes,
        &utf8proc_codepoint);
    int num_lines_read = 0;

    if (utf8proc_num_bytes_read == UTF8PROC_ERROR_INVALIDUTF8
        && (state->buffer->str->length_bytes - (size_t) curr_offset)
           < UTF8LEX_MAX_BYTES_PER_CHAR)
    {
      if (state->buffer->is_eof == true)
      {
        // No more bytes can be read in, we're at EOF.
        // Bad UTF-8 character at the end of the buffer.
        error = UTF8LEX_ERROR_BAD_UTF8;
      }
      else
      {
        // Need to read more bytes for the full grapheme.
        error = UTF8LEX_MORE;
      }
      break;
    }
    else if (utf8proc_num_bytes_read <= (utf8proc_ssize_t) 0)
    {
      // Possible errors:
      //     - UTF8PROC_ERROR_NOMEM
      //       Memory could not be allocated.
      //     - UTF8PROC_ERROR_OVERFLOW
      //       The given string is too long to be processed.
      //     - UTF8PROC_ERROR_INVALIDUTF8
      //       The given string is not a legal UTF-8 string.
      //     - UTF8PROC_ERROR_NOTASSIGNED
      //       The UTF8PROC_REJECTNA flag was set and an unassigned codepoint
      //       was found.
      //     - UTF8PROC_ERROR_INVALIDOPTS
      //       Invalid options have been used.
      if (u8c == 0)
      {
        error = UTF8LEX_ERROR_BAD_UTF8;
        break;
      }
      else
      {
        // Finished reading at least 1 codepoint.  Done.
        // We'll return to this bad character the next time we lex.
        error = UTF8LEX_OK;
        break;
      }
    }

    if (u8c == 0)  // First UTF-8 character we've read in the grapheme.
    {
      first_codepoint = utf8proc_codepoint;
    }
    else if (prev_codepoint == 0x000D  // Special case: CR, LF
             && utf8proc_codepoint == 0x000A)
    {
      // Not a grapheme break; these 2 characters in sequence
      // represent a special grapheme, 1 separator.
      num_lines_read = -1;
    }
    else  // Not the first UTF-8 character we've read in the grapheme.
    {
      // Check for grapheme break.  Keep reading until we find one.
      utf8proc_bool is_grapheme_break = utf8proc_grapheme_break_stateful(
          prev_codepoint,
          utf8proc_codepoint,
          &utf8proc_state);
      if (is_grapheme_break == (utf8proc_bool) true)
      {
        error = UTF8LEX_OK;
        break;
      }
    }

    // Check the category of character, so that we can determine
    // whether to increment # lines:
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    error = utf8lex_cat_codepoint((int32_t) utf8proc_codepoint,
                                  &cat);
    if (error != UTF8LEX_OK)
    {
      // Maybe a utf8lex bug?
      return error;
    }
    if (u8c == 0)
    {
      first_cat = cat;
    }
    if ((cat & UTF8LEX_CAT_SEP_LINE)
        || (cat & UTF8LEX_CAT_SEP_PARAGRAPH)
        || (cat & UTF8LEX_EXT_SEP_LINE))
    {
      num_lines_read ++;
      after_char = (off_t) 0;
      after_grapheme = (off_t) 0;
    }
    else if (after_char >= (off_t) 0)
    {
      after_char ++;
    }

    curr_offset += (off_t) utf8proc_num_bytes_read;
    total_bytes_read += (size_t) utf8proc_num_bytes_read;
    total_chars_read += (size_t) 1;
    total_lines_read += (size_t) num_lines_read;

    // Hash bytes and chars read in:
    for (off_t byte_offset = (off_t) 0;
         byte_offset < (off_t) utf8proc_num_bytes_read;
         byte_offset ++)
    {
      hash <<= 8;
      hash |= (unsigned long) str_pointer[byte_offset];
    }

    // Keep reading more bytes until we find a grapheme boundary
    // (or run out of bytes).
    prev_codepoint = utf8proc_codepoint;
  }

  if (error != UTF8LEX_OK)
  {
    // Could be bad UTF-8, or need more bytes, etc.
    return error;
  }

  // Success.
  *offset_pointer += (off_t) total_bytes_read;
  // Do not change: loc_pointer[UTF8LEX_UNIT_BYTE].start
  loc_pointer[UTF8LEX_UNIT_BYTE].length = (int) total_bytes_read;
  loc_pointer[UTF8LEX_UNIT_BYTE].after = (int) -1;  // Never reset.
  loc_pointer[UTF8LEX_UNIT_BYTE].hash = (unsigned long) hash;
  // Do not change: loc_pointer[UTF8LEX_UNIT_CHAR].start
  loc_pointer[UTF8LEX_UNIT_CHAR].length = (int) total_chars_read;
  loc_pointer[UTF8LEX_UNIT_CHAR].after = (int) after_char;
  loc_pointer[UTF8LEX_UNIT_CHAR].hash = (unsigned long) hash;
  // Do not change: loc_pointer[UTF8LEX_UNIT_GRAPHEME].start
  loc_pointer[UTF8LEX_UNIT_GRAPHEME].length = (int) 1;
  loc_pointer[UTF8LEX_UNIT_GRAPHEME].after = (int) after_grapheme;  // -1 or 0
  loc_pointer[UTF8LEX_UNIT_GRAPHEME].hash = (unsigned long) hash;
  // Do not change: loc_pointer[UTF8LEX_UNIT_LINE].start
  loc_pointer[UTF8LEX_UNIT_LINE].length = (int) total_lines_read;
  loc_pointer[UTF8LEX_UNIT_LINE].after = (int) -1;  // Never reset.
  loc_pointer[UTF8LEX_UNIT_LINE].hash = (unsigned long) 0;  // Don't hash lines.
  *codepoint_pointer = first_codepoint;
  // We only set the category/ies according to the first codepoint
  // of the grapheme cluster.  The remainder of the characters
  // in the grapheme cluster will be diacritics and so on,
  // so we ignore them for categorizationg purposes.
  *cat_pointer = first_cat;

  return UTF8LEX_OK;
}
