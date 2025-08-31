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


// Generate C code that uses mmap().
static utf8lex_error_t test_utf8lex_generate_c_mmap(
        unsigned char *lex_dir,
        unsigned char *template_dir,
        unsigned char *generated_dir,
        unsigned char *name,
        utf8lex_settings_t *settings,  // Already initialized.
        utf8lex_state_t *state         // Will be initialized.
        )
{
  if (lex_dir == NULL
      || template_dir == NULL
      || generated_dir == NULL
      || name == NULL
      || state == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error;

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
      lex_path,  // lex_file_path
      template_dir,  // template_dir_path
      generated_path,  // generated_file_path
      settings,  // settings
      state);  // state
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t test_utf8lex_generate(
        unsigned char *lex_dir,
        unsigned char *template_dir,
        unsigned char *generated_dir,
        unsigned char *name,
        utf8lex_settings_t *settings,  // Already initialized.
        utf8lex_state_t *state         // Will be initialized.
        )
{
  if (lex_dir == NULL
      || template_dir == NULL
      || generated_dir == NULL
      || name == NULL
      || state == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_OK;

  // Generate C code that uses mmap():
  error = test_utf8lex_generate_c_mmap(
              lex_dir,
              template_dir,
              generated_dir,
              name,
              settings,
              state);
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
  if (argc != 5)
  {
    fprintf(stderr, "Usage: %s (lex-dir) (template-dir) (generated-dir) (name)\n",
            argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "(lex-dir):\n");
    fprintf(stderr, "    Directory from which the source lex file will be read.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "(template-dir):\n");
    fprintf(stderr, "    Source directory for the head and tail files.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "(generated-dir):\n");
    fprintf(stderr, "    Directory into which the target source code file\n");
    fprintf(stderr, "    will be written.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "(name):\n");
    fprintf(stderr, "    The base name (without path or extension) of the .l lex file\n");
    fprintf(stderr, "    and of the generated source code file.\n");
    return 1;
  }

  unsigned char *lex_dir = (unsigned char *) argv[1];
  unsigned char *template_dir = (unsigned char *) argv[2];
  unsigned char *generated_dir = (unsigned char *) argv[3];
  unsigned char *name = (unsigned char *) argv[4];

  utf8lex_settings_t settings;
  utf8lex_settings_init_defaults(&settings);

  utf8lex_state_t state;
  state.buffer = NULL;

  utf8lex_error_t error = test_utf8lex_generate(lex_dir,
                                                template_dir,
                                                generated_dir,
                                                name,
                                                &settings,
                                                &state);
  if (error == UTF8LEX_OK)
  {
    printf("SUCCESS lexing\n");
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

    if (state.buffer == NULL)
    {
      // State has not yet been initialized.
      fprintf(stderr,
              "ERROR utf8lex: Failed with error code: %d %s\n",
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
              "ERROR utf8lex %s: Failed with error code: %d %s: \"%s\"\n",
              state_string.bytes,
              (int) error,
              error_string.bytes,
              bad_string);
      fflush(stderr);
    }

    fflush(stdout);
    fflush(stderr);
    return (int) error;
  }
}
