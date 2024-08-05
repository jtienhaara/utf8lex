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
#include <unistd.h>  // For execl(), fork(), getcwd()
#include <sys/wait.h>  // For waitpid()

#include "utf8lex.h"

extern utf8lex_error_t yylex_start(
        unsigned char *path
        );
extern int yyutf8lex(
        utf8lex_token_t *token_or_null,
        utf8lex_lloc_t *location_or_null
        );
extern utf8lex_error_t yylex_end();

// Tests a .c file generated by utf8lex from a .l file.
int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s (input_file) (expected_output_file) (actual_output_file)\n",
            argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "(input_file):\n");
    fprintf(stderr, "    A text file to analyze with the linked lexer.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "(expected_output_file):\n");
    fprintf(stderr, "    A text file of expected output.\n");
    fprintf(stderr, "    The %0 test harness outputs lines such as:\n",
            argv[0]);
    fprintf(stderr, "        TOKEN: xyz_token \"Hello\\n world\"\n");
    fprintf(stderr, "    with the rule name and token's matching text, and:\n");
    fprintf(stderr, "        EOF\n");
    fprintf(stderr, "    and:\n");
    fprintf(stderr, "        ERROR -2\n");
    fprintf(stderr, "    with the error number returned by yyutf8lex().\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "(actual_output_file):\n");
    fprintf(stderr, "    Where to write the actual output, then diff files.\n");
    fprintf(stderr, "\n");
    return 1;
  }

  char *input_file_path = argv[1];
  char *expected_output_file_path = argv[2];
  char *actual_output_file_path = argv[3];

  FILE *fp_out = fopen(actual_output_file_path,
                       "w");
  if (fp_out == NULL)
  {
    fprintf(stderr, "ERROR Failed to open actual output file '%s'\n",
            actual_output_file_path);
    return 1;
  }

  utf8lex_error_t error;

  unsigned char token_str[4096];
  unsigned char printable_str[4096];

  error = yylex_start(input_file_path);
  if (error != UTF8LEX_OK)
  {
    fprintf(stderr, "ERROR Failed yylex_start(\"%s\"): %d\n",
            input_file_path,
            (int) error);
    fclose(fp_out);
    return (int) error;
  }

  utf8lex_token_t token;
  utf8lex_lloc_t location;
  int lex_result = 0;
  while (lex_result >= 0)
  {
    lex_result = yyutf8lex(&token, &location);
    if (lex_result == YYEOF)
    {
      fprintf(fp_out, "EOF\n");
    }
    else if (lex_result == YYerror)
    {
      fprintf(fp_out, "ERROR %d\n",
              lex_result);
    }
    else if (lex_result < 0)
    {
      fprintf(fp_out, "UNKNOWN %d\n",
              lex_result);
    }
    else
    {
      error = utf8lex_token_copy_string(&token,  // self
                                        token_str,  // str
                                        (size_t) 4096);  // max_bytes
      if (error != UTF8LEX_OK)
      {
        fclose(fp_out);
        return error;
      }
      error = utf8lex_printable_str(printable_str,  // printable_str
                                    (size_t) 4096,  // max_bytes
                                    token_str,  // str
                                    UTF8LEX_PRINTABLE_ALL);  // flags
      if (error != UTF8LEX_OK)
      {
        fclose(fp_out);
        return error;
      }

      fprintf(fp_out, "TOKEN: %s \"%s\"\n",
              token.definition->name,
              printable_str);
    }
  }

  error = yylex_end();
  if (error != UTF8LEX_OK)
  {
    fprintf(stderr, "ERROR Failed yylex_end(): %d\n",
            (int) error);
    fclose(fp_out);
    return (int) error;
  }

  fclose(fp_out);

  printf("  Now checking expected vs. actual output...\n");
  fflush(stdout);

  // TODO make this platform independent???
  pid_t diff_pid;
  diff_pid = fork();
  if (diff_pid < 0)
  {
    fprintf(stderr, "ERROR Failed to fork() for diff: %d\n", (int) diff_pid);
    return 1;
  }
  else if (diff_pid == 0)
  {
    printf("diff %s %s:\n",
           expected_output_file_path,
           actual_output_file_path);
    fflush(stdout);

    int exit_code = execl("/usr/bin/diff",
                          "diff",
                          expected_output_file_path,
                          actual_output_file_path,
                          NULL);
    if (exit_code != 0)
    {
      const char error_message[1024];
      fprintf(stderr, "ERROR diffing files %s and %s: ",
              expected_output_file_path,
              actual_output_file_path);
      perror(error_message);
      fflush(stderr);
    }
    fflush(stdout);
    return exit_code;
  }
  else
  {
    // Wait for the child "diff" process to exit, and exit with its exit code.
    int wstatus;
    int wait_error = waitpid(diff_pid,
                             &wstatus,
                             WUNTRACED);

    int exit_code;
    if (WIFEXITED(wstatus))
    {
      exit_code = WEXITSTATUS(wstatus);
    }
    else
    {
      exit_code = 999;
    }

    if (exit_code == 0)
    {
      printf("SUCCESS lexing %s\n",
             input_file_path);
      fflush(stdout);
    }
    else
    {
      fprintf(stderr, "FAILED lexing %s resulted in unexpected output\n",
              input_file_path);
      fflush(stderr);
    }

    return exit_code;
  }
}
