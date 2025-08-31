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

#include "utf8lex.h"


// Print error message to the specified string:
utf8lex_error_t utf8lex_error_string(
        utf8lex_string_t *str,
        utf8lex_error_t error
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_error_string()");

  if (str == NULL
      || str->bytes == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_error_string()");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (error < UTF8LEX_OK
           || error > UTF8LEX_ERROR_MAX)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_error_string()");
    return UTF8LEX_ERROR_BAD_ERROR;
  }

  size_t num_bytes_written;
  switch (error)
  {
  case UTF8LEX_OK:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_OK");
    break;
  case UTF8LEX_EOF:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_EOF");
    break;

  case UTF8LEX_MORE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_MORE");
    break;
  case UTF8LEX_NO_MATCH:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_NO_MATCH");
    break;

  case UTF8LEX_ERROR_NULL_POINTER:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_NULL_POINTER");
    break;

  case UTF8LEX_ERROR_FILE_OPEN_READ:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_FILE_OPEN_READ");
    break;
  case UTF8LEX_ERROR_FILE_OPEN_WRITE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_FILE_OPEN_WRITE");
    break;
  case UTF8LEX_ERROR_FILE_DESCRIPTOR:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_FILE_DESCRIPTOR");
    break;
  case UTF8LEX_ERROR_FILE_EMPTY:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_FILE_EMPTY");
    break;
  case UTF8LEX_ERROR_FILE_MMAP:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_FILE_MMAP");
    break;
  case UTF8LEX_ERROR_FILE_READ:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_FILE_READ");
    break;
  case UTF8LEX_ERROR_FILE_SIZE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_FILE_SIZE");
    break;
  case UTF8LEX_ERROR_FILE_WRITE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_FILE_WRITE");
    break;

  case UTF8LEX_ERROR_BUFFER_INITIALIZED:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BUFFER_INITIALIZED");
    break;
  case UTF8LEX_ERROR_CHAIN_INSERT:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_CHAIN_INSERT");
    break;
  case UTF8LEX_ERROR_CAT:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_CAT");
    break;
  case UTF8LEX_ERROR_DEFINITION_TYPE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_DEFINITION_TYPE");
    break;
  case UTF8LEX_ERROR_EMPTY_DEFINITION:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_EMPTY_DEFINITION");
    break;
  case UTF8LEX_ERROR_MAX_LENGTH:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_MAX_LENGTH");
    break;
  case UTF8LEX_ERROR_NOT_A_RULE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_NOT_A_RULE");
    break;
  case UTF8LEX_ERROR_NOT_FOUND:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_NOT_FOUND");
    break;
  case UTF8LEX_ERROR_NOT_IMPLEMENTED:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_NOT_IMPLEMENTED");
    break;
  case UTF8LEX_ERROR_REGEX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_REGEX");
    break;
  case UTF8LEX_ERROR_UNIT:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_UNIT");
    break;
  case UTF8LEX_ERROR_UNRESOLVED_DEFINITION:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_UNRESOLVED_DEFINITION");
    break;
  case UTF8LEX_ERROR_INFINITE_LOOP:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_INFINITE_LOOP");
    break;
  case UTF8LEX_ERROR_BAD_LENGTH:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_LENGTH");
    break;
  case UTF8LEX_ERROR_BAD_OFFSET:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_OFFSET");
    break;
  case UTF8LEX_ERROR_BAD_START:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_START");
    break;
  case UTF8LEX_ERROR_BAD_AFTER:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_AFTER");
    break;
  case UTF8LEX_ERROR_BAD_HASH:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_HASH");
    break;
  case UTF8LEX_ERROR_BAD_ID:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_ID");
    break;
  case UTF8LEX_ERROR_BAD_MIN:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_MIN");
    break;
  case UTF8LEX_ERROR_BAD_MAX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_MAX");
    break;
  case UTF8LEX_ERROR_BAD_MULTI_TYPE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_MULTI_TYPE");
    break;
  case UTF8LEX_ERROR_BAD_REGEX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_REGEX");
    break;
  case UTF8LEX_ERROR_BAD_UTF8:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_UTF8");
    break;
  case UTF8LEX_ERROR_BAD_ERROR:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_ERROR");
    break;

  case UTF8LEX_ERROR_TOKEN:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_TOKEN");
    break;
  case UTF8LEX_ERROR_STATE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_STATE");
    break;

  case UTF8LEX_ERROR_MAX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_MAX");
    break;

  default:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "unknown error %d", (int) error);
    UTF8LEX_DEBUG("EXIT utf8lex_error_string()");
    return UTF8LEX_ERROR_BAD_ERROR;
  }

  str->length_bytes = num_bytes_written;

  if (num_bytes_written >= str->max_length_bytes)
  {
    // The error string was truncated.
    UTF8LEX_DEBUG("EXIT utf8lex_error_string()");
    return UTF8LEX_MORE;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_error_string()");
  return UTF8LEX_OK;
}
