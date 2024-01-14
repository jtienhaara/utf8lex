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
#include <fcntl.h>  // For open(), unlink()
#include <inttypes.h>  // For uint32_t
#include <stdbool.h>  // For bool, true, false
#include <string.h>  // For strlen(), strcpy(), strcat, strncpy
#include <unistd.h>  // For write()

#include "utf8lex.h"


typedef struct _STRUCT_utf8lex_definition       utf8lex_definition_t;
typedef struct _STRUCT_utf8lex_generate_lexicon utf8lex_generate_lexicon_t;

#define UTF8LEX_MAX_CATS 64
struct _STRUCT_utf8lex_generate_lexicon
{
  // Newline
  utf8lex_cat_definition_t newline_definition;
  utf8lex_rule_t newline;

  // %%
  utf8lex_literal_definition_t section_divider_definition;
  utf8lex_rule_t section_divider;
  // %{
  utf8lex_literal_definition_t enclosed_open_definition;
  utf8lex_rule_t enclosed_open;
  // %}
  utf8lex_literal_definition_t enclosed_close_definition;
  utf8lex_rule_t enclosed_close;
  // "
  utf8lex_literal_definition_t quote_definition;
  utf8lex_rule_t quote;
  // |
  utf8lex_literal_definition_t or_definition;
  utf8lex_rule_t or;
  // {
  utf8lex_literal_definition_t rule_open_definition;
  utf8lex_rule_t rule_open;
  // }
  utf8lex_literal_definition_t rule_close_definition;
  utf8lex_rule_t rule_close;

  // Literals matching the category names
  int num_cats;
  char cat_names[UTF8LEX_MAX_CATS][UTF8LEX_CAT_FORMAT_MAX_LENGTH];
  utf8lex_literal_definition_t cat_definitions[UTF8LEX_MAX_CATS];
  utf8lex_rule_t cats[UTF8LEX_MAX_CATS];

  // definition ID
  utf8lex_regex_definition_t definition_definition;
  utf8lex_rule_t definition;
  // horizontal whitespace
  utf8lex_regex_definition_t space_definition;
  utf8lex_rule_t space;

  // code
  utf8lex_regex_definition_t code_definition;
  utf8lex_rule_t code;

  // read to end of line
  utf8lex_cat_definition_t to_eol_definition;
  utf8lex_rule_t to_eol;
};


static utf8lex_error_t utf8lex_generate_setup(
        utf8lex_generate_lexicon_t *lex
        )
{
  if (lex == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_definition_t *prev_definition = NULL;
  utf8lex_rule_t *prev = NULL;
  utf8lex_error_t error = UTF8LEX_OK;

  // Newline
  error = utf8lex_cat_definition_init(&(lex->newline_definition),
                                      prev_definition,  // prev
                                      "NEWLINE",  // name
                                      UTF8LEX_CAT_SEP_LINE  // cat
                                      | UTF8LEX_CAT_SEP_PARAGRAPH
                                      | UTF8LEX_EXT_SEP_LINE,
                                      1,  // min
                                      -1);  // max
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->newline_definition);
  error = utf8lex_rule_init(&(lex->newline),
                            prev,
                            "newline",  // name
                            (utf8lex_definition_t *)
                            &(lex->newline_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->newline);

  // %%
  error = utf8lex_literal_definition_init(&(lex->section_divider_definition),
                                          prev_definition,  // prev
                                          "SECTION_DIVIDER",  // name
                                          "%%");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->section_divider_definition);
  error = utf8lex_rule_init(&(lex->section_divider),
                            prev,
                            "section_divider",  // name
                            (utf8lex_definition_t *)  // definition
                            &(lex->section_divider_definition),
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->section_divider);
  // %{
  error = utf8lex_literal_definition_init(&(lex->enclosed_open_definition),
                                          prev_definition,  // prev
                                          "ENCLOSED_OPEN",  // name
                                          "%{");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->enclosed_open_definition);
  error = utf8lex_rule_init(&(lex->enclosed_open),
                            prev,
                            "enclosed_open",  // name
                            (utf8lex_definition_t *)  // definition
                            &(lex->enclosed_open_definition),
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->enclosed_open);
  // %<}
  error = utf8lex_literal_definition_init(&(lex->enclosed_close_definition),
                                          prev_definition,  // prev
                                          "ENCLOSED_CLOSE",  // name
                                          "%}");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->enclosed_close_definition);
  error = utf8lex_rule_init(&(lex->enclosed_close),
                            prev,
                            "enclosed_close",  // name
                            (utf8lex_definition_t *)  // definition
                            &(lex->enclosed_close_definition),
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->enclosed_close);
  // "
  error = utf8lex_literal_definition_init(&(lex->quote_definition),
                                          prev_definition,  // prev
                                          "QUOTE",  // name
                                          "\"");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->quote_definition);
  error = utf8lex_rule_init(&(lex->quote),
                            prev,
                            "quote",  // name
                            (utf8lex_definition_t *)
                            &(lex->quote_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->quote);
  // |
  error = utf8lex_literal_definition_init(&(lex->or_definition),
                                          prev_definition,  // prev
                                          "OR",  // name
                                          "|");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->or_definition);
  error = utf8lex_rule_init(&(lex->or),
                            prev,
                            "or",  // name
                            (utf8lex_definition_t *)
                            &(lex->or_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->or);
  // {
  error = utf8lex_literal_definition_init(&(lex->rule_open_definition),
                                          prev_definition,  // prev
                                          "RULE_OPEN",  // name
                                          "{");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->rule_open_definition);
  error = utf8lex_rule_init(&(lex->rule_open),
                            prev,
                            "rule_open",  // name
                            (utf8lex_definition_t *)
                            &(lex->rule_open_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->rule_open);
  // }
  error = utf8lex_literal_definition_init(&(lex->rule_close_definition),
                                          prev_definition,  // prev
                                          "RULE_CLOSE",  // name
                                          "}");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->rule_close_definition);
  error = utf8lex_rule_init(&(lex->rule_close),
                            prev,
                            "rule_close",  // name
                            (utf8lex_definition_t *)
                            &(lex->rule_close_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->rule_close);

  // Literals matching the category names
  int num_cats = 0;
  for (utf8lex_cat_t cat = UTF8LEX_CAT_NONE + (utf8lex_cat_t) 1;
       cat < UTF8LEX_CAT_MAX;
       cat *= 2)
  {
    strcpy(lex->cat_names[num_cats], ":");
    error = utf8lex_format_cat(cat, &(lex->cat_names[num_cats][1]));
    if (error != UTF8LEX_OK)
    {
      return error;
    }
    strcat(lex->cat_names[num_cats], ":");

    error = utf8lex_literal_definition_init(&(lex->cat_definitions[num_cats]),
                                            prev_definition,  // prev
                                            lex->cat_names[num_cats],  // name
                                            lex->cat_names[num_cats]);  // str
    if (error != UTF8LEX_OK) { return error; }
    prev_definition = (utf8lex_definition_t *) &(lex->cat_definitions[num_cats]);
    error = utf8lex_rule_init(&(lex->cats[num_cats]),
                              prev,
                              lex->cat_names[num_cats],  // name
                              (utf8lex_definition_t *)  // definition
                              &(lex->cat_definitions[num_cats]),
                              "",  // code
                              (size_t) 0);  // code_length_bytes
    if (error != UTF8LEX_OK) { return error; }
    prev = &(lex->cats[num_cats]);

    num_cats ++;
  }
  lex->num_cats = num_cats;

  // definition ID
  error = utf8lex_regex_definition_init(&(lex->definition_definition),
                                        prev_definition,  // prev
                                        "DEFINITION",  // name
                                        "[_\\p{L}][_\\p{L}\\p{N}]*");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->definition_definition);
  error = utf8lex_rule_init(&(lex->definition),
                            prev,
                            "definition",  // name
                            (utf8lex_definition_t *)
                            &(lex->definition_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->definition);

  // horizontal whitespace
  error = utf8lex_regex_definition_init(&(lex->space_definition),
                                        prev_definition,  // prev
                                        "SPACE",  // name
                                        "[\\h]+");
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->space_definition);
  error = utf8lex_rule_init(&(lex->space),
                            prev,
                            "space",  // name
                            (utf8lex_definition_t *)
                            &(lex->space_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }
  prev = &(lex->space);


  // Everything to the end of the line
  error = utf8lex_cat_definition_init(&(lex->to_eol_definition),
                                      prev_definition,  // prev
                                      "TO_EOL",  // name
                                      UTF8LEX_GROUP_NOT_VSPACE,  // cat
                                      1,  // min
                                      -1);  // max
  if (error != UTF8LEX_OK) { return error; }
  prev_definition = (utf8lex_definition_t *) &(lex->to_eol_definition);
  error = utf8lex_rule_init(&(lex->to_eol),
                            NULL,  // Do not link to other rules.
                            "to_eol",  // name
                            (utf8lex_definition_t *)
                            &(lex->to_eol_definition),  // definition
                            "",  // code
                            (size_t) 0);  // code_length_bytes
  if (error != UTF8LEX_OK) { return error; }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_fill_some_of_remaining_buffer(
        unsigned char *some_of_remaining_buffer,
        utf8lex_state_t *state,
        size_t buffer_bytes,
        size_t max_bytes
        )
{
  if (some_of_remaining_buffer == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->loc == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (buffer_bytes <= (size_t) 0
           || max_bytes <= (size_t) 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  off_t start_byte = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;

  size_t num_bytes;
  if (buffer_bytes < max_bytes)
  {
    num_bytes = buffer_bytes;
  }
  else
  {
    num_bytes = max_bytes - 1;
  }
  strncpy(some_of_remaining_buffer,
          &(state->buffer->str->bytes[start_byte]),
          num_bytes);
  some_of_remaining_buffer[num_bytes] = 0;

  bool is_first_eof = true;
  off_t c = (off_t) 0;
  for (c = (off_t) 0; c <= (off_t) (max_bytes - 4); c ++)
  {
    if (some_of_remaining_buffer[c] == 0)
    {
      break;
    }
    else if (some_of_remaining_buffer[c] == '\r'
             || some_of_remaining_buffer[c] == '\n')
    {
      if (is_first_eof == false
          || c >= (max_bytes - 6))
      {
        some_of_remaining_buffer[c] = 0;
      }
      else
      {
        // We have enough room to shift everything,
        // and insert an extra character, so we'll put in
        // "\\r" or "\\n".
        for (int d = num_bytes; d > c; d --)
        {
          some_of_remaining_buffer[d] = some_of_remaining_buffer[d - 1];
        }
        if (some_of_remaining_buffer[c] == '\r')
        {
          some_of_remaining_buffer[c + 1] = 'r';
        }
        else if (some_of_remaining_buffer[c] == '\n')
        {
          some_of_remaining_buffer[c + 1] = 'n';
        }
        else
        {
          some_of_remaining_buffer[c + 1] = '?';
        }
        some_of_remaining_buffer[c] = '\\';
        if ((num_bytes + 1) < max_bytes)
        {
          some_of_remaining_buffer[num_bytes + 1] = 0;
        }
        else
        {
          some_of_remaining_buffer[num_bytes] = 0;
        }
        is_first_eof = false;
        c += 2;
      }
    }
  }
  if (c >= (off_t) max_bytes)
  {
    some_of_remaining_buffer[max_bytes - 4] = '.';
    some_of_remaining_buffer[max_bytes - 3] = '.';
    some_of_remaining_buffer[max_bytes - 2] = '.';
    some_of_remaining_buffer[max_bytes - 1] = 0;
  }

  return UTF8LEX_OK;
}

static utf8lex_error_t utf8lex_generate_token_error(
        utf8lex_state_t *state,
        utf8lex_token_t *token,
        unsigned char *message
        )
{
  if (state == NULL
      || state->buffer == NULL
      || state->buffer->loc == NULL
      || state->loc == NULL
      || token == NULL
      || token->rule == NULL
      || token->rule->name == NULL
      || message == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  unsigned char some_of_remaining_buffer[32];
  utf8lex_fill_some_of_remaining_buffer(some_of_remaining_buffer,
                                        state,
                                        (size_t) token->length_bytes,
                                        (size_t) 32);  // max_bytes
  fprintf(stderr,
          "ERROR utf8lex %d.%d %s %s [#%d]: \"%s\"\n",
          state->loc[UTF8LEX_UNIT_LINE].start + 1,
          state->loc[UTF8LEX_UNIT_CHAR].start,
          message,
          token->rule->name,
          token->rule->id,
          some_of_remaining_buffer);

  return UTF8LEX_ERROR_TOKEN;
}

static utf8lex_error_t utf8lex_generate_read_to_eol(
        utf8lex_generate_lexicon_t *lex,
        utf8lex_state_t *state,
        utf8lex_token_t *line_token_pointer,
        utf8lex_token_t *newline_token_pointer
        )
{
  bool is_empty_to_eol = false;
  utf8lex_error_t error = utf8lex_lex(&(lex->to_eol),
                                      state,
                                      line_token_pointer);
  if (error == UTF8LEX_EOF)
  {
    return error;
  }
  else if (error == UTF8LEX_NO_MATCH)
  {
    is_empty_to_eol = true;
  }
  else if (error != UTF8LEX_OK)
  {
    // Error.
    unsigned char some_of_remaining_buffer[32];
    utf8lex_fill_some_of_remaining_buffer(
        some_of_remaining_buffer,
        state,
        (size_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].length,
        (size_t) 32);
    fprintf(stderr,
            "ERROR utf8lex: Failed to read to EOL %d.%d: \"%s\"\n",
            state->loc[UTF8LEX_UNIT_LINE].start + 1,
            state->loc[UTF8LEX_UNIT_CHAR].start,
            some_of_remaining_buffer);
    return error;
  }

  // Read in the newline.
  error = utf8lex_lex(&(lex->newline),
                      state,
                      newline_token_pointer);
  if (error == UTF8LEX_EOF)
  {
    return error;
  }
  else if (error != UTF8LEX_OK)
  {
    // Error.
    unsigned char some_of_remaining_buffer[32];
    utf8lex_fill_some_of_remaining_buffer(
        some_of_remaining_buffer,
        state,
        (size_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].length,
        (size_t) 32);
    fprintf(stderr,
            "ERROR utf8lex: Failed to read newline %d.%d: \"%s\"\n",
            state->loc[UTF8LEX_UNIT_LINE].start + 1,
            state->loc[UTF8LEX_UNIT_CHAR].start,
            some_of_remaining_buffer);
    return error;
  }

  if (is_empty_to_eol == true)
  {
    // Hack Kludge Janky
    // We can't really generate an empty token, so we fake it here.
    line_token_pointer->rule = &(lex->to_eol);
    line_token_pointer->start_byte = newline_token_pointer->start_byte;
    line_token_pointer->length_bytes = (int) 0;
    line_token_pointer->str = newline_token_pointer->str;
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      line_token_pointer->loc[unit].start =
        newline_token_pointer->loc[unit].start;
      line_token_pointer->loc[unit].length = (int) 0;
      line_token_pointer->loc[unit].after = (int) -1;
    }
  }

  return UTF8LEX_OK;
}

static utf8lex_error_t utf8lex_generate_write_line(
        int fd_out,
        utf8lex_generate_lexicon_t *lex,
        utf8lex_state_t *state
        )
{
  if (lex == NULL
      || state == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  utf8lex_token_t line_token;
  utf8lex_token_t newline_token;
  utf8lex_error_t error = UTF8LEX_OK;
  size_t bytes_written;

  // Read the rest of the current line as a single token:
  error = utf8lex_generate_read_to_eol(lex,
                                       state,
                                       &line_token,
                                       &newline_token);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Write out the rest of the line:
  bytes_written = write(fd_out,
                        &(line_token.str->bytes[line_token.start_byte]),
                        line_token.length_bytes);
  if (bytes_written != line_token.length_bytes)
  {
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  // Write out the newline:
  bytes_written = write(fd_out,
                        &(newline_token.str->bytes[newline_token.start_byte]),
                        newline_token.length_bytes);
  if (bytes_written != newline_token.length_bytes)
  {
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  return UTF8LEX_OK;
}

static utf8lex_error_t utf8lex_generate_parse(
        const utf8lex_target_language_t *target_language,
        utf8lex_state_t *state_pointer,
        utf8lex_buffer_t *lex_file,
        int fd_out
        )
{
  if (target_language == NULL
      || state_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  utf8lex_error_t error;

  utf8lex_generate_lexicon_t lex;
  error = utf8lex_generate_setup(&lex);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  error = utf8lex_state_init(state_pointer, lex_file);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  utf8lex_rule_t *first_rule = &(lex.newline);

  // Database of definitions:
  utf8lex_definition_t definitions[UTF8LEX_DEFINITIONS_DB_LENGTH_MAX];
  int num_definitions = 0;
  utf8lex_definition_t *first_definition = NULL;
  // Currently defined definition's id:
  unsigned char definition_id[256];
  // Definitions for rules:
  utf8lex_cat_t categories = UTF8LEX_CAT_NONE;
  int cat_min;
  int cat_max;
  char regex[1024];
  char literal[1024];
  utf8lex_definition_t *definition_refs[UTF8LEX_DEFINITIONS_DB_LENGTH_MAX];
  int num_definition_refs = 0;

  // This horrid approach will have to do for now.
  // After trying to make the lex grammar context-free, and running
  // out of Christmas break in which to battle that windmill,
  // it's time for a hack.

  // -------------------------------------------------------------------
  // Definitions section:
  // -------------------------------------------------------------------
  bool is_enclosed = false;
  int infinite_loop_protector = 0;
  while (true)
  {
    infinite_loop_protector ++;
    if (infinite_loop_protector > 65536)  // That's a lot of lines of lex code.
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex: Aborting, possible infinite loop %d.%d: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    // Read a token in from the beginning of the line:
    utf8lex_token_t token;
    error = utf8lex_lex(first_rule,
                        state_pointer,
                        &token);
    if (is_enclosed == true
        && error != UTF8LEX_OK
        && error != UTF8LEX_EOF)
    {
      // Not a token, but we're inside the enclosed code section,
      // so just write it out.
      utf8lex_generate_write_line(fd_out,
                                  &lex,
                                  state_pointer);
      error = UTF8LEX_OK;
      continue;
    }
    else if (error != UTF8LEX_OK)
    {
      // Error.
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex_file_parse() failed to parse %d.%d: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return error;
    }

    // Debug:
    // !!!
    unsigned char token_string[token.length_bytes + 1];
    strncpy(token_string,
            &(token.str->bytes[token.start_byte]),
            token.length_bytes);
    token_string[token.length_bytes] = 0;
    printf("!!! YAY definitions lexed rule id %d '%s' = '%s'\n",
           token.rule->id,
           token.rule->name,
           token_string);

    // Acceptable tokens in the definitions section:
    //   (newline) Blank line, ignored.
    //   %{        Start enclosed code section.  (Write out contents to fd_out.)
    //   %}        End enclosed code section.
    //   (space)   Indented code line.
    //   (id)      Definition.
    error = UTF8LEX_ERROR_TOKEN;  // Default to invalid token.
    if (lex.newline.id == token.rule->id)
    {
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_open.id == token.rule->id
             && is_enclosed == false)
    {
      is_enclosed = true;
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_close.id == token.rule->id
             && is_enclosed == true)
    {
      is_enclosed = false;
      error = UTF8LEX_OK;
    }
    else if (is_enclosed == true)
    {
      // Write out the token, and the rest of the line, to fd_out.
      size_t bytes_written = write(fd_out,
                                   &(token.str->bytes[token.start_byte]),
                                   token.length_bytes);
      if (bytes_written != token.length_bytes)
      {
        error = UTF8LEX_ERROR_FILE_WRITE;
      }

      // Now write out the rest of the line to fd_out.
      error = utf8lex_generate_write_line(fd_out,
                                          &lex,
                                          state_pointer);
    }
    else if (lex.space.id == token.rule->id)
    {
      utf8lex_generate_write_line(fd_out,
                                  &lex,
                                  state_pointer);
      error = UTF8LEX_OK;
    }
    else if (lex.definition.id == token.rule->id)
    {
      printf("!!! todo store definition id, use rest of line as definition\n");
      // Read the rest of the current line as a single token:
      utf8lex_token_t line_token;
      utf8lex_token_t newline_token;
      error = utf8lex_generate_read_to_eol(&lex,
                                           state_pointer,
                                           &line_token,
                                           &newline_token);
      if (error != UTF8LEX_OK)
      {
        return error;
      }

      // !!! definition is now stored in line_token.
      // !!! we don't care about newline_token.

      error = UTF8LEX_OK;
    }
    else if (lex.section_divider.id == token.rule->id)
    {
      error = utf8lex_lex(first_rule,
                          state_pointer,
                          &token);
      if (error != UTF8LEX_OK)
      {
        // Error.
        unsigned char some_of_remaining_buffer[32];
        utf8lex_fill_some_of_remaining_buffer(
            some_of_remaining_buffer,
            state_pointer,
            (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
            (size_t) 32);
        fprintf(stderr,
                "ERROR utf8lex_file_parse() failed to parse %d.%d: \"%s\"\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
                some_of_remaining_buffer);
        return error;
      }
      else if (lex.newline.id != token.rule->id)
      {
        return utf8lex_generate_token_error(
            state_pointer,
            &token,
            "Expected newline after %% definitions/rules section divider");
      }

      // Move on to the rules section.
      error = UTF8LEX_OK;
      break;
    }

    if (error != UTF8LEX_OK)
    {
      return utf8lex_generate_token_error(
          state_pointer,
          &token,
          "Unexpected token in definitions section");
    }
  }

  if (error != UTF8LEX_OK)
  {
    return error;
  }


  // -------------------------------------------------------------------
  // Rules section:
  // -------------------------------------------------------------------
  is_enclosed = false;
  infinite_loop_protector = 0;
  while (true)
  {
    infinite_loop_protector ++;
    if (infinite_loop_protector > 65536)  // That's a lot of lines of lex code.
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex: Aborting, possible infinite loop %d.%d: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    // Read a token in from the beginning of the line:
    utf8lex_token_t token;
    error = utf8lex_lex(first_rule,
                        state_pointer,
                        &token);
    if (is_enclosed == true
        && error != UTF8LEX_OK
        && error != UTF8LEX_EOF)
    {
      // Not a token, but we're inside the enclosed code section,
      // so just write it out.
      utf8lex_generate_write_line(fd_out,
                                  &lex,
                                  state_pointer);
      error = UTF8LEX_OK;
      continue;
    }
    else if (error != UTF8LEX_OK)
    {
      // Error.
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex_file_parse() failed to parse %d.%d: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return error;
    }

    // Debug:
    // !!!
    unsigned char token_string[token.length_bytes + 1];
    strncpy(token_string,
            &(token.str->bytes[token.start_byte]),
            token.length_bytes);
    token_string[token.length_bytes] = 0;
    printf("!!! YAY rules got rule id %d '%s' = '%s'\n",
           token.rule->id,
           token.rule->name,
           token_string);

    // Acceptable tokens in the rules section:
    //   (newline)                 Blank line, ignored.
    //   %{                        Start enclosed code section.
    //   %}                        End enclosed code section.
    //   (space)                   Indented code line.
    //   (definition) { ...code... }  Rule.
    error = UTF8LEX_ERROR_TOKEN;  // Default to invalid token.
    if (lex.newline.id == token.rule->id)
    {
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_open.id == token.rule->id
             && is_enclosed == false)
    {
      is_enclosed = true;
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_close.id == token.rule->id
             && is_enclosed == true)
    {
      is_enclosed = false;
      error = UTF8LEX_OK;
    }
    else if (is_enclosed == true)
    {
      // Write out the token, and the rest of the line, to fd_out.
      size_t bytes_written = write(fd_out,
                                   &(token.str->bytes[token.start_byte]),
                                   token.length_bytes);
      if (bytes_written != token.length_bytes)
      {
        error = UTF8LEX_ERROR_FILE_WRITE;
      }

      // Now write out the rest of the line to fd_out.
      error = utf8lex_generate_write_line(fd_out,
                                          &lex,
                                          state_pointer);
    }
    else if (lex.space.id == token.rule->id)
    {
      utf8lex_generate_write_line(fd_out,
                                  &lex,
                                  state_pointer);
      error = UTF8LEX_OK;
    }
    else if (lex.definition.id == token.rule->id)
    {
      printf("!!! todo definition: start with id\n");
      // !!! Read the rest of the current line as a single token:
      utf8lex_token_t line_token;
      utf8lex_token_t newline_token;
      error = utf8lex_generate_read_to_eol(&lex,
                                           state_pointer,
                                           &line_token,
                                           &newline_token);
      if (error != UTF8LEX_OK)
      {
        return error;
      }

      error = UTF8LEX_OK;
    }
    else if (lex.section_divider.id == token.rule->id)
    {
      error = utf8lex_lex(first_rule,
                          state_pointer,
                          &token);
      if (error != UTF8LEX_OK)
      {
        // Error.
        unsigned char some_of_remaining_buffer[32];
        utf8lex_fill_some_of_remaining_buffer(
            some_of_remaining_buffer,
            state_pointer,
            (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
            (size_t) 32);
        fprintf(stderr,
                "ERROR utf8lex_file_parse() failed to parse %d.%d: \"%s\"\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
                some_of_remaining_buffer);
        return error;
      }
      else if (lex.newline.id != token.rule->id)
      {
        return utf8lex_generate_token_error(
            state_pointer,
            &token,
            "Expected newline after %% rules/user code section divider");
      }

      // Move on to the user code section.
      error = UTF8LEX_OK;
      break;
    }
    else
    {
      // OK let's try cats, regex...
      // !!!
    }

    if (error != UTF8LEX_OK)
    {
      return utf8lex_generate_token_error(
          state_pointer,
          &token,
          "Unexpected token in rules section");
    }
  }

  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // -------------------------------------------------------------------
  // User code section:
  // -------------------------------------------------------------------
  infinite_loop_protector = 0;
  while (true)
  {
    infinite_loop_protector ++;
    if (infinite_loop_protector > 65536)  // That's a lot of lines of lex code.
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex: Aborting, possible infinite loop %d.%d: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    // Write out the whole line to fd_out.
    error = utf8lex_generate_write_line(fd_out,
                                        &lex,
                                        state_pointer);
    if (error == UTF8LEX_EOF)
    {
      break;
    }
    else if (error != UTF8LEX_OK)
    {
      // Error.
      unsigned char some_of_remaining_buffer[32];
      utf8lex_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex_file_parse() failed to parse %d.%d: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      return error;
    }
  }

  if (error != UTF8LEX_EOF)
  {
    return error;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_generate(
        const utf8lex_target_language_t *target_language,
        unsigned char *lex_file_path,
        unsigned char *template_dir_path,
        unsigned char *generated_file_path,
        utf8lex_state_t *state_pointer  // Will be initialized.
        )
{
  if (target_language == NULL
      || target_language->extension == NULL
      || lex_file_path == NULL
      || template_dir_path == NULL
      || generated_file_path == NULL
      || state_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;

  // First, read in the lex .l file:
  // mmap the file to be lexed:
  utf8lex_string_t lex_file_str;
  lex_file_str.max_length_bytes = -1;
  lex_file_str.length_bytes = -1;
  lex_file_str.bytes = NULL;
  utf8lex_buffer_t lex_file_buffer;
  lex_file_buffer.next = NULL;
  lex_file_buffer.prev = NULL;
  lex_file_buffer.str = &lex_file_str;
  error = utf8lex_buffer_mmap(&lex_file_buffer,
                              lex_file_path);  // path
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Now, read in the "head" and "tail" template files
  // for the target language.
  size_t template_dir_path_length = strlen(template_dir_path);
  unsigned char *path_sep = "/";  // TODO OS independent
  size_t path_sep_length = strlen(path_sep);
  char *extension = target_language->extension;
  size_t extension_length = strlen(extension);
  size_t template_length =
    template_dir_path_length
    + path_sep_length
    // Your name goes here (e.g. "head", "tail").
    + extension_length;

  char head_path[template_length + strlen("head") + 1];
  strcpy(head_path, template_dir_path);
  strcat(head_path, path_sep);
  strcat(head_path, "head");
  strcat(head_path, extension);
  utf8lex_string_t head_str;
  head_str.max_length_bytes = -1;
  head_str.length_bytes = -1;
  head_str.bytes = NULL;
  utf8lex_buffer_t head_buffer;
  head_buffer.next = NULL;
  head_buffer.prev = NULL;
  head_buffer.str = &head_str;
  error = utf8lex_buffer_mmap(&head_buffer,
                              head_path);  // path
  if (error != UTF8LEX_OK)
  {
    utf8lex_buffer_munmap(&lex_file_buffer);
    return error;
  }

  char tail_path[template_length + strlen("tail") + 1];
  strcpy(tail_path, template_dir_path);
  strcat(tail_path, path_sep);
  strcat(tail_path, "tail");
  strcat(tail_path, extension);
  utf8lex_string_t tail_str;
  tail_str.max_length_bytes = -1;
  tail_str.length_bytes = -1;
  tail_str.bytes = NULL;
  utf8lex_buffer_t tail_buffer;
  tail_buffer.next = NULL;
  tail_buffer.prev = NULL;
  tail_buffer.str = &tail_str;
  error = utf8lex_buffer_mmap(&tail_buffer,
                              tail_path);  // path
  if (error != UTF8LEX_OK)
  {
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    return error;
  }

  // Generate and write out the target language source code file:
  int fd_out = open(generated_file_path,
                    O_CREAT | O_WRONLY | O_TRUNC,
                    S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH);
  if (fd_out < 0)
  {
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    utf8lex_buffer_munmap(&tail_buffer);
    return UTF8LEX_ERROR_FILE_OPEN;
  }

  size_t bytes_written = (size_t) 0;

  bytes_written = write(fd_out,
                        head_buffer.str->bytes,
                        head_buffer.str->length_bytes);
  if (bytes_written != head_buffer.str->length_bytes)
  {
    close(fd_out);
    unlink(generated_file_path);  // Delete the failed output file.
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    utf8lex_buffer_munmap(&tail_buffer);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  error = utf8lex_generate_parse(target_language,
                                 state_pointer,  // Will be initialized.
                                 &lex_file_buffer,
                                 fd_out);
  if (error != UTF8LEX_OK)
  {
    close(fd_out);
    unlink(generated_file_path);  // Delete the failed output file.
    state_pointer->buffer = NULL;
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    utf8lex_buffer_munmap(&tail_buffer);
    return error;
  }

  bytes_written = write(fd_out,
                        tail_buffer.str->bytes,
                        tail_buffer.str->length_bytes);
  if (bytes_written != tail_buffer.str->length_bytes)
  {
    close(fd_out);
    unlink(generated_file_path);  // Delete the failed output file.
    state_pointer->buffer = NULL;
    utf8lex_buffer_munmap(&lex_file_buffer);
    utf8lex_buffer_munmap(&head_buffer);
    utf8lex_buffer_munmap(&tail_buffer);
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  close(fd_out);
  state_pointer->buffer = NULL;
  utf8lex_buffer_munmap(&lex_file_buffer);
  utf8lex_buffer_munmap(&head_buffer);
  utf8lex_buffer_munmap(&tail_buffer);

  return UTF8LEX_OK;
}
