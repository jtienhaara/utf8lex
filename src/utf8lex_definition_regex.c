/*
 * utf8lex
 * Copyright © 2023-2024 Johann Tienhaara
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
#include <inttypes.h>  // For uint32_t, int32_t.

// 8-bit character units for pcre2:
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "utf8lex.h"


// ---------------------------------------------------------------------
//                       utf8lex_regex_definition_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_regex_definition_init(
        utf8lex_regex_definition_t *self,
        utf8lex_definition_t *prev,  // Previous definition in DB, or NULL.
        unsigned char *name,  // Usually all uppercase name of definition.
        unsigned char *pattern
        )
{
  if (self == NULL
      || name == NULL
      || pattern == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  // Compile the pattern string into a pcre2code
  // regular expression:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#SEC20
  int pcre2_error;  // We don't actually use this for now.
  PCRE2_SIZE pcre2_offset;  // We don't actually use this for now.
  self->regex = pcre2_compile(
                              pattern,  // the regular expression pattern
                              PCRE2_ZERO_TERMINATED,  // '\0' at end.
                              0,  // default options.
                              &pcre2_error,  // for error code.
                              &pcre2_offset,  // for error offset.
                              NULL);  // no compile context.
  if (pcre2_error < 0)
  {
    PCRE2_UCHAR pcre2_error_string[256];
    pcre2_get_error_message(pcre2_error,
                            pcre2_error_string,
                            (PCRE2_SIZE) 256);
    fprintf(stderr,
            "*** utf8lex: pcre2 regex compile error: %s\n", pcre2_error_string);
    utf8lex_regex_definition_clear((utf8lex_definition_t *) self);

    return UTF8LEX_ERROR_BAD_REGEX;
  }
  else if (self->regex == NULL)
  {
    fprintf(stderr,
            "*** utf8lex: pcre2 compiled NULL regex from pattern: %s\n",
            pattern);
    utf8lex_regex_definition_clear((utf8lex_definition_t *) self);

    return UTF8LEX_ERROR_BAD_REGEX;
  }

  self->base.definition_type = UTF8LEX_DEFINITION_TYPE_REGEX;
  self->base.name = name;
  self->base.next = NULL;
  self->base.prev = prev;
  self->pattern = pattern;

  if (self->base.prev == NULL)
  {
    self->base.id = (uint32_t) 0;
  }
  else
  {
    self->base.id = self->base.prev->id + 1;
    if (self->base.id >= UTF8LEX_DEFINITIONS_DB_LENGTH_MAX)
    {
      return UTF8LEX_ERROR_MAX_LENGTH;
    }
    self->base.prev->next = (utf8lex_definition_t *) self;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_regex_definition_clear(
        utf8lex_definition_t *self  // Must be utf8lex_regex_definition_t *
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->definition_type != UTF8LEX_DEFINITION_TYPE_REGEX)
  {
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  utf8lex_regex_definition_t *regex_definition =
    (utf8lex_regex_definition_t *) self;

  if (regex_definition->regex != NULL)
  {
    pcre2_code_free(regex_definition->regex);
  }

  if (regex_definition->base.next != NULL)
  {
    regex_definition->base.next->prev = regex_definition->base.prev;
  }
  if (regex_definition->base.prev != NULL)
  {
    regex_definition->base.prev->next = regex_definition->base.next;
  }

  regex_definition->base.definition_type = NULL;
  regex_definition->base.id = (uint32_t) 0;
  regex_definition->base.name = NULL;
  regex_definition->pattern = NULL;
  regex_definition->regex = NULL;

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_lex_regex(
        utf8lex_rule_t *rule,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  if (rule == NULL
      || rule->definition == NULL
      || rule->definition->definition_type == NULL
      || rule->definition->name == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (rule->definition->definition_type
           != UTF8LEX_DEFINITION_TYPE_REGEX)
  {
    return UTF8LEX_ERROR_DEFINITION_TYPE;
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  size_t remaining_bytes = state->buffer->str->length_bytes - (size_t) offset;

  // For now we use the traditional (Perl-compatible, "NFA") algorithm:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#SEC28
  //
  // For differences between the traditional and "DFA" algorithms, see:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2matching.html
  pcre2_match_data *match = pcre2_match_data_create(
      1,  // ovecsize.  We only care about the whole match, not sub-groups.
      NULL);  // gcontext.
  if (match == NULL)
  {
    return UTF8LEX_ERROR_STATE;
  }

  utf8lex_regex_definition_t *regex_definition =
    (utf8lex_regex_definition_t *) rule->definition;

  // Match options:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#matchoptions
  //     PCRE2_ANCHORED
  //     The PCRE2_ANCHORED option limits pcre2_match() to matching
  //     at the first matching position. If a pattern was compiled
  //     with PCRE2_ANCHORED, or turned out to be anchored by virtue
  //     of its contents, it cannot be made unachored at matching time.
  //     Note that setting the option at match time disables JIT matching.
  //     PCRE2_NOTBOL
  //     This option specifies that first character of the subject string
  //     is not the beginning of a line, so the circumflex metacharacter
  //     should not match before it. Setting this without having
  //     set PCRE2_MULTILINE at compile time causes circumflex never to match.
  //     This option affects only the behaviour of the circumflex
  //     metacharacter. It does not affect \A.
  //     PCRE2_NOTEOL
  //     This option specifies that the end of the subject string
  //     is not the end of a line, so the dollar metacharacter
  //     should not match it nor (except in multiline mode) a newline
  //     immediately before it. Setting this without having
  //     set PCRE2_MULTILINE at compile time causes dollar never to match.
  //     This option affects only the behaviour of the dollar metacharacter.
  //     It does not affect \Z or \z.
  //     PCRE2_NOTEMPTY
  //     An empty string is not considered to be a valid match if this option
  //     is set. If there are alternatives in the pattern, they are tried.
  //     If all the alternatives match the empty string, the entire match
  //     fails. For example, if the pattern
  //         a?b?
  //     is applied to a string not beginning with "a" or "b", it matches
  //     an empty string at the start of the subject. With PCRE2_NOTEMPTY set,
  //     this match is not valid, so pcre2_match() searches further
  //     into the string for occurrences of "a" or "b".
  int pcre2_error = pcre2_match(
      regex_definition->regex,  // The pcre2_code (compiled regex).
      (PCRE2_SPTR) state->buffer->str->bytes,  // subject
      (PCRE2_SIZE) state->buffer->str->length_bytes,  // length
      (PCRE2_SIZE) offset,  // startoffset
      (uint32_t) PCRE2_ANCHORED,  // options (see above)
      match,  // match_data
      (pcre2_match_context *) NULL);  // NULL means use defaults.

  if (pcre2_error == PCRE2_ERROR_NOMATCH)
  {
    pcre2_match_data_free(match);
    return UTF8LEX_NO_MATCH;
  }
  else if (pcre2_error == PCRE2_ERROR_PARTIAL)
  {
    pcre2_match_data_free(match);
    if (state->buffer->is_eof)
    {
      // No more bytes can be read in, we're at EOF.
      // Bad UTF-8 character at the end of the buffer.
      return UTF8LEX_ERROR_BAD_UTF8;
    }
    else
    {
      // Need to read more bytes for the full grapheme.
      return UTF8LEX_MORE;
    }
  }
  // Negative number indicates error:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#SEC32
  else if (pcre2_error < 0)
  {
    unsigned char pcre2_error_message[256];
    int error_get_error = pcre2_get_error_message(
                              pcre2_error,  // errorcode
                              (PCRE2_UCHAR *) pcre2_error_message,  // buffer
                              (PCRE2_SIZE) 256);  // bufflen
    fprintf(stderr, "*** ut8flex: pcre2 regex match error: %s\n",
            pcre2_error_message);
    fflush(stderr);
    pcre2_match_data_free(match);
    return UTF8LEX_ERROR_REGEX;
  }

  uint32_t num_ovectors = pcre2_get_ovector_count(match);
  if (num_ovectors == (uint32_t) 0)
  {
    pcre2_match_data_free(match);
    return UTF8LEX_ERROR_REGEX;
  }

  // Extract the whole match from the ovectors:
  // pcre2_match_data *match_data);
  PCRE2_SIZE pcre2_match_length_bytes;
  pcre2_substring_length_bynumber(
      match, // match_data
      (uint32_t) 0,  // number
      &pcre2_match_length_bytes);
  size_t match_length_bytes = (size_t) pcre2_match_length_bytes;

  pcre2_match_data_free(match);

  if (match_length_bytes == (size_t) 0)
  {
    return UTF8LEX_NO_MATCH;
  }

  PCRE2_UCHAR match_substring[256];
  PCRE2_SIZE match_substring_length = 256;
  pcre2_substring_copy_bynumber(match,
                                (uint32_t) 0,
                                match_substring,
                                &match_substring_length);

  // Matched.
  //
  // We know how many bytes matched the regular expression.
  // Now we'll now use utf8proc to count how many characters,
  // graphemes, lines, etc. are in the matching region.
  utf8lex_location_t token_loc[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    token_loc[unit].start = state->loc[unit].start;
    token_loc[unit].length = (int) 0;
    token_loc[unit].after = (int) -1;  // No reset after token.
  }

  // Keep reading graphemes until we've reached the end of the regex match:
  for (int ug = 0;
       token_loc[UTF8LEX_UNIT_BYTE].length < match_length_bytes;
       ug ++)
  {
    // Read in one UTF-8 grapheme cluster per loop iteration:
    off_t grapheme_offset = offset;
    utf8lex_location_t grapheme_loc[UTF8LEX_UNIT_MAX];  // Unitialized is fine.
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        state,  // state, including absolute locations.
        &grapheme_offset,  // start byte, relative to start of buffer string.
        grapheme_loc,  // Char, grapheme newline resets, and grapheme lengths
        &codepoint,  // codepoint
        &cat  //cat
        );

    if (error != UTF8LEX_OK)
    {
      // pcre2 matched something that utf8proc either needs MORE bytes for,
      // or rejected outright.
      return error;
    }

    // We found another grapheme inside the regex match.
    // Keep looking for more graphemes inside the regex match.
    offset = grapheme_offset;
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      // Ignore the grapheme's start location.
      // Add to length (bytes, chars, graphemes, lines):
      token_loc[unit].length += grapheme_loc[unit].length;
      // Possible resets to char, grapheme position due to newlines:
      token_loc[unit].after = grapheme_loc[unit].after;
    }
  }

  // Check to make sure pcre2 and utf8proc agree on # bytes.  (They should.)
  if (token_loc[UTF8LEX_UNIT_BYTE].length != match_length_bytes)
  {
    fprintf(stderr,
            "*** utf8lex: pcre2 and utf8proc disagree: pcre2 match_length_bytes = %d vs utf8proc = %d\n",
            (int) match_length_bytes,
            token_loc[UTF8LEX_UNIT_BYTE].length);
  }

  utf8lex_error_t error = utf8lex_token_init(
      token_pointer,  // self
      rule,  // rule
      rule->definition,  // definition
      token_loc,  // Resets for newlines, and lengths in bytes, chars, etc.
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


// A token definition that matches a regular expression,
// such as "^[0-9]+" or "[\\p{N}]+" or "[_\\p{L}][_\\p{L}\\p{N}]*" or "[\\s]+"
// and so on:
static utf8lex_definition_type_t UTF8LEX_DEFINITION_TYPE_REGEX_INTERNAL =
  {
    .name = "REGEX",
    .lex = utf8lex_lex_regex,
    .clear = utf8lex_regex_definition_clear
  };
utf8lex_definition_type_t *UTF8LEX_DEFINITION_TYPE_REGEX =
  &UTF8LEX_DEFINITION_TYPE_REGEX_INTERNAL;
