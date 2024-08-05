%{
/*
* utf8lex tokens commonly used in programming languages
*
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
// Expressions
// ---------------------------------------------------------------------
%}
EXPRESSION                      INT_EXPRESSION | FLOAT_EXPRESSION
INT_EXPRESSION                  INT WHITESPACE OP WHITESPACE INT
FLOAT_EXPRESSION                FLOAT WHITESPACE OP WHITESPACE FLOAT
OP                              OP_BITWISE | OP_ARITHMETIC | OP_COMPARISON
OP_BITWISE                      OP_AND | OP_OR | OP_XOR
OP_ARITHMETIC                   OP_DIVIDED_BY | OP_MINUS | OP_MODULO | OP_PLUS | OP_TIMES
OP_COMPARISON                   OP_EQUAL | OP_GREATER_THAN_EQUAL | OP_GREATER_THAN | OP_LESS_THAN_EQUAL | OP_LESS_THAN | OP_NOT_EQUAL

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

COMMENT_SINGLE_LINE LINE_BREAK                                  {}
ID WHITESPACE OP_ASSIGN WHITESPACE EXPRESSION LINE_BREAK {}
ID WHITESPACE OP_ASSIGN WHITESPACE STRING LINE_BREAK {}
ID WHITESPACE OP_ASSIGN WHITESPACE RANGE LINE_BREAK {}

%%
