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
  if (self == NULL
      || self->loc == NULL
      || self->str == NULL
      || path == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->str->bytes != NULL)
  {
    return UTF8LEX_ERROR_BUFFER_INITIALIZED;
  }

  // First mmap the file:
  int fd = open(path, O_RDONLY);
  if (fd < 0)
  {
    return UTF8LEX_ERROR_FILE_OPEN;
  }

  struct stat file_statistics;
  int error_code = fstat(fd, &file_statistics);
  if (error_code != 0)
  {
    close(fd);
    return UTF8LEX_ERROR_FILE_SIZE;
  }

  size_t file_size = file_statistics.st_size;
  if (file_size <= (size_t) 0)  // mmap() requires a length > 0.
  {
    close(fd);
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
    return UTF8LEX_ERROR_FILE_MMAP;
  }

  close(fd);  // mmap doesn't need the file descriptor to remain open.

  // Now set up the buffer to point to the mmap'ed file:
  self->next = NULL;
  self->prev = NULL;

  self->fd = -1;

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

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_buffer_munmap(
        utf8lex_buffer_t *self
        )
{
  if (self == NULL
      || self->loc == NULL
      || self->str == NULL
      || self->str->bytes == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->str->max_length_bytes <= (size_t) 0
           || self->str->length_bytes <= (size_t) 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  int munmap_error = munmap(self->str->bytes, self->str->length_bytes);
  if (munmap_error != 0)
  {
    return UTF8LEX_ERROR_FILE_MMAP;
  }

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

  return UTF8LEX_OK;
}


// Call utf8lex_buffer_init() first, then utf8lex_buffer_read().
utf8lex_error_t utf8lex_buffer_read(
        utf8lex_buffer_t *self,
        int fd
        )
{
  if (self == NULL
      || self->loc == NULL
      || self->str == NULL
      || self->str->bytes == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (self->str->max_length_bytes <= (size_t) 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  else if (fd < 0)
  {
    return UTF8LEX_ERROR_FILE_DESCRIPTOR;
  }

  size_t read_length_bytes = self->str->max_length_bytes - 1;

  size_t length_bytes = read(fd,
                             self->str->bytes,
                             read_length_bytes);
  if (length_bytes < (size_t) 0)
  {
    return UTF8LEX_ERROR_FILE_READ;
  }

  // Now set up the buffer to point to the mmap'ed file:
  self->next = NULL;
  // Leave self->prev alone.

  self->fd = fd;

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

  return UTF8LEX_OK;
}

// !!! musaico_error_t *musaico_block_readf(
// !!!         musaico_block_t *block,
// !!!         FILE *fp
// !!!         )
// !!! {
// !!!   if (block == NULL
// !!!       || block->musaico == NULL)
// !!!   {
// !!!     return MUSAICO_NULL_POINTER;
// !!!   }
// !!! 
// !!!   musaico_t *musaico = block->musaico;
// !!! 
// !!!   musaico->trace_step(musaico,
// !!!                       MUSAICO_TRACE_ENTER,
// !!!                       "musaico_block_readf()",
// !!!                       NULL);  // source
// !!! 
// !!!   if (block->buffer == NULL)
// !!!   {
// !!!     musaico->allocate(musaico,
// !!!                       "char[]",
// !!!                       (void **) &block->buffer,
// !!!                       (size_t) MUSAICO_BLOCK_SIZE * sizeof(char));
// !!!     block->flags |= MUSAICO_BLOCK_FLAG_OWNED;
// !!!   }
// !!!   else if (! (block->flags & MUSAICO_BLOCK_FLAG_OWNED))
// !!!   {
// !!!     musaico->trace_step(musaico,
// !!!                         MUSAICO_TRACE_EXIT,
// !!!                         "musaico_block_readf()",
// !!!                         NULL);  // source
// !!! 
// !!!     return musaico->error(musaico,
// !!!                           "musaico_block_readf(%x, %x) write to unowned block",
// !!!                           (long) block,
// !!!                           (long) fp);
// !!!   }
// !!! 
// !!!   size_t length_to_read = MUSAICO_BLOCK_SIZE - (size_t) block->offset;
// !!! 
// !!!   if (length_to_read <= (size_t) 0)
// !!!   {
// !!!     musaico->trace_step(musaico,
// !!!                         MUSAICO_TRACE_EXIT,
// !!!                         "musaico_block_readf()",
// !!!                         NULL);  // source
// !!! 
// !!!     return musaico->error(musaico,
// !!!                           "musaico_block_readf(%x, %x) write to full buffer (offset %d, length %d)",
// !!!                           (long) block,
// !!!                           (long) fp,
// !!!                           block->offset,
// !!!                           (int) block->length);
// !!!   }
// !!! 
// !!!   size_t length = fread(&block->buffer[block->offset],
// !!!                         sizeof(char),
// !!!                         length_to_read,
// !!!                         fp);
// !!! 
// !!!   if (length < length_to_read)
// !!!   {
// !!!     block->flags |= MUSAICO_BLOCK_FLAG_EOF;
// !!!   }
// !!! 
// !!!   if (length < 0)
// !!!   {
// !!!     musaico->trace_step(musaico,
// !!!                         MUSAICO_TRACE_EXIT,
// !!!                         "musaico_block_readf()",
// !!!                         NULL);  // source
// !!! 
// !!!     return musaico->error(musaico,
// !!!                           "musaico_block_readf(%x, %x) failed: $d",
// !!!                           (long) block,
// !!!                           (long) fp,
// !!!                           (int) length);
// !!!   }
// !!! 
// !!!   block->length += length;
// !!!   block->offset += (int) length;
// !!! 
// !!!   musaico->trace_step(musaico,
// !!!                       MUSAICO_TRACE_EXIT,
// !!!                       "musaico_block_readf()",
// !!!                       NULL);  // source
// !!! 
// !!!   return MUSAICO_OK;
// !!! }
