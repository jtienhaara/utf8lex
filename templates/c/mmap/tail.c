static utf8lex_error_t yylex_start(
        unsigned char *path,
        utf8lex_state_t *state_pointer,
        utf8lex_buffer_t *buffer_pointer,
        utf8lex_string_t *string_pointer
        )
{
  if (path == NULL
      || state_pointer == NULL
      || buffer_pointer == NULL
      || string_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Minimally initialize the string and buffer contents:
  string_pointer->max_length_bytes = -1;
  string_pointer->length_bytes = -1;
  string_pointer->bytes = NULL;

  buffer_pointer->next = NULL;
  buffer_pointer->prev = NULL;
  buffer_pointer->str = string_pointer;

  // mmap the file to be lexed:
  error = utf8lex_buffer_mmap(buffer_pointer,
                              path);  // path
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Initialize the lexing state:
  error = utf8lex_state_init(state, buffer_pointer);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


static int yylex (
        YYSTYPE *lvalp,
        YYLTYPE *llocp,
        utf8lex_state_t *state
        )
{
  utf8lex_token_t token;
  error = utf8lex_lex(FIRST_RULE,  // first_rule
                      state,  // state
                      &token);  // token
  if (error == UTF8LEX_EOF)
  {
    // Nothing more to lex.
    return YYEOF;
  }
  else if (error != UTF8LEX_OK)
  {
    return YYerror;
  }

  int token_code = (int) token.rule->id;

  llocp->first_line = token.loc[UTF8LEX_UNIT_LINE].start;
  llocp->first_column = token.loc[UTF8LEX_UNIT_GRAPHEME].start;
  llocp->last_line =
    token.loc[UTF8LEX_UNIT_LINE].start
    + token.loc[UTF8LEX_UNIT_LINE].length;
  llocp->last_column =
    token.loc[UTF8LEX_UNIT_GRAPHEME].start
    + token.loc[UTF8LEX_UNIT_GRAPHEME].length;

  return token_code;
}


static utf8lex_error_t yylex_end(
        utf8lex_state_t *state
        )
{
  // Unmap the mmap'ed file:
  error = utf8lex_buffer_munmap(state->buffer);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Teardown:
  error = utf8lex_state_clear(state_pointer);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}
