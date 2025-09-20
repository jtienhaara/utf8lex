WHITESPACE [\h]+
EVERYTHING_ELSE [^\h]+

%%

WHITESPACE {
  printf("Matched WHITESPACE with definition type '%s' name '%s'\n",
         token->definition->definition_type->name,
         token->definition->name);
  fflush(stdout);
}

EVERYTHING_ELSE {
  unsigned char token_str[256];
  utf8lex_error_t err = utf8lex_token_copy_string(token, token_str, 256);
  if (err == UTF8LEX_OK)
  {
    printf("%s\n", token_str);
  }
  else
  {
    printf("Error %d\n", (int) err);
  }
  fflush(stdout);
}

%%
