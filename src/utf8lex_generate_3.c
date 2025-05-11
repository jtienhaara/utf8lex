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
#include <fcntl.h>  // For open(), unlink()
#include <inttypes.h>  // For uint32_t
#include <stdbool.h>  // For bool, true, false
#include <string.h>  // For strlen(), strcpy(), strcat, strncpy
#include <unistd.h>  // For write()

#include "utf8lex.h"

#include "utf8lex_generate.h"

static utf8lex_error_t utf8lex_generate_write_rules(
        int fd_out,
        utf8lex_db_t *db
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_write_rules()");

  if (db == NULL)
  {
    fprintf(stderr, "ERROR 24 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    fprintf(stderr, "ERROR 25 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_DESCRIPTOR\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  unsigned char line[4096];
  size_t max_bytes = 4096;
  size_t line_bytes;
  size_t bytes_written;

  unsigned char newline[] = "\n";
  size_t newline_bytes = strlen(newline);

  bool is_first;

  utf8lex_error_t error;

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_cat_definition_t YY_CAT_DEFINITIONS[%d];\n",
                        UTF8LEX_NUM_CATEGORIES);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 26 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 27 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_literal_definition_t YY_LITERAL_DEFINITIONS[%d];\n",
                        db->num_literal_definitions);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 28 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 29 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_multi_definition_t YY_MULTI_DEFINITIONS[%d];\n",
                        db->num_multi_definitions);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 30 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 31 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_reference_t YY_REFERENCES[%d];\n",
                        db->num_references);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 32 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 33 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_regex_definition_t YY_REGEX_DEFINITIONS[%d];\n",
                        db->num_regex_definitions);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 34 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 35 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 36 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_rule_t YY_RULES[%d];\n",
                        db->num_rules);
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 37 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 38 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 39 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 40 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static utf8lex_error_t yy_rules_init()\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 41 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 42 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "{\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 43 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 44 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    utf8lex_error_t error;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 45 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 46 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    utf8lex_definition_t *rule_definition;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 47 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 48 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  int cd = 0;  // cat definition #
  int ld = 0;  // literal definition #
  int rd = 0;  // regex definition #
  int md = 0;  // multi definition #
  int ref = 0;  // reference #

  unsigned char printable_str[2 * UTF8LEX_LITERAL_REGEX_MAX_BYTES];

  // Previous definition, reference, rule, etc:
  unsigned char previous[UTF8LEX_NAME_LENGTH_MAX + 16];


  // Definitions:
  // ====================
  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 49 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // Definitions:\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 50 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 51 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // =================================================================\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 52 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 53 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  utf8lex_definition_t *definition = db->definitions_db;
  strcpy(previous, "NULL");
  is_first = true;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < db->num_definitions;
       infinite_loop_protector ++)
  {
    if (definition == NULL)
    {
      fprintf(stderr, "ERROR 54 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    bytes_written = write(fd_out, newline, newline_bytes);
    if (bytes_written != newline_bytes) {
      fprintf(stderr, "ERROR 55 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    if (definition->definition_type == UTF8LEX_DEFINITION_TYPE_CAT)
    {
      // Start cat
      // --------------------
      if (cd >= UTF8LEX_NUM_CATEGORIES)
      {
        fprintf(stderr, "ERROR 56 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_STATE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    // Definition # %d: %s (cat)\n",
                            infinite_loop_protector,
                            db->cat_definitions[cd].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 57 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 58 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    error = utf8lex_cat_definition_init(\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 59 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 60 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                &(YY_CAT_DEFINITIONS[%d]),  // self\n",
                            cd);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 6 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 62 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_definition_t *) %s,  // prev\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 63 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 64 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\",  // name\n",
                            db->cat_definitions[cd].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 65 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }

      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 66 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      // TODO create a cat enum to full cat enum string function
      // TDOO e.g. UTF8LEX_CAT_LETTER_LOWER -> "UTF8LEX_CAT_LETTER_LOWER"
      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_cat_t) %d,  // cat\n",
                            db->cat_definitions[cd].cat);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 67 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 68 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                %d,  // min\n",
                            db->cat_definitions[cd].min);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 69 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 70 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                %d);  // max\n",
                            db->cat_definitions[cd].max);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 71 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 72 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    if (error != UTF8LEX_OK) { return error; }\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 73 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 74 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                            "&(YY_CAT_DEFINITIONS[%d])",
                            cd);
      if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
        fprintf(stderr, "ERROR 75 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }

      cd ++;
      // --------------------
      // End cat
    }
    else if (definition->definition_type == UTF8LEX_DEFINITION_TYPE_LITERAL)
    {
      // Start literal
      // --------------------
      if (ld >= db->num_literal_definitions)
      {
        fprintf(stderr, "ERROR 76 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_STATE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    // Definition # %d: %s (literal)\n",
                            infinite_loop_protector,
                            db->literal_definitions[ld].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 77 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 78 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    error = utf8lex_literal_definition_init(\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 79 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 80 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                &(YY_LITERAL_DEFINITIONS[%d]),  // self\n",
                            ld);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 81 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 82 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_definition_t *) %s,  // prev\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 83 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 84 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\",  // name\n",
                            db->literal_definitions[ld].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 85 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 86 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      error = utf8lex_printable_str(printable_str,
                                    2 * UTF8LEX_LITERAL_REGEX_MAX_BYTES,
                                    db->literal_definitions[ld].str,
                                    UTF8LEX_PRINTABLE_ALL);
      if (error != UTF8LEX_OK)
      {
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return error;
      }
      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\");  // str\n",
                            printable_str);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 87 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 88 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    if (error != UTF8LEX_OK) { return error; }\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 89 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 90 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                            "&(YY_LITERAL_DEFINITIONS[%d])",
                            ld);
      if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
        fprintf(stderr, "ERROR 91 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }

      ld ++;
      // --------------------
      // End literal
    }
    else if (definition->definition_type == UTF8LEX_DEFINITION_TYPE_MULTI)
    {
      // Start multi
      // --------------------
      if (md >= db->num_multi_definitions)
      {
        fprintf(stderr, "ERROR 92 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_STATE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    // Definition # %d: %s (multi)\n",
                            infinite_loop_protector,
                            db->multi_definitions[md].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 93 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 94 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    error = utf8lex_multi_definition_init(\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 95 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 96 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                &(YY_MULTI_DEFINITIONS[%d]),  // self\n",
                            md);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 97 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 98 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_definition_t *) %s,  // prev\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 99 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 100 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\",  // name\n",
                            db->multi_definitions[md].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 101 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 102 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      // TODO allow nested multi definitions e.g. A B (X | D) etc.
      if (db->multi_definitions[md].parent != NULL)
      {
        fprintf(stderr, "ERROR 103 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_NOT_IMPLEMENTED\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_NOT_IMPLEMENTED;
      }
      line_bytes = snprintf(line, max_bytes,
                            "                NULL,  // parent\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 104 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 105 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      if (db->multi_definitions[md].multi_type == UTF8LEX_MULTI_TYPE_SEQUENCE)
      {
        line_bytes = snprintf(line, max_bytes,
                              "                UTF8LEX_MULTI_TYPE_SEQUENCE);  // multi_type\n");
      }
      else if (db->multi_definitions[md].multi_type == UTF8LEX_MULTI_TYPE_OR)
      {
        line_bytes = snprintf(line, max_bytes,
                              "                UTF8LEX_MULTI_TYPE_OR);  // multi_type\n");
      }
      else
      {
        fprintf(stderr, "ERROR 106 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_MULTI_TYPE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_MULTI_TYPE;
      }
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 107 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 108 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    if (error != UTF8LEX_OK) { return error; }\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 109 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 110 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      // Now the references:
      bytes_written = write(fd_out, newline, newline_bytes);
      if (bytes_written != newline_bytes) {
        fprintf(stderr, "ERROR 111 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      strcpy(previous, "NULL");
      utf8lex_reference_t *reference = db->multi_definitions[md].references;
      for (int ref_infinite = 0;
           ref_infinite < UTF8LEX_REFERENCES_LENGTH_MAX;
           ref_infinite ++)
      {
        if (reference == NULL)
        {
          break;
        }
        else if (md >= db->num_references)
        {
          fprintf(stderr, "ERROR 112 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_STATE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "    error = utf8lex_reference_init(\n");
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 113 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 114 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                &(YY_REFERENCES[%d]),  // self\n",
                              ref);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 115 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 116 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                %s,  // prev\n",
                              previous);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 117 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 118 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                \"%s\",  // name\n",
                              reference->definition_name);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 119 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 120 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                %d,  // min\n",
                              reference->min);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 121 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 122 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                %d,  // max\n",
                              reference->max);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 123 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 124 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "                &(YY_MULTI_DEFINITIONS[%d]));  // parent\n",
                              md);
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 125 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 126 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(line, max_bytes,
                              "    if (error != UTF8LEX_OK) { return error; }\n");
        if (line_bytes >= max_bytes) {
          fprintf(stderr, "ERROR 127 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_BAD_LENGTH;
        }
        bytes_written = write(fd_out, line, line_bytes);
        if (bytes_written != line_bytes) {
          fprintf(stderr, "ERROR 128 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_FILE_WRITE;
        }

        line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                              "&(YY_REFERENCES[%d])",
                              ref);
        if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
          fprintf(stderr, "ERROR 129 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
          UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
          return UTF8LEX_ERROR_BAD_LENGTH;
        }

        reference = reference->next;
        ref ++;
      }

      if (reference != NULL)
      {
        fprintf(stderr, "ERROR 130 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_INFINITE_LOOP\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_INFINITE_LOOP;
      }

      line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                            "&(YY_MULTI_DEFINITIONS[%d])",
                            md);
      if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
        fprintf(stderr, "ERROR 131 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }

      md ++;
      // --------------------
      // End multi
    }
    else if (definition->definition_type == UTF8LEX_DEFINITION_TYPE_REGEX)
    {
      // Start regex
      // --------------------
      if (rd >= db->num_regex_definitions)
      {
        fprintf(stderr, "ERROR 132 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_STATE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    // Definition # %d: %s (regex)\n",
                            infinite_loop_protector,
                            db->regex_definitions[rd].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 133 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 134 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "    error = utf8lex_regex_definition_init(\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 135 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 136 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                &(YY_REGEX_DEFINITIONS[%d]),  // self\n",
                            rd);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 137 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 138 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                (utf8lex_definition_t *) %s,  // prev\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 139 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 140 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\",  // name\n",
                            db->regex_definitions[rd].base.name);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 141 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 142 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      error = utf8lex_printable_str(printable_str,
                                    2 * UTF8LEX_LITERAL_REGEX_MAX_BYTES,
                                    db->regex_definitions[rd].pattern,
                                    UTF8LEX_PRINTABLE_ALL);
      if (error != UTF8LEX_OK)
      {
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return error;
      }
      line_bytes = snprintf(line, max_bytes,
                            "                \"%s\");  // pattern\n",
                            printable_str);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 143 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 144 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }


      line_bytes = snprintf(line, max_bytes,
                            "    if (error != UTF8LEX_OK) { return error; }\n");
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 145 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 146 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                            "&(YY_REGEX_DEFINITIONS[%d])",
                            rd);
      if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
        fprintf(stderr, "ERROR 147 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }

      rd ++;
      // --------------------
      // End regex
    }
    else
    {
      fprintf(stderr, "ERROR 148 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_DEFINITION_TYPE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_DEFINITION_TYPE;
    }

    if (is_first == true)
    {
      line_bytes = snprintf(line, max_bytes,
                            "    YY_FIRST_DEFINITION = (utf8lex_definition_t *) %s;\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 149 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 150 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      is_first = false;
    }

    definition = definition->next;
  }

  if (definition != NULL)
  {
    // The definitions db has a loop in it somewhere, somehow.
    fprintf(stderr, "ERROR 151 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_INFINITE_LOOP\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (cd != UTF8LEX_NUM_CATEGORIES
           || ld != db->num_literal_definitions
           || md != db->num_multi_definitions
           || rd != db->num_regex_definitions)
  {
    fprintf(stderr, "ERROR 152 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_STATE;
  }


  // Resolve multi-definitions:
  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 153 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // Resolve multi-definitions:\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 154 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 155 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  for (int md = 0; md < db->num_multi_definitions; md ++)
  {
    line_bytes = snprintf(line, max_bytes,
                          "    // # %d %s:\n",
                          md,
                          db->multi_definitions[md].base.name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 156 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 157 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    error = utf8lex_multi_definition_resolve(\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 158 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 159 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "        &(YY_MULTI_DEFINITIONS[%d]),  // self\n",
                          md);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 160 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 161 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "        YY_FIRST_DEFINITION);  // db\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 162 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 163 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    if (error == UTF8LEX_ERROR_NOT_FOUND) {\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.1 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.1 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "      utf8lex_reference_t *unresolved_reference =\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.2 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.2 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "        YY_MULTI_DEFINITIONS[%d].references;\n",
                          md);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.3 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.3 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "      for (int infinite_loop_protector = 0;\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.4 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.4 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "           infinite_loop_protector < UTF8LEX_REFERENCES_LENGTH_MAX;\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.5 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.5 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "           infinite_loop_protector ++)\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.6 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.6 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "      {\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.7 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.7 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "        if (unresolved_reference == NULL) {\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.8 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.8 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "          break;\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.9 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.9 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "        }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.10 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.10 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "        else if (unresolved_reference->definition_or_null == NULL) {\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.11 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.11 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "          // This is the unresolved definition.\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.12 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.12 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "          break;\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.13 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.13 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "        }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.14 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.14 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "        unresolved_reference = unresolved_reference->next;\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.15 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.15 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "      }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.16 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.16 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "      if (unresolved_reference != NULL) {\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.17 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.17 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "        fprintf(stderr, \"ERROR utf8lex No such definition [%%d.%%d]: %%s\\n\",\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.18 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.18 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "                YY_STATE.loc[UTF8LEX_UNIT_LINE].start + 1,\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.19 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.19 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "                YY_STATE.loc[UTF8LEX_UNIT_CHAR].start,\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.20 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
                                           UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.20 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
                                           UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "                unresolved_reference->definition_name);\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.21 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.21 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "        return error;\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.22 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.22 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "      }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.23 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.23 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
    line_bytes = snprintf(line, max_bytes,
                          "    }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 163.24 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165.24 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    if (error != UTF8LEX_OK) { return error; }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 164 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 165 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }
  }


  // Rules:
  // ====================
  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 166 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // Rules:\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 167 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 168 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    // =================================================================\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 169 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 170 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  utf8lex_rule_t *rule = db->rules_db;
  int rn = 0;
  strcpy(previous, "NULL");
  is_first = true;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < db->num_rules;
       infinite_loop_protector ++)
  {
    if (rule == NULL)
    {
      fprintf(stderr, "ERROR 171 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    bytes_written = write(fd_out, newline, newline_bytes);
    if (bytes_written != newline_bytes) {
      fprintf(stderr, "ERROR 172 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    // Rule # %d: %s\n",
                          rn,
                          rule->name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 173 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 174 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    error = utf8lex_definition_find_by_id(\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 175 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 176 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                YY_FIRST_DEFINITION,  // first_definition\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 177 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 178 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                (uint32_t) %u,  // id (\"%s\")\n",
                          (unsigned int) rule->definition->id,
                          rule->definition->name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 179 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 180 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                &rule_definition);  // found_pointer\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 181 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 182 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    if (error != UTF8LEX_OK) { return error; }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 183 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 184 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    error = utf8lex_rule_init(\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 185 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
                                                         UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 186 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
                                                         UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                &(YY_RULES[%d]),  // self\n",
                          rn);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 187 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 188 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                %s,  // prev\n",
                          previous);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 189 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 190 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                \"%s\",  // name\n",
                          rule->name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 191 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 192 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                rule_definition,  // definition\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 193 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 194 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                \"\",  // code\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 195 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 196 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "                (size_t) 0);  // code_length_bytes\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 197 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 198 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "    if (error != UTF8LEX_OK) { return error; }\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 199 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 200 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(previous, UTF8LEX_NAME_LENGTH_MAX + 16,
                          "&(YY_RULES[%d])",
                          rn);
    if (line_bytes >= (UTF8LEX_NAME_LENGTH_MAX + 16)) {
      fprintf(stderr, "ERROR 201 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }

    if (is_first == true)
    {
      line_bytes = snprintf(line, max_bytes,
                            "    YY_FIRST_RULE = %s;\n",
                            previous);
      if (line_bytes >= max_bytes) {
        fprintf(stderr, "ERROR 202 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_BAD_LENGTH;
      }
      bytes_written = write(fd_out, line, line_bytes);
      if (bytes_written != line_bytes) {
        fprintf(stderr, "ERROR 203 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
        return UTF8LEX_ERROR_FILE_WRITE;
      }

      is_first = false;
    }

    rule = rule->next;
    rn ++;
  }

  if (rule != NULL)
  {
    // The rules db has a loop in it somewhere, somehow.
    fprintf(stderr, "ERROR 204 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_INFINITE_LOOP\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (rn != db->num_rules)
  {
    fprintf(stderr, "ERROR 205 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_STATE;
  }
  else if (is_first == true)
  {
    fprintf(stderr, "ERROR generating yy_rules_init() [%d.%d]: no rules in the 2nd section\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_STATE;
  }


  // Close yy_rules_init() function declaration:
  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 206 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    return UTF8LEX_OK;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 207 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 208 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "}\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 209 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 210 in utf8lex_generate_write_rules() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rules()");
  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_generate_write_rule_callbacks(
        int fd_out,
        utf8lex_db_t *db
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_write_rule_callbacks()");

  if (db == NULL)
  {
    fprintf(stderr, "ERROR 211 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    fprintf(stderr, "ERROR 212 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_DESCRIPTOR\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  unsigned char line[4096];
  size_t max_bytes = 4096;
  size_t line_bytes;
  size_t bytes_written;

  unsigned char newline[] = "\n";
  size_t newline_bytes = strlen(newline);

  bool is_first;

  utf8lex_error_t error;

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 213 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "static int yy_rule_callback(\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 214 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 215 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        utf8lex_token_t *token\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 216 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 217 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        )\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 218 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 219 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "{\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 220 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 221 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    if (token == NULL\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 222 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 223 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        || token->rule == NULL\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 224 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 225 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        || token->rule->code == NULL)\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 226 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 227 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    {\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 228 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 229 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        return YYerror;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 230 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 231 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    }\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 232 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 233 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  bytes_written = write(fd_out, newline, newline_bytes);
  if (bytes_written != newline_bytes) {
    fprintf(stderr, "ERROR 234 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    switch (token->rule->id)\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 235 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 236 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    {\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 237 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 238 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  utf8lex_rule_t *rule = db->rules_db;
  int rn = 0;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < db->num_rules;
       infinite_loop_protector ++)
  {
    if (rule == NULL)
    {
      fprintf(stderr, "ERROR 239 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
      return UTF8LEX_ERROR_NULL_POINTER;
    }

    line_bytes = snprintf(line, max_bytes,
                          "        case (uint32_t) %d:  // # %d %s\n",
                          rule->id,
                          rn,
                          rule->name);
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 240 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 241 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    // TODO indent the rule's code
    bytes_written = write(fd_out, rule->code, rule->code_length_bytes);
    if (bytes_written != rule->code_length_bytes) {
      fprintf(stderr, "ERROR 242 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    line_bytes = snprintf(line, max_bytes,
                          "            break;\n");
    if (line_bytes >= max_bytes) {
      fprintf(stderr, "ERROR 243 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
    bytes_written = write(fd_out, line, line_bytes);
    if (bytes_written != line_bytes) {
      fprintf(stderr, "ERROR 244 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
      return UTF8LEX_ERROR_FILE_WRITE;
    }

    rule = rule->next;
    rn ++;
  }

  if (rule != NULL)
  {
    // The rules db has a loop in it somewhere, somehow.
    fprintf(stderr, "ERROR 245 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_INFINITE_LOOP\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }
  else if (rn != db->num_rules)
  {
    fprintf(stderr, "ERROR 246 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_STATE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_STATE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "        default:\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 247 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 248 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "            return YYerror;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 249 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 250 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    }\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 251 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 252 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "    return (int) token->rule->id;\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 253 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 254 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  line_bytes = snprintf(line, max_bytes,
                        "}\n");
  if (line_bytes >= max_bytes) {
    fprintf(stderr, "ERROR 255 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_BAD_LENGTH\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  bytes_written = write(fd_out, line, line_bytes);
  if (bytes_written != line_bytes) {
    fprintf(stderr, "ERROR 256 in utf8lex_generate_write_rule_callbacks() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

UTF8LEX_DEBUG("EXIT utf8lex_generate_write_rule_callbacks()");
  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_generate_parse(
        const utf8lex_target_language_t *target_language,
        utf8lex_state_t *state_pointer,
        utf8lex_buffer_t *lex_file,
        int fd_out
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_generate_parse()");

  int i = 5;
  if (target_language == NULL
      || state_pointer == NULL)
  {
    fprintf(stderr, "ERROR 257 in utf8lex_generate_parse() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (fd_out < 0)
  {
    fprintf(stderr, "ERROR 258 in utf8lex_generate_parse() [%d.%d]: UTF8LEX_ERROR_FILE_DESCRIPTOR\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  utf8lex_error_t error;

  // lex: the tokens that we parse in every .l file, as well as
  // lex.db, the database of definitions that this particular
  // .l file generates (we pre-define all the character
  // categories as builtin definitions which can be referenced
  // or overridden in the .l file).
  utf8lex_generate_lexicon_t lex;
  error = utf8lex_generate_init(&lex);
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return error;
  }

  // Get the .l file lexer state set up:
  error = utf8lex_state_init(state_pointer, lex_file);
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return error;
  }

  // This horrid approach will have to do for now.
  // After trying to make the lex grammar context-free, and running
  // out of Christmas break in which to battle that windmill,
  // it's time for a hack.

  // -------------------------------------------------------------------
  // Definitions section:
  // -------------------------------------------------------------------
  bool is_enclosed = false;
  bool is_end_of_section = false;
  int infinite_loop_protector = 0;
  while (true)
  {
    infinite_loop_protector ++;
    if (infinite_loop_protector > UTF8LEX_LEX_FILE_NUM_LINES_MAX)
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_generate_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex Aborting, possible infinite loop [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    // Read a token in from the beginning of the line:
    utf8lex_token_t token;
    error = utf8lex_lex(lex.lex_rules,
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
      utf8lex_generate_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
      return error;
    }

    // Acceptable tokens in the definitions section:
    //   (newline)    Blank line, ignored.
    //   %{           Start enclosed code section.  (Write contents to fd_out.)
    //   %}           End enclosed code section.
    //   (space) ...  Indented code line.
    //   (id) ...     Definition.
    //   %%           Section divider, move on to next section.
    error = UTF8LEX_ERROR_TOKEN;  // Default to invalid token.
    if (lex.newline.id == token.rule->id)
    {
      // (newline)
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_open.id == token.rule->id
             && is_enclosed == false)
    {
      // ${
      is_enclosed = true;
      error = UTF8LEX_OK;
    }
    else if (lex.enclosed_close.id == token.rule->id
             && is_enclosed == true)
    {
      // %}
      is_enclosed = false;
      error = UTF8LEX_OK;
    }
    else if (is_enclosed == true)
    {
      // Inside %{ ... %}
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
      // (space) ...
      utf8lex_generate_write_line(fd_out,
                                  &lex,
                                  state_pointer);
      error = UTF8LEX_OK;
    }
    else if (lex.id.id == token.rule->id)
    {
      // (id) ...
      uint32_t d = lex.db.num_definitions;
      error = utf8lex_token_copy_string(&token,  // self
                                        lex.db.definition_names[d],  // str
                                        UTF8LEX_NAME_LENGTH_MAX);  // max_bytes
      if (error != UTF8LEX_OK)
      {
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return error;
      }
      error = utf8lex_generate_definition(
                  &lex,  // lex
                  state_pointer,  // state
                  lex.db.definition_names[d],  // name
                  false);  // is_rule -- we're not in the rules section.
      if (error != UTF8LEX_OK)
      {
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return error;
      }

      error = UTF8LEX_OK;
    }
    else if (lex.section_divider.id == token.rule->id)
    {
      // %%
      error = utf8lex_lex(lex.lex_rules,
                          state_pointer,
                          &token);
      if (error != UTF8LEX_OK)
      {
        // Error.
        unsigned char some_of_remaining_buffer[32];
        utf8lex_generate_fill_some_of_remaining_buffer(
            some_of_remaining_buffer,
            state_pointer,
            (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
            (size_t) 32);
        fprintf(stderr,
                "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
                some_of_remaining_buffer);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return error;
      }
      else if (lex.newline.id != token.rule->id)
      {
        error = utf8lex_generate_token_error(
            state_pointer,
            &token,
            "Expected newline after %% definitions/rules section divider");
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return error;
      }

      // Move on to the rules section.
      error = UTF8LEX_OK;
      is_end_of_section = true;
      break;
    }
    else
    {
      error = UTF8LEX_ERROR_TOKEN;
    }

    if (error != UTF8LEX_OK)
    {
      error = utf8lex_generate_token_error(
          state_pointer,
          &token,
          "Unexpected token in definitions section");
      UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
      return error;
    }

    if (is_end_of_section == true)
    {
      break;
    }
  }

  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return error;
  }


  // Resolve all the multi-definitions from the definitions section
  // (if any).
  int num_multi_definitions = lex.db.num_multi_definitions;
  for (int md = 0; md < num_multi_definitions; md ++)
  {
    error = utf8lex_multi_definition_resolve(
                &(lex.db.multi_definitions[md]),  // self,
                lex.db.definitions_db);  // db
    if (error == UTF8LEX_ERROR_NOT_FOUND) {
      utf8lex_reference_t *unresolved_reference =
        lex.db.multi_definitions[md].references;
      for (int infinite_loop_protector = 0;
           infinite_loop_protector < UTF8LEX_REFERENCES_LENGTH_MAX;
           infinite_loop_protector ++)
      {
        if (unresolved_reference == NULL) {
          break;
        }
        else if (unresolved_reference->definition_or_null == NULL) {
          // This is the unresolved definition.
          break;
        }
        unresolved_reference = unresolved_reference->next;
      }
      if (unresolved_reference != NULL) {
        fprintf(stderr, "ERROR utf8lex No such definition [%d.%d]: %s\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
                unresolved_reference->definition_name);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return error;
      }
    }
    if (error != UTF8LEX_OK) {
      UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
      return error;
    }
  }


  // -------------------------------------------------------------------
  // Rules section:
  // -------------------------------------------------------------------
  is_enclosed = false;
  is_end_of_section = false;
  infinite_loop_protector = 0;
  while (true)
  {
    infinite_loop_protector ++;
    if (infinite_loop_protector > UTF8LEX_LEX_FILE_NUM_LINES_MAX)
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_generate_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex Aborting, possible infinite loop [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
      return UTF8LEX_ERROR_INFINITE_LOOP;
    }

    // Read a token in from the beginning of the line:
    utf8lex_token_t token;
    error = utf8lex_lex(lex.lex_rules,
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
      utf8lex_generate_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
      return error;
    }

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
    else if (lex.section_divider.id == token.rule->id)
    {
      error = utf8lex_lex(lex.lex_rules,
                          state_pointer,
                          &token);
      if (error != UTF8LEX_OK)
      {
        // Error.
        unsigned char some_of_remaining_buffer[32];
        utf8lex_generate_fill_some_of_remaining_buffer(
            some_of_remaining_buffer,
            state_pointer,
            (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
            (size_t) 32);
        fprintf(stderr,
                "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
                some_of_remaining_buffer);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return error;
      }
      else if (lex.newline.id != token.rule->id)
      {
        error = utf8lex_generate_token_error(
            state_pointer,
            &token,
            "Expected newline after %% rules/user code section divider");
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return error;
      }

      // Move on to the user code section.
      error = UTF8LEX_OK;
      is_end_of_section = true;
      break;
    }
    else
    {
      // rule: (definition-without-id) {rule} or just (definition-without-id).
      // Note that the rule code could sprawl across multiple lines
      // (not yet handled).
      // Backup the buffer and state pointers to where the token pointer is:
      for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
           unit < UTF8LEX_UNIT_MAX;
           unit ++)
      {
        if (token.loc[unit].after == -1)
        {
          int length_units = token.loc[unit].length;
          state_pointer->buffer->loc[unit].start -= token.loc[unit].length;
          state_pointer->loc[unit].start -= token.loc[unit].length;
        }
        else
        {
          state_pointer->buffer->loc[unit].start = token.loc[unit].start;
          state_pointer->loc[unit].start = token.loc[unit].start;
        }
      }

      uint32_t r = lex.db.num_rules;
      int printed = snprintf(lex.db.rule_names[r],  // str
                             (size_t) UTF8LEX_NAME_LENGTH_MAX,  // size
                             "rule_%d",  // format
                             (int) r + 1);
      if (printed < 0)
      {
        fprintf(stderr, "ERROR 259 in utf8lex_generate_parse() [%d.%d]: UTF8LEX_ERROR_TOKEN\n",
                state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
                state_pointer->loc[UTF8LEX_UNIT_CHAR].start);
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return UTF8LEX_ERROR_TOKEN;
      }

      error = utf8lex_generate_definition(
                  &lex,  // lex
                  state_pointer,  // state
                  lex.db.rule_names[r],  // name
                  true);  // is_rule -- we're in the rules section.
      if (error != UTF8LEX_OK)
      {
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return error;
      }

      error = utf8lex_rule_init(
                  &(lex.db.rules[r]),  // self
                  lex.db.last_rule,  // prev
                  lex.db.rule_names[r],  // name
                  lex.db.last_definition,  // definition
                  lex.db.rule_code[r],  // code
                  strlen(lex.db.rule_code[r]));  // code_length_bytes
      if (error != UTF8LEX_OK)
      {
        UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
        return error;
      }

      if (r == 0)
      {
        lex.db.rules_db = &(lex.db.rules[r]);
      }
      lex.db.last_rule = &(lex.db.rules[r]);
      lex.db.num_rules ++;

      error = UTF8LEX_OK;
    }

    if (error != UTF8LEX_OK)
    {
      error = utf8lex_generate_token_error(
          state_pointer,
          &token,
          "Unexpected token in rules section");
      UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
      return error;
    }

    if (is_end_of_section == true)
    {
      break;
    }
  }

  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return error;
  }


  // Now write out the definitions and rules to fd_out:
  error = utf8lex_generate_write_rules(fd_out,
                                       &(lex.db));
  if (error != UTF8LEX_OK) {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return error;
  }

  error = utf8lex_generate_write_rule_callbacks(fd_out,
                                                &(lex.db));
  if (error != UTF8LEX_OK) {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return error;
  }


  // -------------------------------------------------------------------
  // User code section:
  // -------------------------------------------------------------------
  infinite_loop_protector = 0;
  while (true)
  {
    infinite_loop_protector ++;
    if (infinite_loop_protector > UTF8LEX_LEX_FILE_NUM_LINES_MAX)
    {
      unsigned char some_of_remaining_buffer[32];
      utf8lex_generate_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex Aborting, possible infinite loop [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
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
      utf8lex_generate_fill_some_of_remaining_buffer(
          some_of_remaining_buffer,
          state_pointer,
          (size_t) state_pointer->buffer->loc[UTF8LEX_UNIT_BYTE].length,
          (size_t) 32);
      fprintf(stderr,
              "ERROR utf8lex_file_parse() failed to parse [%d.%d]: \"%s\"\n",
              state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
              state_pointer->loc[UTF8LEX_UNIT_CHAR].start,
              some_of_remaining_buffer);
      UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
      return error;
    }
  }

  if (error != UTF8LEX_EOF)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return error;
  }

  error = utf8lex_generate_clear(&lex);
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
    return error;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_generate_parse()");
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
  UTF8LEX_DEBUG("ENTER utf8lex_generate()");

  if (target_language == NULL
      || target_language->extension == NULL
      || lex_file_path == NULL
      || template_dir_path == NULL
      || generated_file_path == NULL
      || state_pointer == NULL)
  {
    fprintf(stderr, "ERROR 260 in utf8lex_generate() [%d.%d]: UTF8LEX_ERROR_NULL_POINTER\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate()");
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
    UTF8LEX_DEBUG("EXIT utf8lex_generate()");
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
    UTF8LEX_DEBUG("EXIT utf8lex_generate()");
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
    UTF8LEX_DEBUG("EXIT utf8lex_generate()");
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
    fprintf(stderr, "ERROR 261 in utf8lex_generate() [%d.%d]: UTF8LEX_ERROR_FILE_OPEN\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate()");
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
    fprintf(stderr, "ERROR 262 in utf8lex_generate() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n", /* line */ 0, /* char */ 0);
    UTF8LEX_DEBUG("EXIT utf8lex_generate()");
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
    UTF8LEX_DEBUG("EXIT utf8lex_generate()");
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
    fprintf(stderr, "ERROR 263 in utf8lex_generate() [%d.%d]: UTF8LEX_ERROR_FILE_WRITE\n",
            state_pointer->loc[UTF8LEX_UNIT_LINE].start + 1,
            state_pointer->loc[UTF8LEX_UNIT_CHAR].start);
    UTF8LEX_DEBUG("EXIT utf8lex_generate()");
    return UTF8LEX_ERROR_FILE_WRITE;
  }

  close(fd_out);
  state_pointer->buffer = NULL;
  utf8lex_buffer_munmap(&lex_file_buffer);
  utf8lex_buffer_munmap(&head_buffer);
  utf8lex_buffer_munmap(&tail_buffer);

  UTF8LEX_DEBUG("EXIT utf8lex_generate()");
  return UTF8LEX_OK;
}
