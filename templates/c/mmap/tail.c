// =====================================================================
// Since an entire source file can be mmapped in, when we print an error
// message, we only want to grab some of it.  The yylex_print_error()
// procedure uses this to pull in some context.
// ---------------------------------------------------------------------
static utf8lex_error_t yylex_fill_some_of_remaining_buffer(
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


// =====================================================================
// Prints an error message to stderr, then returns the error code.
// ---------------------------------------------------------------------
static utf8lex_error_t yylex_print_error(
        utf8lex_error_t error
        )
{
  if (error == UTF8LEX_OK)
  {
    return UTF8LEX_OK;
  }
  else if (YY_STATE.buffer == NULL)
  {
    return error;
  }

  unsigned char error_name[256];
  utf8lex_string_t error_string;
  utf8lex_error_t string_error = utf8lex_string(&error_string, 256, error_name);
  string_error = utf8lex_error_string(&error_string, error);
  if (string_error != UTF8LEX_OK)
  {
    return error;
  }

  unsigned char some_of_remaining_buffer[32];
  utf8lex_error_t fill_error = yylex_fill_some_of_remaining_buffer(
      some_of_remaining_buffer,
      &YY_STATE,
      (size_t) YY_STATE.buffer->str->length_bytes,
      (size_t) 32);
  if (fill_error == UTF8LEX_OK)
  {
    fprintf(stderr,
            "ERROR (%s) yylex failed to parse [%d.%d]: \"%s\"\n",
            error_name,
            YY_STATE.loc[UTF8LEX_UNIT_LINE].start + 1,
            YY_STATE.loc[UTF8LEX_UNIT_CHAR].start,
            some_of_remaining_buffer);
  }

  return error;
}


// =====================================================================
// Settings for lexing.
// (If not called, defaults will be used.)
// ---------------------------------------------------------------------
utf8lex_error_t yylex_settings(
        utf8lex_settings_t *settings
        )
{
  if (settings == NULL)
  {
    return yylex_print_error(UTF8LEX_ERROR_NULL_POINTER);
  }

  utf8lex_error_t error = utf8lex_settings_copy(
        settings,       // from
        &YY_SETTINGS);  // to

  if (error != UTF8LEX_OK)
  {
    return yylex_print_error(error);
  }

  YY_IS_SETTINGS_INITIALIZED = true;

  return UTF8LEX_OK;
}


// =====================================================================
// Begin lexing.
// ---------------------------------------------------------------------
utf8lex_error_t yylex_start(
        unsigned char *path
        )
{
  if (path == NULL)
  {
    return yylex_print_error(UTF8LEX_ERROR_NULL_POINTER);
  }

  // Initialize YY_FIRST_RULE, and the database of definitions and rules:
  utf8lex_error_t error = yy_rules_init();
  if (error != UTF8LEX_OK)
  {
    return yylex_print_error(error);
  }

  // Minimally initialize the string and buffer contents:
  YY_STRING.max_length_bytes = -1;
  YY_STRING.length_bytes = -1;
  YY_STRING.bytes = NULL;

  YY_BUFFER.next = NULL;
  YY_BUFFER.prev = NULL;
  YY_BUFFER.str = &YY_STRING;

  // mmap the file to be lexed:
  error = utf8lex_buffer_mmap(&YY_BUFFER,
                              path);  // path
  if (error != UTF8LEX_OK)
  {
    return yylex_print_error(error);
  }

  if (YY_IS_SETTINGS_INITIALIZED != true)
  {
    utf8lex_settings_init(
        &YY_SETTINGS,  // self
        path,          // input_filename
        NULL,          // output_filename
        false);        // is_tracing
  }

  // Initialize the lexing state:
  error = utf8lex_state_init(&YY_STATE,     // self
                             &YY_SETTINGS,  // settings
                             &YY_BUFFER,    // buffer
                             0);            // stack_depth
  if (error != UTF8LEX_OK)
  {
    return yylex_print_error(error);
  }

  return UTF8LEX_OK;
}


// =====================================================================
// Lex with utf8 locations (including grapheme # etc).
// ---------------------------------------------------------------------
int yyutf8lex(
        utf8lex_token_t *token_or_null,
        utf8lex_lloc_t *location_or_null
        )
{
  utf8lex_token_t token;
  utf8lex_token_t *token_pointer;
  if (token_or_null == NULL)
  {
    token_pointer = &token;
  }
  else
  {
    token_pointer = token_or_null;
  }

  utf8lex_error_t error = utf8lex_lex(YY_FIRST_RULE,  // first_rule
                                      &YY_STATE,  // state
                                      token_pointer);  // token
  if (error == UTF8LEX_EOF)
  {
    // Nothing more to lex.
    return YYEOF;
  }
  else if (error != UTF8LEX_OK)
  {
    yylex_print_error(error);
    return YYerror;
  }

  int token_code = (int) token_pointer->rule->id;

  // Execute the rule callback for matched rule:
  token_code = yy_rule_callback(token_pointer);
  if (token_code < 0)
  {
    return token_code;
  }

  if (location_or_null != NULL)
  {
    location_or_null->first_line =
      token_pointer->loc[UTF8LEX_UNIT_LINE].start;
    location_or_null->first_column =
      token_pointer->loc[UTF8LEX_UNIT_BYTE].start;

    location_or_null->last_line =
      token_pointer->loc[UTF8LEX_UNIT_LINE].start
      + token_pointer->loc[UTF8LEX_UNIT_LINE].length;
    location_or_null->last_column =
      token_pointer->loc[UTF8LEX_UNIT_BYTE].start
      + token_pointer->loc[UTF8LEX_UNIT_BYTE].length;

    location_or_null->start_byte =
      token_pointer->loc[UTF8LEX_UNIT_BYTE].start;
    location_or_null->length_bytes =
      token_pointer->loc[UTF8LEX_UNIT_BYTE].length;
    location_or_null->start_char =
      token_pointer->loc[UTF8LEX_UNIT_CHAR].start;
    location_or_null->length_chars =
      token_pointer->loc[UTF8LEX_UNIT_CHAR].length;
    location_or_null->start_grapheme =
      token_pointer->loc[UTF8LEX_UNIT_GRAPHEME].start;
    location_or_null->length_graphemes =
      token_pointer->loc[UTF8LEX_UNIT_GRAPHEME].length;
    location_or_null->start_line =
      token_pointer->loc[UTF8LEX_UNIT_LINE].start;
    location_or_null->length_lines =
      token_pointer->loc[UTF8LEX_UNIT_LINE].length;
  }

  return token_code;
}


// =====================================================================
// Traditional yylex(), with no location tracking.
// ---------------------------------------------------------------------
int yylex()
{
  return yyutf8lex(NULL,  // token_or_null
                   NULL);  // location_or_null
}


// =====================================================================
// Finish lexing.
// ---------------------------------------------------------------------
utf8lex_error_t yylex_end()
{
  // Unmap the mmap'ed file:
  utf8lex_error_t error = utf8lex_buffer_munmap(YY_STATE.buffer);
  if (error != UTF8LEX_OK)
  {
    return yylex_print_error(error);
  }

  // Teardown:
  error = utf8lex_state_clear(&YY_STATE);
  if (error != UTF8LEX_OK)
  {
    return yylex_print_error(error);
  }

  utf8lex_rule_t *rule = YY_FIRST_RULE;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < UTF8LEX_RULES_DB_LENGTH_MAX;
       infinite_loop_protector ++)
  {
    if (rule == NULL)
    {
      break;
    }

    error = utf8lex_rule_clear(rule);
    if (error != UTF8LEX_OK)
    {
      return yylex_print_error(error);
    }
    rule = rule->next;
  }

  if (rule != NULL)
  {
    return yylex_print_error(UTF8LEX_ERROR_INFINITE_LOOP);
  }

  utf8lex_definition_t *definition = YY_FIRST_DEFINITION;
  for (int infinite_loop_protector = 0;
       infinite_loop_protector < UTF8LEX_DEFINITIONS_DB_LENGTH_MAX;
       infinite_loop_protector ++)
  {
    if (definition == NULL)
    {
      break;
    }

    if (definition->definition_type == NULL
        || definition->definition_type->clear == NULL)
    {
      // Already cleared by clearing a rule.
      definition = definition->next;
      continue;
    }
    error = definition->definition_type->clear(definition);
    if (error != UTF8LEX_OK)
    {
      return yylex_print_error(error);
    }
    definition = definition->next;
  }

  if (definition != NULL)
  {
    return yylex_print_error(UTF8LEX_ERROR_INFINITE_LOOP);
  }

  return UTF8LEX_OK;
}

/*
 * Generated by utf8lex.
 *
 * Requires at runtime:
 *
 *     utf8lex
 *       https://github.com/jtienhaara/utf8lex
 *       Apache 2.0 license
 *
 *     utf8proc
 *       https://juliastrings.github.io/utf8proc
 *       MIT license
 *       Unicode data license
 *
 *     pcre2
 *       https://github.com/PCRE2Project/pcre2
 *       BSD license
 *
 * This generated file is licensed according to the source code with which
 * it is distributed.  The template files (from which this generated file
 * derives) are public domain.
 */
