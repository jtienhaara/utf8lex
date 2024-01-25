utf8lex_error_t yylex_start(
        unsigned char *path
        )
{
  if (path == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Initialize YY_FIRST_RULE, and the database of definitions and rules:
  utf8lex_error_t error = yy_rules_init();
  if (error != UTF8LEX_OK)
  {
    return error;
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
    return error;
  }

  // Initialize the lexing state:
  error = utf8lex_state_init(&YY_STATE,  // self
                             &YY_BUFFER);  // buffer
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


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


// Traditional yylex():
int yylex()
{
  return yyutf8lex(NULL,  // token_or_null
                   NULL);  // location_or_null
}


utf8lex_error_t yylex_end()
{
  // Unmap the mmap'ed file:
  utf8lex_error_t error = utf8lex_buffer_munmap(YY_STATE.buffer);
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  // Teardown:
  error = utf8lex_state_clear(&YY_STATE);
  if (error != UTF8LEX_OK)
  {
    return error;
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
      return error;
    }
    rule = rule->next;
  }

  if (rule != NULL)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
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
      return error;
    }
    definition = definition->next;
  }

  if (definition != NULL)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  return UTF8LEX_OK;
}
