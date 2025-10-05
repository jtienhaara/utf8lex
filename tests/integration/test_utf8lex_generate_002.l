%{
/*
 * Integration test file for utf8lex.
 *
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
%}

COMMENT_MULTI_LINE      COMMENT_MULTI_LINE_OPEN COMMENT_MULTI_LINE_BODY COMMENT_MULTI_LINE_CLOSE
COMMENT_MULTI_LINE_BODY ([^*\\]+|[*][^/\\]|[*]\\/|[^*\\]/|\\.)*

COMMENT_SINGLE_LINE     COMMENT_SINGLE_LINE_OPEN TO_EOL

PATTERN_INT_RANGE       SLASH INT DOT_DOT INT SLASH
PATTERN_FLOAT_RANGE     SLASH FLOAT DOT_DOT FLOAT SLASH
PATTERN_REGEX           PATTERN_REGEX_OPEN PATTERN_REGEX_BODY PATTERN_REGEX_CLOSE
PATTERN_REGEX_BODY      ([^$\\\v]+|[$][^/\\\v]|[$]\\/|\\[^\v])*

STRING_MULTI_LINE       DOUBLE_QUOTES_3 STRING_MULTI_LINE_BODY DOUBLE_QUOTES_3
STRING_MULTI_LINE_BODY  ([^"\\]+|[^"\\]"["]?[^"\\]|[^"\\]"["]?\\.|\\.)*

STRING_DOUBLE_QUOTE     DOUBLE_QUOTE STRING_DOUBLE_QUOTE_BODY DOUBLE_QUOTE
STRING_DOUBLE_QUOTE_BODY ([^"\\\v]+|\\[^\v])*

STRING_SINGLE_QUOTE     SINGLE_QUOTE STRING_SINGLE_QUOTE_BODY SINGLE_QUOTE
STRING_SINGLE_QUOTE_BODY ([^'\\\v]+|\\[^\v])*

DOUBLE_QUOTE "\""
DOUBLE_QUOTES_3 "\"\"\""
SINGLE_QUOTE "'"
SLASH                   "/"
TO_EOL                  [^\v]*

BRACE_CLOSE             "}"
BRACE_OPEN              "{"
BRACKET_CLOSE           "]"
BRACKET_OPEN            "["
COLON                   ":"
COMMA                   ","
COMMENT_MULTI_LINE_OPEN "/*"
COMMENT_MULTI_LINE_CLOSE "*/"
COMMENT_SINGLE_LINE_OPEN "//"
DOT_DOT_DOT             "..."
DOT_DOT                 ".."
DOT                     "."
FLOAT                   [-+]?[0-9]*\.[0-9]+(e[-+]?([0-9]*\.)?[0-9]+)?
ID                      [_\p{L}][_\p{L}\p{N}]*
INT                     [-+]?[0-9]+(e[+]?[0-9]+)?
NEWLINE                 [\r]?\n
OP_AND                  "&"
OP_AND_AND              "&&"
OP_ASSIGN               "="
OP_ASSIGN_AND           "&="
OP_ASSIGN_AND_AND       "&&="
OP_ASSIGN_DECREMENT     "--"
OP_ASSIGN_DIVIDE        "/="
OP_ASSIGN_INCREMENT     "++"
OP_ASSIGN_MINUS         "-="
OP_ASSIGN_MODULO        "%="
OP_ASSIGN_OR            "|="
OP_ASSIGN_OR_OR         "||="
OP_ASSIGN_PLUS          "+="
OP_ASSIGN_POWER         "**="
OP_ASSIGN_TIMES         "*="
OP_ASSIGN_XOR           "^="
OP_ASSIGN_XOR_XOR       "^^="
OP_AT                   "@"
OP_DIVIDED_BY           SLASH
OP_DOLLAR               "$"
OP_EQUAL                "=="
OP_GREATER_THAN         ">"
OP_GREATER_THAN_EQUAL   ">="
OP_HASH                 "#"
OP_LESS_THAN            "<"
OP_LESS_THAN_EQUAL      "<="
OP_MINUS                "-"
OP_MODULO               "%"
OP_NOT                  "!"
OP_NOT_EQUAL            "!="
OP_OR                   "|"
OP_OR_OR                "||"
OP_PLUS                 "+"
OP_POWER                "**"
OP_QUERY                "?"
OP_TILDE                "~"
OP_TILDE_EQUAL          "~="
OP_TIMES                "*"
OP_XOR                  "^"
OP_XOR_XOR              "^^"
PARENTHESIS_CLOSE       ")"
PARENTHESIS_OPEN        "("
PATTERN_REGEX_OPEN      "/^"
PATTERN_REGEX_CLOSE     "$/"
SEMI_COLON              ";"
TIMES                   "*"
WHITESPACE              [\h]

%%

COMMENT_MULTI_LINE { char comment_body[256]; utf8lex_sub_token_t *sub_token = NULL; utf8lex_error_t error = utf8lex_sub_token_find(token->sub_tokens, "COMMENT_MULTI_LINE_BODY", 0, &sub_token); if (error != UTF8LEX_OK) { comment_body[0] = '\0'; } else { utf8lex_sub_token_copy_string(sub_token, comment_body, 256); } printf("!!! COMMENT_MULTI_LINE with body: %s\n", comment_body); fflush(stdout); }
COMMENT_SINGLE_LINE { printf("!!! COMMENT_SINGLE_LINE\n"); fflush(stdout); }

PATTERN_INT_RANGE { printf("!!! PATTERN_INT_RANGE\n"); fflush(stdout); }
PATTERN_FLOAT_RANGE { printf("!!! PATTERN_FLOAT_RANGE\n"); fflush(stdout); }
PATTERN_REGEX { printf("!!! PATTERN_REGEX\n"); fflush(stdout); }

STRING_MULTI_LINE { printf("!!! STRING_MULTI_LINE\n"); fflush(stdout); }
STRING_DOUBLE_QUOTE { printf("!!! STRING_DOUBLE_QUOTE\n"); fflush(stdout); }
STRING_SINGLE_QUOTE { printf("!!! STRING_SINGLE_QUOTE\n"); fflush(stdout); }

BRACE_CLOSE { printf("!!! BRACE_CLOSE\n"); fflush(stdout); }
BRACE_OPEN { printf("!!! BRACE_OPEN\n"); fflush(stdout); }
BRACKET_CLOSE { printf("!!! BRACKET_CLOSE\n"); fflush(stdout); }
BRACKET_OPEN { printf("!!! BRACKET_OPEN\n"); fflush(stdout); }
COLON { printf("!!! COLON\n"); fflush(stdout); }
COMMA { printf("!!! COMMA\n"); fflush(stdout); }
DOT_DOT_DOT { printf("!!! DOT_DOT_DOT\n"); fflush(stdout); }
DOT_DOT { printf("!!! DOT_DOT\n"); fflush(stdout); }
DOT { printf("!!! DOT\n"); fflush(stdout); }
FLOAT { printf("!!! FLOAT\n"); fflush(stdout); }
ID { printf("!!! ID\n"); fflush(stdout); }
INT { printf("!!! INT\n"); fflush(stdout); }
NEWLINE { printf("!!! NEWLINE\n"); fflush(stdout); }
OP_AND { printf("!!! OP_AND\n"); fflush(stdout); }
OP_AND_AND { printf("!!! OP_AND_AND\n"); fflush(stdout); }
OP_ASSIGN { printf("!!! OP_ASSIGN\n"); fflush(stdout); }
OP_ASSIGN_AND { printf("!!! OP_ASSIGN_AND\n"); fflush(stdout); }
OP_ASSIGN_AND_AND { printf("!!! OP_ASSIGN_AND_AND\n"); fflush(stdout); }
OP_ASSIGN_DECREMENT { printf("!!! OP_ASSIGN_DECREMENT\n"); fflush(stdout); }
OP_ASSIGN_DIVIDE { printf("!!! OP_ASSIGN_DIVIDE\n"); fflush(stdout); }
OP_ASSIGN_INCREMENT { printf("!!! OP_ASSIGN_INCREMENT\n"); fflush(stdout); }
OP_ASSIGN_MINUS { printf("!!! OP_ASSIGN_MINUS\n"); fflush(stdout); }
OP_ASSIGN_MODULO { printf("!!! OP_ASSIGN_MODULO\n"); fflush(stdout); }
OP_ASSIGN_OR { printf("!!! OP_ASSIGN_OR\n"); fflush(stdout); }
OP_ASSIGN_OR_OR { printf("!!! OP_ASSIGN_OR_OR\n"); fflush(stdout); }
OP_ASSIGN_PLUS { printf("!!! OP_ASSIGN_PLUS\n"); fflush(stdout); }
OP_ASSIGN_POWER { printf("!!! OP_ASSIGN_POWER\n"); fflush(stdout); }
OP_ASSIGN_TIMES { printf("!!! OP_ASSIGN_TIMES\n"); fflush(stdout); }
OP_ASSIGN_XOR { printf("!!! OP_ASSIGN_XOR\n"); fflush(stdout); }
OP_ASSIGN_XOR_XOR { printf("!!! OP_ASSIGN_XOR_XOR\n"); fflush(stdout); }
OP_AT { printf("!!! OP_AT\n"); fflush(stdout); }
OP_DIVIDED_BY { printf("!!! OP_DIVIDED_BY\n"); fflush(stdout); }
OP_DOLLAR { printf("!!! OP_DOLLAR\n"); fflush(stdout); }
OP_EQUAL { printf("!!! OP_EQUAL\n"); fflush(stdout); }
OP_GREATER_THAN { printf("!!! OP_GREATER_THAN\n"); fflush(stdout); }
OP_GREATER_THAN_EQUAL { printf("!!! OP_GREATER_THAN_EQUAL\n"); fflush(stdout); }
OP_HASH { printf("!!! OP_HASH\n"); fflush(stdout); }
OP_LESS_THAN { printf("!!! OP_LESS_THAN\n"); fflush(stdout); }
OP_LESS_THAN_EQUAL { printf("!!! OP_LESS_THAN_EQUAL\n"); fflush(stdout); }
OP_MINUS { printf("!!! OP_MINUS\n"); fflush(stdout); }
OP_MODULO { printf("!!! OP_MODULO\n"); fflush(stdout); }
OP_NOT { printf("!!! OP_NOT\n"); fflush(stdout); }
OP_NOT_EQUAL { printf("!!! OP_NOT_EQUAL\n"); fflush(stdout); }
OP_OR { printf("!!! OP_OR\n"); fflush(stdout); }
OP_OR_OR { printf("!!! OP_OR_OR\n"); fflush(stdout); }
OP_PLUS { printf("!!! OP_PLUS\n"); fflush(stdout); }
OP_POWER { printf("!!! OP_POWER\n"); fflush(stdout); }
OP_QUERY { printf("!!! OP_QUERY\n"); fflush(stdout); }
OP_TILDE { printf("!!! OP_TILDE\n"); fflush(stdout); }
OP_TILDE_EQUAL { printf("!!! OP_TILDE_EQUAL\n"); fflush(stdout); }
OP_TIMES { printf("!!! OP_TIMES\n"); fflush(stdout); }
OP_XOR { printf("!!! OP_XOR\n"); fflush(stdout); }
OP_XOR_XOR { printf("!!! OP_XOR_XOR\n"); fflush(stdout); }
PARENTHESIS_CLOSE { printf("!!! PARENTHESIS_CLOSE\n"); fflush(stdout); }
PARENTHESIS_OPEN { printf("!!! PARENTHESIS_OPEN\n"); fflush(stdout); }
SEMI_COLON { printf("!!! SEMI_COLON\n"); fflush(stdout); }
TIMES { printf("!!! TIMES\n"); fflush(stdout); }
WHITESPACE { printf("!!! WHITESPACE\n"); fflush(stdout); }

%%
