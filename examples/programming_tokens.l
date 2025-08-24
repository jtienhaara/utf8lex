%{
/*
 * utf8lex tokens commonly used in programming languages
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

typedef enum _ENUM_yylex_token                  yylex_token_t;

enum _ENUM_yylex_token
{
  TOKEN_NONE = -1,

  TOKEN_WHITESPACE,
  TOKEN_LINE_BREAK,

  TOKEN_BACKSLASH,

  TOKEN_RANGE,
  TOKEN_INT_RANGE,
  TOKEN_FLOAT_RANGE,

  TOKEN_STRING_DOUBLE_CONTENT,
  TOKEN_STRING_DOUBLE_FULL,
  TOKEN_STRING_DOUBLE_EMPTY,
  TOKEN_STRING_DOUBLE_QUOTE,

  TOKEN_STRING_SINGLE_CONTENT,
  TOKEN_STRING_SINGLE_FULL,
  TOKEN_STRING_SINGLE_EMPTY,
  TOKEN_STRING_SINGLE_QUOTE,

  TOKEN_STRING,

  TOKEN_BRACE_CLOSE,
  TOKEN_BRACE_OPEN,
  TOKEN_BRACKET_CLOSE,
  TOKEN_BRACKET_OPEN,
  TOKEN_COLON,
  TOKEN_COMMA,
  TOKEN_COMMENT_MULTI_LINE_OPEN,
  TOKEN_COMMENT_MULTI_LINE_CLOSE,
  TOKEN_COMMENT_SINGLE_LINE,
  TOKEN_DOT_DOT_DOT,
  TOKEN_DOT_DOT,
  TOKEN_DOT,
  TOKEN_QUOTE_TRIPLE,
  TOKEN_QUOTE_DOUBLE,
  TOKEN_QUOTE_SINGLE,
  TOKEN_OP_ASSIGN_AND_AND,
  TOKEN_OP_AND_AND,
  TOKEN_OP_ASSIGN_AND,
  TOKEN_OP_AND,
  TOKEN_OP_ASSIGN,
  TOKEN_OP_ASSIGN_DECREMENT,
  TOKEN_OP_ASSIGN_DIVIDE,
  TOKEN_OP_ASSIGN_INCREMENT,
  TOKEN_OP_ASSIGN_MINUS,
  TOKEN_OP_ASSIGN_MODULO,
  TOKEN_OP_ASSIGN_OR,
  TOKEN_OP_ASSIGN_OR_OR,
  TOKEN_OP_ASSIGN_PLUS,
  TOKEN_OP_ASSIGN_POWER,
  TOKEN_OP_ASSIGN_TIMES,
  TOKEN_OP_ASSIGN_XOR,
  TOKEN_OP_ASSIGN_XOR_XOR,
  TOKEN_OP_AT,
  TOKEN_OP_DIVIDED_BY,
  TOKEN_OP_DOLLAR,
  TOKEN_OP_EQUAL,
  TOKEN_OP_GREATER_THAN_EQUAL,
  TOKEN_OP_GREATER_THAN,
  TOKEN_OP_HASH,
  TOKEN_OP_LESS_THAN_EQUAL,
  TOKEN_OP_LESS_THAN,
  TOKEN_OP_MINUS,
  TOKEN_OP_MODULO,
  TOKEN_OP_NOT,
  TOKEN_OP_NOT_EQUAL,
  TOKEN_OP_OR,
  TOKEN_OP_OR_OR,
  TOKEN_OP_PLUS,
  TOKEN_OP_POWER,
  TOKEN_OP_QUERY,
  TOKEN_OP_TILDE,
  TOKEN_OP_TILDE_EQUAL,
  TOKEN_OP_TIMES,
  TOKEN_OP_XOR,
  TOKEN_OP_XOR_XOR,
  TOKEN_PARENTHESIS_CLOSE,
  TOKEN_PARENTHESIS_OPEN,
  TOKEN_SEMI_COLON,

  TOKEN_FLOAT,
  TOKEN_INT,
  TOKEN_ID
};
%}

%{
// =====================================================================
// Whitespace
// ---------------------------------------------------------------------
%}
WHITESPACE                      HSPACE
LINE_BREAK                      VSPACE | PARAGRAPH | NEWLINE

%{
// =====================================================================
// Single character tokens
// ---------------------------------------------------------------------
%}
BACKSLASH                       "\\"

%{
// =====================================================================
// Ranges: /1..5/, /63.9..92.6/, etc.
// ---------------------------------------------------------------------
%}
RANGE                           INT_RANGE | FLOAT_RANGE
INT_RANGE                       INT DOT_DOT INT
FLOAT_RANGE                     FLOAT DOT_DOT FLOAT

%{
// =====================================================================
// Strings
// ---------------------------------------------------------------------
%}
STRING_DOUBLE_CONTENT           ((\\.)*[^"\\]*)+
STRING_DOUBLE_FULL              QUOTE_DOUBLE STRING_DOUBLE_CONTENT QUOTE_DOUBLE
STRING_DOUBLE_EMPTY             QUOTE_DOUBLE QUOTE_DOUBLE
STRING_DOUBLE_QUOTE             STRING_DOUBLE_FULL | STRING_DOUBLE_EMPTY

STRING_SINGLE_CONTENT           ((\\.)*[^'\\]*)+
STRING_SINGLE_FULL              QUOTE_SINGLE STRING_SINGLE_CONTENT QUOTE_SINGLE
STRING_SINGLE_EMPTY             QUOTE_SINGLE QUOTE_SINGLE
STRING_SINGLE_QUOTE             STRING_SINGLE_FULL | STRING_SINGLE_EMPTY

STRING                          STRING_DOUBLE_QUOTE | STRING_SINGLE_QUOTE

%{
// =====================================================================
// Fairly simple tokens.
// ---------------------------------------------------------------------
%}
BRACE_CLOSE                     "}"
BRACE_OPEN                      "{"
BRACKET_CLOSE                   "]"
BRACKET_OPEN                    "["
COLON                           ":"
COMMA                           ","
COMMENT_MULTI_LINE_OPEN         "/*"
COMMENT_MULTI_LINE_CLOSE        "*/"
COMMENT_SINGLE_LINE             //.*
DOT_DOT_DOT                     "..."
DOT_DOT                         ".."
DOT                             "."
QUOTE_TRIPLE                    "\"\"\""
QUOTE_DOUBLE                    "\""
QUOTE_SINGLE                    "'"
OP_ASSIGN_AND_AND               "&&="
OP_AND_AND                      "&&"
OP_ASSIGN_AND                   "&="
OP_AND                          "&"
OP_ASSIGN                       "="
OP_ASSIGN_DECREMENT             "--"
OP_ASSIGN_DIVIDE                "/="
OP_ASSIGN_INCREMENT             "++"
OP_ASSIGN_MINUS                 "-="
OP_ASSIGN_MODULO                "%="
OP_ASSIGN_OR                    "|="
OP_ASSIGN_OR_OR                 "||="
OP_ASSIGN_PLUS                  "+="
OP_ASSIGN_POWER                 "**="
OP_ASSIGN_TIMES                 "*="
OP_ASSIGN_XOR                   "^="
OP_ASSIGN_XOR_XOR               "^^="
OP_AT                           "@"
OP_DIVIDED_BY                   "/"
OP_DOLLAR                       "$"
OP_EQUAL                        "=="
OP_GREATER_THAN_EQUAL           ">="
OP_GREATER_THAN                 ">"
OP_HASH                         "#"
OP_LESS_THAN_EQUAL              "<="
OP_LESS_THAN                    "<"
OP_MINUS                        "-"
OP_MODULO                       "%"
OP_NOT                          "!"
OP_NOT_EQUAL                    "!="
OP_OR                           "|"
OP_OR_OR                        "||"
OP_PLUS                         "+"
OP_POWER                        "**"
OP_QUERY                        "?"
OP_TILDE                        "~"
OP_TILDE_EQUAL                  "~="
OP_TIMES                        "*"
OP_XOR                          "^"
OP_XOR_XOR                      "^^"
PARENTHESIS_CLOSE               ")"
PARENTHESIS_OPEN                "("
SEMI_COLON                      ";"

FLOAT                           [\+\-]?[1-9][0-9]*(\.[1-9][0-9]*)?(e[\+\-]?[1-9][0-9]*)?
INT                             [\+\-]?[1-9][0-9]*(e[\+]?[1-9][0-9]*)?
ID                              [_\p{L}][_\p{L}\p{N}]*

%%

%{
// =====================================================================
// RULES
// ---------------------------------------------------------------------
%}

WHITESPACE                      { return TOKEN_WHITESPACE; }
LINE_BREAK                      { return TOKEN_LINE_BREAK; }

BACKSLASH                       { return TOKEN_BACKSLASH; }

INT_RANGE                       { return TOKEN_INT_RANGE; }
FLOAT_RANGE                     { return TOKEN_FLOAT_RANGE; }

STRING_DOUBLE_QUOTE             { return TOKEN_STRING_DOUBLE_QUOTE; }
STRING_SINGLE_QUOTE             { return TOKEN_STRING_SINGLE_QUOTE; }

BRACE_CLOSE                     { return TOKEN_BRACE_CLOSE; }
BRACE_OPEN                      { return TOKEN_BRACE_OPEN; }
BRACKET_CLOSE                   { return TOKEN_BRACKET_CLOSE; }
BRACKET_OPEN                    { return TOKEN_BRACKET_OPEN; }
COLON                           { return TOKEN_COLON; }
COMMA                           { return TOKEN_COMMA; }
COMMENT_MULTI_LINE_OPEN         { return TOKEN_COMMENT_MULTI_LINE_OPEN; }
COMMENT_MULTI_LINE_CLOSE        { return TOKEN_COMMENT_MULTI_LINE_CLOSE; }
COMMENT_SINGLE_LINE             { return TOKEN_COMMENT_SINGLE_LINE; }
DOT_DOT_DOT                     { return TOKEN_DOT_DOT_DOT; }
DOT_DOT                         { return TOKEN_DOT_DOT; }
DOT                             { return TOKEN_DOT; }
QUOTE_TRIPLE                    { return TOKEN_QUOTE_TRIPLE; }
QUOTE_DOUBLE                    { return TOKEN_QUOTE_DOUBLE; }
QUOTE_SINGLE                    { return TOKEN_QUOTE_SINGLE; }
OP_ASSIGN_AND_AND               { return TOKEN_OP_ASSIGN_AND_AND; }
OP_AND_AND                      { return TOKEN_OP_AND_AND; }
OP_ASSIGN_AND                   { return TOKEN_OP_ASSIGN_AND; }
OP_AND                          { return TOKEN_OP_AND; }
OP_ASSIGN                       { return TOKEN_OP_ASSIGN; }
OP_ASSIGN_DECREMENT             { return TOKEN_OP_ASSIGN_DECREMENT; }
OP_ASSIGN_DIVIDE                { return TOKEN_OP_ASSIGN_DIVIDE; }
OP_ASSIGN_INCREMENT             { return TOKEN_OP_ASSIGN_INCREMENT; }
OP_ASSIGN_MINUS                 { return TOKEN_OP_ASSIGN_MINUS; }
OP_ASSIGN_MODULO                { return TOKEN_OP_ASSIGN_MODULO; }
OP_ASSIGN_OR                    { return TOKEN_OP_ASSIGN_OR; }
OP_ASSIGN_OR_OR                 { return TOKEN_OP_ASSIGN_OR_OR; }
OP_ASSIGN_PLUS                  { return TOKEN_OP_ASSIGN_PLUS; }
OP_ASSIGN_POWER                 { return TOKEN_OP_ASSIGN_POWER; }
OP_ASSIGN_TIMES                 { return TOKEN_OP_ASSIGN_TIMES; }
OP_ASSIGN_XOR                   { return TOKEN_OP_ASSIGN_XOR; }
OP_ASSIGN_XOR_XOR               { return TOKEN_OP_ASSIGN_XOR_XOR; }
OP_AT                           { return TOKEN_OP_AT; }
OP_DIVIDED_BY                   { return TOKEN_OP_DIVIDED_BY; }
OP_DOLLAR                       { return TOKEN_OP_DOLLAR; }
OP_EQUAL                        { return TOKEN_OP_EQUAL; }
OP_GREATER_THAN_EQUAL           { return TOKEN_OP_GREATER_THAN_EQUAL; }
OP_GREATER_THAN                 { return TOKEN_OP_GREATER_THAN; }
OP_HASH                         { return TOKEN_OP_HASH; }
OP_LESS_THAN_EQUAL              { return TOKEN_OP_LESS_THAN_EQUAL; }
OP_LESS_THAN                    { return TOKEN_OP_LESS_THAN; }
OP_MINUS                        { return TOKEN_OP_MINUS; }
OP_MODULO                       { return TOKEN_OP_MODULO; }
OP_NOT                          { return TOKEN_OP_NOT; }
OP_NOT_EQUAL                    { return TOKEN_OP_NOT_EQUAL; }
OP_OR                           { return TOKEN_OP_OR; }
OP_OR_OR                        { return TOKEN_OP_OR_OR; }
OP_PLUS                         { return TOKEN_OP_PLUS; }
OP_POWER                        { return TOKEN_OP_POWER; }
OP_QUERY                        { return TOKEN_OP_QUERY; }
OP_TILDE                        { return TOKEN_OP_TILDE; }
OP_TILDE_EQUAL                  { return TOKEN_OP_TILDE_EQUAL; }
OP_TIMES                        { return TOKEN_OP_TIMES; }
OP_XOR                          { return TOKEN_OP_XOR; }
OP_XOR_XOR                      { return TOKEN_OP_XOR_XOR; }
PARENTHESIS_CLOSE               { return TOKEN_PARENTHESIS_CLOSE; }
PARENTHESIS_OPEN                { return TOKEN_PARENTHESIS_OPEN; }
SEMI_COLON                      { return TOKEN_SEMI_COLON; }

FLOAT                           { return TOKEN_FLOAT; }
INT                             { return TOKEN_INT; }
ID                              { return TOKEN_ID; }

%%
