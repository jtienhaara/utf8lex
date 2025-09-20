/*
 * utf8lex
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
#include <string.h>  // For strcpy()
#include <unistd.h>  // For read()

#include "utf8lex.h"


//
// Generate C code that uses mmap() on the .l source file.
//
// lex_dir:
//     Directory from which the source lex file will be read.
//
// template_dir:
//     Source directory for the head and tail files.
// 
// generated_dir:
//     Directory into which the target source code file
//     will be written.
// 
// name:
//     The base name (without path or extension) of the .l lex file
//     and of the generated source code file.
//
static utf8lex_error_t yylex_from_template(
        unsigned char *lex_dir,
        unsigned char *template_dir,
        unsigned char *generated_dir,
        unsigned char *name,
        utf8lex_settings_t *settings
        )
{
  UTF8LEX_DEBUG("ENTER yylex_from_template()");

  if (lex_dir == NULL
      || template_dir == NULL
      || generated_dir == NULL
      || name == NULL
      || settings == NULL)
  {
    UTF8LEX_DEBUG("EXIT yylex_from_template()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error;

  utf8lex_state_t state;
  state.buffer = NULL;

  unsigned char *lex_extension = ".l";
  unsigned char *path_sep = "/";  // TODO OS independent
  unsigned char *generated_extension = TARGET_LANGUAGE_C->extension;

  size_t lex_dir_length = strlen(lex_dir);
  size_t generated_dir_length = strlen(generated_dir);
  size_t path_sep_length = strlen(path_sep);
  size_t name_length = strlen(name);
  size_t lex_extension_length = strlen(lex_extension);
  size_t generated_extension_length = strlen(generated_extension);

  char lex_path[lex_dir_length
                + path_sep_length
                + name_length
                + lex_extension_length
                + 1];
  strcpy(lex_path, lex_dir);
  strcat(lex_path, path_sep);
  strcat(lex_path, name);
  strcat(lex_path, lex_extension);

  char generated_path[generated_dir_length
                + path_sep_length
                + name_length
                + generated_extension_length
                + 1];
  strcpy(generated_path, generated_dir);
  strcat(generated_path, path_sep);
  strcat(generated_path, name);
  strcat(generated_path, generated_extension);

  error = utf8lex_generate(
      TARGET_LANGUAGE_C,  // target_language
      lex_path,           // lex_file_path
      template_dir,       // template_dir_path
      generated_path,     // generated_file_path
      settings,           // settings
      &state);            // state

  if (error != UTF8LEX_OK)
  {
    char error_bytes[256];
    utf8lex_string_t error_string;
    utf8lex_string_init(&error_string,
                        256,  // max_length_bytes
                        0,  // length_bytes
                        &error_bytes[0]);
    utf8lex_error_string(&error_string,
                         error);

    if (state.buffer == NULL)
    {
      // State has not yet been initialized.
      fprintf(stderr,
              "ERROR utf8lex: Failed at [%d.%d] with error code: %d %s\n",
              state.loc[UTF8LEX_UNIT_LINE].start + 1,
              state.loc[UTF8LEX_UNIT_CHAR].start,
              (int) error,
              error_string.bytes);
    }
    else
    {
      // State has been initialized, we might have started lexing.
      char state_bytes[256];
      utf8lex_string_t state_string;
      utf8lex_string_init(&state_string,
                          256,  // max_length_bytes
                          0,  // length_bytes
                          &state_bytes[0]);
      utf8lex_state_string(&state_string,
                           &state);

      off_t offset = (off_t) state.loc[UTF8LEX_UNIT_BYTE].start;
      unsigned char *bad_string = &state.buffer->str->bytes[offset];
      fprintf(stderr,
              "ERROR utf8lex %s: Failed at [%d.%d] with error code: %d %s: \"%s\"\n",
              state.loc[UTF8LEX_UNIT_LINE].start + 1,
              state.loc[UTF8LEX_UNIT_CHAR].start,
              state_string.bytes,
              (int) error,
              error_string.bytes,
              bad_string);
      fflush(stderr);
    }

    UTF8LEX_DEBUG("EXIT yylex_from_template()");
    return error;
  }

  UTF8LEX_DEBUG("EXIT yylex_from_template()");
  return UTF8LEX_OK;
}


static utf8lex_error_t yylex(
        unsigned char *source_l_file,
        utf8lex_settings_t *settings
        )
{
  UTF8LEX_DEBUG("ENTER yylex()");

  utf8lex_error_t error = UTF8LEX_OK;

  // TODO Make this platform-independent:
  unsigned char separator = '/';
  unsigned char dot = '.';
  unsigned char *lex_dir = source_l_file;
  unsigned char *template_dir = "/utf8lex/templates/c/mmap";
  unsigned char *generated_dir = lex_dir;
  unsigned char *name = NULL;
  unsigned char *search = (unsigned char *)
    source_l_file + sizeof(unsigned char) * ( strlen(source_l_file) - 1 );
  unsigned char *extension = NULL;

  // Search backward from end of filename for ".l" then "/".
  while (search != source_l_file)
  {
    if (extension == NULL)
    {
      if (search[0] == dot)
      {
        extension = (unsigned char *) search + 1;
        search[0] = '\0';
      }

      search -= sizeof(unsigned char);
      continue;
    }

    if (search[0] == separator)
    {
      int num_backslashes = 0;
      unsigned char * backslashes_start = NULL;
      for (unsigned char *bs = (unsigned char *) search - sizeof(unsigned char);
           bs >= source_l_file;
           bs -= sizeof(unsigned char))
      {
        if (bs[0] == '\\')
        {
          num_backslashes ++;
          backslashes_start = bs;
        }
        else
        {
          break;
        }
      }

      if ((num_backslashes % 2) != 0)
      {
        // Not a separator, e.g. \/ or \\\/.
        search -= sizeof(unsigned char);
        continue;
      }

      // Separator, e.g. /.
      name = (unsigned char *) search + sizeof(unsigned char);
      search[0] = '\0';
      break;
    }

    search -= sizeof(unsigned char);
  }

  if (strcmp("l", extension) != 0)
  {
    fprintf(stderr, "ERROR Expected '.l' extension, not '.%s'\n", extension);
    UTF8LEX_DEBUG("EXIT yylex()");
    return UTF8LEX_ERROR_FILE_OPEN_READ;
  }

  if (name == NULL)
  {
    // Just a path, no separator, e.g. "foo.l".
    // Use "." as the path, and the full "foo" as the name.
    name = source_l_file;
    lex_dir = ".";
    generated_dir = ".";
  }

  // Generate C code that uses mmap():
  error = yylex_from_template(
              lex_dir,
              template_dir,
              generated_dir,
              name,
              settings);
  if (error != UTF8LEX_OK)
  {
    UTF8LEX_DEBUG("EXIT yylex()");
    return error;
  }

  UTF8LEX_DEBUG("EXIT yylex()");
  return UTF8LEX_OK;
}


int main(
        int argc,
        unsigned char *argv[]
        )
{
  UTF8LEX_DEBUG("ENTER main()");

  utf8lex_settings_t settings;
  utf8lex_settings_init(&settings,    // self
                        NULL,         // input_filename
                        NULL,         // output_filename
                        false);       // is_tracing

  
  unsigned char *source_l_file = NULL;
  for (int a = 1; a < argc; a ++)
  {
    if (a == (argc - 1))
    {
      source_l_file = argv[a];
      settings.input_filename = source_l_file;
    }
    else if (strcmp("--output", argv[a]) == 0)
    {
      settings.output_filename = argv[a + 1];
      a ++;  // Can consume the input_filename, which will lead to error/usage.
    }
    else if (strcmp("--tracing", argv[a]) == 0)
    {
      settings.is_tracing = true;
    }
    else
    {
      fprintf(stderr, "ERROR Unrecognized option: '%s'.\n", argv[a]);
    }
  }

  if (source_l_file == NULL)
  {
    fprintf(stderr, "Usage: %s (option)... (lex-file)\n",
            argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "(option):\n");
    fprintf(stderr, "    --output (filename):\n");
    fprintf(stderr, "        Specifies the path to the .c file to generate.\n");
    fprintf(stderr, "    --tracing:\n");
    fprintf(stderr, "        Enables stdout tracing through definitions and rules.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "(lex-file):\n");
    fprintf(stderr, "    Full path to the .l file to source.\n");
    fprintf(stderr, "    A .c file will be generated in the same directory,\n");
    fprintf(stderr, "    or at the path specified by --output (filename).\n");
    UTF8LEX_DEBUG("EXIT main()");
    return 1;
  }

  utf8lex_error_t error = yylex(source_l_file, &settings);

  utf8lex_settings_clear(&settings);  // self

  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS lexing\n");
    fflush(stdout);
    fflush(stderr);
    UTF8LEX_DEBUG("EXIT main()");
    return 0;
  }
  else
  {
    fflush(stdout);
    fflush(stderr);
    UTF8LEX_DEBUG("EXIT main()");
    return (int) error;
  }
}
