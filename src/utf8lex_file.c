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
#include <fcntl.h>  // For open()
#include <stdbool.h>  // For bool, true, false.
#include <unistd.h>  // For close(), read()

#include <sys/stat.h>  // For fstat()
#include <sys/mman.h>  // For mmap()

#include "utf8lex.h"


// Do NOT call utf8lex_buffer_init() before calling utf8lex_buffer_mmap().
utf8lex_error_t utf8lex_buffer_mmap(
        utf8lex_buffer_t *self,
        unsigned char *path
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_buffer_mmap()");

  if (self == NULL
      || self->loc == NULL
      || self->str == NULL
      || path == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_mmap(): UTF8LEX_ERROR_NULL_POINTER");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->str->bytes != NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_mmap(): UTF8LEX_ERROR_BUFFER_INITIALIZED");
    return UTF8LEX_ERROR_BUFFER_INITIALIZED;
  }

  // First mmap the file:
  int fd = open(path, O_RDONLY);
  if (fd < 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_mmap(): UTF8LEX_ERROR_FILE_OPEN_READ");
    return UTF8LEX_ERROR_FILE_OPEN_READ;
  }

  struct stat file_statistics;
  int error_code = fstat(fd, &file_statistics);
  if (error_code != 0)
  {
    close(fd);
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_mmap(): UTF8LEX_ERROR_FILE_SIZE");
    return UTF8LEX_ERROR_FILE_SIZE;
  }

  size_t file_size = file_statistics.st_size;
  if (file_size <= (size_t) 0)  // mmap() requires a length > 0.
  {
    close(fd);
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_mmap(): UTF8LEX_ERROR_FILE_EMPTY");
    return UTF8LEX_ERROR_FILE_EMPTY;
  }

  void *mapped_file = mmap((void *) NULL,  // addr
                           (size_t) file_size,  // length
                           PROT_READ,  // prot
                           MAP_PRIVATE,  // flags
                           fd,  // Open fd
                           (off_t) 0);  // offset
  if (mapped_file == MAP_FAILED
      || mapped_file == NULL)
  {
    close(fd);
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_mmap(): UTF8LEX_ERROR_FILE_MMAP");
    return UTF8LEX_ERROR_FILE_MMAP;
  }

  close(fd);  // mmap doesn't need the file descriptor to remain open.

  // Now set up the buffer to point to the mmap'ed file:
  self->next = NULL;
  self->prev = NULL;

  self->fd = -1;
  self->fp = NULL;

  self->str->bytes = (unsigned char *) mapped_file;
  self->str->max_length_bytes = file_size;
  self->str->length_bytes = file_size;

  self->is_eof = true;

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = 0;
    self->loc[unit].length = 0;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_buffer_mmap(): UTF8LEX_OK");
  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_buffer_munmap(
        utf8lex_buffer_t *self
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_buffer_munmap()");

  if (self == NULL
      || self->loc == NULL
      || self->str == NULL
      || self->str->bytes == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_munmap(): UTF8LEX_ERROR_NULL_POINTER");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->str->max_length_bytes <= (size_t) 0
           || self->str->length_bytes <= (size_t) 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_munmap(): UTF8LEX_ERROR_BAD_LENGTH");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  int munmap_error = munmap(self->str->bytes, self->str->length_bytes);
  if (munmap_error != 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_munmap(): UTF8LEX_ERROR_FILE_MMAP");
    return UTF8LEX_ERROR_FILE_MMAP;
  }

  self->fd = -1;
  self->fp = NULL;

  self->str->max_length_bytes = (size_t) -1;
  self->str->length_bytes = (size_t) -1;
  self->str->bytes = NULL;

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_buffer_munmap(): UTF8LEX_OK");
  return UTF8LEX_OK;
}


// Call utf8lex_buffer_init() first, then utf8lex_buffer_read().
utf8lex_error_t utf8lex_buffer_read(
        utf8lex_buffer_t *self,
        int fd
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_buffer_read()");

  if (self == NULL
      || self->loc == NULL
      || self->str == NULL
      || self->str->bytes == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_read(): UTF8LEX_ERROR_NULL_POINTER");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->str->max_length_bytes <= (size_t) 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_read(): UTF8LEX_ERROR_BAD_LENGTH");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  else if (fd < 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_read(): UTF8LEX_ERROR_FILE_DESCRIPTOR");
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  size_t read_length_bytes = self->str->max_length_bytes - 1;

  size_t length_bytes = read(fd,
                             self->str->bytes,
                             read_length_bytes);
  if (length_bytes < (size_t) 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_read(): UTF8LEX_ERROR_FILE_READ");
    return UTF8LEX_ERROR_FILE_READ;
  }

  // Now set up the buffer to point to the mmap'ed file:
  self->next = NULL;
  // Leave self->prev alone.

  self->fd = fd;
  self->fp = NULL;

  self->str->bytes[length_bytes] = 0;
  self->str->length_bytes = length_bytes;

  if (length_bytes < read_length_bytes)
  {
    self->is_eof = true;
  }
  else
  {
    self->is_eof = false;
  }

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = 0;
    self->loc[unit].length = 0;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_buffer_read(): UTF8LEX_OK");
  return UTF8LEX_OK;
}


// Call utf8lex_buffer_init() first, then utf8lex_buffer_readf().
utf8lex_error_t utf8lex_buffer_readf(
        utf8lex_buffer_t *self,
        FILE *fp
        )
{
  UTF8LEX_DEBUG("ENTER utf8lex_buffer_readf()");

  if (self == NULL
      || self->loc == NULL
      || self->str == NULL
      || self->str->bytes == NULL
      || fp == NULL)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_readf(): UTF8LEX_ERROR_NULL_POINTER");
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->str->max_length_bytes <= (size_t) 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_readf(): UTF8LEX_ERROR_BAD_LENGTH");
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  size_t read_length_bytes = self->str->max_length_bytes - 1;

  size_t length_bytes = fread(self->str->bytes,
                              sizeof(unsigned char),
                              read_length_bytes,
                              fp);
  if (length_bytes < (size_t) 0)
  {
    UTF8LEX_DEBUG("EXIT utf8lex_buffer_readf(): UTF8LEX_ERROR_FILE_READ");
    return UTF8LEX_ERROR_FILE_READ;
  }

  // Now set up the buffer to point to the mmap'ed file:
  self->next = NULL;
  // Leave self->prev alone.

  self->fd = -1;
  self->fp = fp;

  self->str->bytes[length_bytes] = 0;
  self->str->length_bytes = length_bytes;

  if (length_bytes < read_length_bytes)
  {
    self->is_eof = true;
  }
  else
  {
    self->is_eof = false;
  }

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = 0;
    self->loc[unit].length = 0;
  }

  UTF8LEX_DEBUG("EXIT utf8lex_buffer_readf(): UTF8LEX_OK");
  return UTF8LEX_OK;
}
