/*
 * utf8lex
 * Copyright 2023 Johann Tienhaara
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
#include <inttypes.h>  // For uint32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strlen(), memcpy().
#include <utf8proc.h>

// 8-bit character units for pcre2:
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "utf8lex.h"

//
// Base categories are equivalent to (but not equal to) those
// in the utf8proc library (UTF8PROC_CATEGORY_LU, etc):
//
const utf8lex_cat_t UTF8LEX_CAT_NONE = 0x00000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_NA = 0x00000001;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_UPPER = 0x00000002;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_LOWER = 0x00000004;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_TITLE = 0x00000008;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_MODIFIER = 0x00000010;
const utf8lex_cat_t UTF8LEX_CAT_LETTER_OTHER = 0x00000020;
const utf8lex_cat_t UTF8LEX_CAT_MARK_NON_SPACING = 0x00000040;
const utf8lex_cat_t UTF8LEX_CAT_MARK_SPACING_COMBINING = 0x00000080;
const utf8lex_cat_t UTF8LEX_CAT_MARK_ENCLOSING = 0x00000100;
const utf8lex_cat_t UTF8LEX_CAT_NUM_DECIMAL = 0x00000200;
const utf8lex_cat_t UTF8LEX_CAT_NUM_LETTER = 0x00000400;
const utf8lex_cat_t UTF8LEX_CAT_NUM_OTHER = 0x00000800;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_CONNECTOR = 0x00001000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_DASH = 0x00002000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_OPEN = 0x00004000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_CLOSE = 0x00008000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_QUOTE_OPEN = 0x00010000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_QUOTE_CLOSE = 0x00020000;
const utf8lex_cat_t UTF8LEX_CAT_PUNCT_OTHER = 0x00040000;
const utf8lex_cat_t UTF8LEX_CAT_SYM_MATH = 0x00080000;
const utf8lex_cat_t UTF8LEX_CAT_SYM_CURRENCY = 0x00100000;
const utf8lex_cat_t UTF8LEX_CAT_SYM_MODIFIER = 0x00200000;
const utf8lex_cat_t UTF8LEX_CAT_SYM_OTHER = 0x00400000;
const utf8lex_cat_t UTF8LEX_CAT_SEP_SPACE = 0x00800000;
const utf8lex_cat_t UTF8LEX_CAT_SEP_LINE = 0x01000000;
const utf8lex_cat_t UTF8LEX_CAT_SEP_PARAGRAPH = 0x02000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_CONTROL = 0x04000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_FORMAT = 0x08000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_SURROGATE = 0x10000000;
const utf8lex_cat_t UTF8LEX_CAT_OTHER_PRIVATE = 0x20000000;

//
// Separator rules specified by Unicode that are NOT obeyed
// by utf8proc (except when using its malloc()-based functions,
// and specifying options such as UTF8PROC_NLF2LS).
//
// From:
//     Unicode 15.1.0
//     https://www.unicode.org/reports/tr14/tr14-51.html#BK
//     https://www.unicode.org/reports/tr14/tr14-51.html#CR
//     https://www.unicode.org/reports/tr14/tr14-51.html#LF
//     https://www.unicode.org/reports/tr14/tr14-51.html#NL
//   Also for more details on history (such as VT / vertical tab), see:
//     https://www.unicode.org/standard/reports/tr13/tr13-5.html#Definitions
//
// 000A    LINE FEED (LF)
// 000B    LINE TABULATION (VT)
// 000C    FORM FEED (FF)
// 000D    CARRIAGE RETURN (CR) (unless followed by 000A LF)
// 0085    NEXT LINE (NEL)
// 2028    LINE SEPARATOR
// 2029    PARAGRAPH SEPARATOR
//
const utf8lex_cat_t UTF8LEX_EXT_SEP_LINE = 0x40000000;

const utf8lex_cat_t UTF8LEX_CAT_MAX = 0x80000000;

//
// Combined categories, OR'ed together base categories e.g. letter
// can be upper, lower or title case, etc.:
//
const utf8lex_cat_t UTF8LEX_GROUP_OTHER =
  UTF8LEX_CAT_OTHER_NA
  | UTF8LEX_CAT_OTHER_CONTROL
  | UTF8LEX_CAT_OTHER_FORMAT
  | UTF8LEX_CAT_OTHER_SURROGATE
  | UTF8LEX_CAT_OTHER_PRIVATE;
const utf8lex_cat_t UTF8LEX_GROUP_LETTER =
  UTF8LEX_CAT_LETTER_UPPER
  | UTF8LEX_CAT_LETTER_LOWER
  | UTF8LEX_CAT_LETTER_TITLE
  | UTF8LEX_CAT_LETTER_MODIFIER
  | UTF8LEX_CAT_LETTER_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_MARK =
  UTF8LEX_CAT_MARK_NON_SPACING
  | UTF8LEX_CAT_MARK_SPACING_COMBINING
  | UTF8LEX_CAT_MARK_ENCLOSING;
const utf8lex_cat_t UTF8LEX_GROUP_NUM =
  UTF8LEX_CAT_NUM_DECIMAL
  | UTF8LEX_CAT_NUM_LETTER
  | UTF8LEX_CAT_NUM_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_PUNCT =
  UTF8LEX_CAT_PUNCT_CONNECTOR
  | UTF8LEX_CAT_PUNCT_DASH
  | UTF8LEX_CAT_PUNCT_OPEN
  | UTF8LEX_CAT_PUNCT_CLOSE
  | UTF8LEX_CAT_PUNCT_QUOTE_OPEN
  | UTF8LEX_CAT_PUNCT_QUOTE_CLOSE
  | UTF8LEX_CAT_PUNCT_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_SYM =
  UTF8LEX_CAT_SYM_MATH
  | UTF8LEX_CAT_SYM_CURRENCY
  | UTF8LEX_CAT_SYM_MODIFIER
  | UTF8LEX_CAT_SYM_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_WHITESPACE =
  UTF8LEX_CAT_SEP_SPACE
  | UTF8LEX_CAT_SEP_LINE
  | UTF8LEX_CAT_SEP_PARAGRAPH
  | UTF8LEX_EXT_SEP_LINE;


// Formats the specified OR'ed category/ies as a string,
// overwriting the specified str_pointer.
utf8lex_error_t utf8lex_format_cat(
        utf8lex_cat_t cat,
        unsigned char *str_pointer
        )
{
  if (cat <= UTF8LEX_CAT_NONE
      || cat >= UTF8LEX_CAT_MAX)
  {
    return UTF8LEX_ERROR_CAT;
  }
  else if (str_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Curently can format up to 386 bytes.

  char *or = " | ";
  char *first = "";
  char *maybe_or = first;

  utf8lex_cat_t remaining_cat = cat;
  size_t total_bytes_written = (size_t) 0;
  size_t remaining_num_bytes = UTF8LEX_CAT_FORMAT_MAX_LENGTH;

  //
  // Groups first.
  //
  // Combined categories, OR'ed together base categories e.g. letter
  // can be upper, lower or title case, etc.:
  //
  if ((remaining_cat & UTF8LEX_GROUP_OTHER) == UTF8LEX_GROUP_OTHER)
  {
    // 5-8 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sOTHER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_GROUP_OTHER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_GROUP_LETTER) == UTF8LEX_GROUP_LETTER)
  {
    // 6-9 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sLETTER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_GROUP_LETTER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_GROUP_MARK) == UTF8LEX_GROUP_MARK)
  {
    // 4-7 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sMARK",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_GROUP_MARK);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_GROUP_NUM) == UTF8LEX_GROUP_NUM)
  {
    // 3-6 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sNUM",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_GROUP_NUM);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_GROUP_PUNCT) == UTF8LEX_GROUP_PUNCT)
  {
    // 5-8 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sPUNCT",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_GROUP_PUNCT);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_GROUP_SYM) == UTF8LEX_GROUP_SYM)
  {
    // 3-6 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sSYM",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_GROUP_SYM);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_GROUP_WHITESPACE) == UTF8LEX_GROUP_WHITESPACE)
  {
    // 10-13 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sWHITESPACE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_GROUP_WHITESPACE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }

  if ((remaining_cat & UTF8LEX_CAT_OTHER_NA) == UTF8LEX_CAT_OTHER_NA)
  {
    // 2-5 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sNA",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_OTHER_NA);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_LETTER_UPPER) == UTF8LEX_CAT_LETTER_UPPER)
  {
    // 5-8 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sUPPER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_LETTER_UPPER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_LETTER_LOWER) == UTF8LEX_CAT_LETTER_LOWER)
  {
    // 5-8 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sLOWER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_LETTER_LOWER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_LETTER_TITLE) == UTF8LEX_CAT_LETTER_TITLE)
  {
    // 5-8 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sTITLE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_LETTER_TITLE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_LETTER_MODIFIER) == UTF8LEX_CAT_LETTER_MODIFIER)
  {
    // 8-11 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sMODIFIER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_LETTER_MODIFIER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_LETTER_OTHER) == UTF8LEX_CAT_LETTER_OTHER)
  {
    // 12-15 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sLETTER_OTHER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_LETTER_OTHER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_MARK_NON_SPACING) == UTF8LEX_CAT_MARK_NON_SPACING)
  {
    // 7-10 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sMARK_NS",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_MARK_NON_SPACING);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_MARK_SPACING_COMBINING) == UTF8LEX_CAT_MARK_SPACING_COMBINING)
  {
    // 7-10 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sMARK_SC",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_MARK_SPACING_COMBINING);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_MARK_ENCLOSING) == UTF8LEX_CAT_MARK_ENCLOSING)
  {
    // 6-9 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sMARK_E",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_MARK_ENCLOSING);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_NUM_DECIMAL) == UTF8LEX_CAT_NUM_DECIMAL)
  {
    // 7-10 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sDECIMAL",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_NUM_DECIMAL);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_NUM_LETTER) == UTF8LEX_CAT_NUM_LETTER)
  {
    // 10-13 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sNUM_LETTER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_NUM_LETTER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_NUM_OTHER) == UTF8LEX_CAT_NUM_OTHER)
  {
    // 9-12 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sNUM_OTHER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_NUM_OTHER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_PUNCT_CONNECTOR) == UTF8LEX_CAT_PUNCT_CONNECTOR)
  {
    // 9-12 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sCONNECTOR",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_PUNCT_CONNECTOR);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_PUNCT_DASH) == UTF8LEX_CAT_PUNCT_DASH)
  {
    // 4-7 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sDASH",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_PUNCT_DASH);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_PUNCT_OPEN) == UTF8LEX_CAT_PUNCT_OPEN)
  {
    // 10-13 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sPUNCT_OPEN",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_PUNCT_OPEN);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_PUNCT_CLOSE) == UTF8LEX_CAT_PUNCT_CLOSE)
  {
    // 11-14 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sPUNCT_CLOSE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_PUNCT_CLOSE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_PUNCT_QUOTE_OPEN) == UTF8LEX_CAT_PUNCT_QUOTE_OPEN)
  {
    // 10-13 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sQUOTE_OPEN",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_PUNCT_QUOTE_OPEN);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_PUNCT_QUOTE_CLOSE) == UTF8LEX_CAT_PUNCT_QUOTE_CLOSE)
  {
    // 11-14 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sQUOTE_CLOSE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_PUNCT_QUOTE_CLOSE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_PUNCT_OTHER) == UTF8LEX_CAT_PUNCT_OTHER)
  {
    // 11-14 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sPUNCT_OTHER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_PUNCT_OTHER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_SYM_MATH) == UTF8LEX_CAT_SYM_MATH)
  {
    // 4-7 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sMATH",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_SYM_MATH);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_SYM_CURRENCY) == UTF8LEX_CAT_SYM_CURRENCY)
  {
    // 8-11 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sCURRENCY",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_SYM_CURRENCY);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_SYM_MODIFIER) == UTF8LEX_CAT_SYM_MODIFIER)
  {
    // 12-15 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sSYM_MODIFIER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_SYM_MODIFIER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_SYM_OTHER) == UTF8LEX_CAT_SYM_OTHER)
  {
    // 9-12 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sSYM_OTHER",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_SYM_OTHER);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_SEP_SPACE) == UTF8LEX_CAT_SEP_SPACE)
  {
    // 6-9 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sHSPACE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_SEP_SPACE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_SEP_LINE) == UTF8LEX_CAT_SEP_LINE)
  {
    // 6-9 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sVSPACE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_SEP_LINE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_SEP_PARAGRAPH) == UTF8LEX_CAT_SEP_PARAGRAPH)
  {
    // 9-12 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sPARAGRAPH",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_SEP_PARAGRAPH);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_OTHER_CONTROL) == UTF8LEX_CAT_OTHER_CONTROL)
  {
    // 7-10 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sCONTROL",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_OTHER_CONTROL);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_OTHER_FORMAT) == UTF8LEX_CAT_OTHER_FORMAT)
  {
    // 6-9 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sFORMAT",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_OTHER_FORMAT);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_OTHER_SURROGATE) == UTF8LEX_CAT_OTHER_SURROGATE)
  {
    // 9-12 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sSURROGATE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_OTHER_SURROGATE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_CAT_OTHER_PRIVATE) == UTF8LEX_CAT_OTHER_PRIVATE)
  {
    // 7-10 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sPRIVATE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_CAT_OTHER_PRIVATE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_EXT_SEP_LINE) == UTF8LEX_EXT_SEP_LINE)
  {
    // 7-10 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sNEWLINE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_EXT_SEP_LINE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }

  // Sanity checks:
  if (remaining_cat != UTF8LEX_CAT_NONE)
  {
    fprintf(stderr, "*** utf8lex bug: utf8lex_format_cat() remaining = %d\n",
            (int) remaining_cat);
    return UTF8LEX_ERROR_CAT;
  }
  else if (remaining_num_bytes == (size_t) 0)
  {
    fprintf(stderr, "*** utf8lex bug: utf8lex_format_cat() too many bytes\n");
    return UTF8LEX_ERROR_CAT;
  }

  return UTF8LEX_OK;
}

// Parses the specified string into OR'ed category/ies,
// overwriting the specified cat_pointer.
utf8lex_error_t utf8lex_parse_cat(
        utf8lex_cat_t *cat_pointer,
        unsigned char *str
        )
{
  if (cat_pointer == NULL
      || str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
  int length_bytes = strlen(str);

  bool is_category_expected = true;
  for (off_t c = 0; c < length_bytes;)
  {
    if (str[c] == ' ')
    {
      c ++;
      continue;
    }

    unsigned char *ptr = (unsigned char *) (str + c);
    if (strncmp("CONNECTOR", ptr, (size_t) 9) == 0)
    {
      cat |= UTF8LEX_CAT_PUNCT_CONNECTOR;
      c += (off_t) 9;
    }
    else if (strncmp("CONTROL", ptr, (size_t) 7) == 0)
    {
      cat |= UTF8LEX_CAT_OTHER_CONTROL;
      c += (off_t) 7;
    }
    else if (strncmp("CURRENCY", ptr, (size_t) 8) == 0)
    {
      cat |= UTF8LEX_CAT_SYM_CURRENCY;
      c += (off_t) 8;
    }
    else if (strncmp("DASH", ptr, (size_t) 4) == 0)
    {
      cat |= UTF8LEX_CAT_PUNCT_DASH;
      c += (off_t) 4;
    }
    else if (strncmp("DECIMAL", ptr, (size_t) 7) == 0)
    {
      cat |= UTF8LEX_CAT_NUM_DECIMAL;
      c += (off_t) 7;
    }
    else if (strncmp("FORMAT", ptr, (size_t) 6) == 0)
    {
      cat |= UTF8LEX_CAT_OTHER_FORMAT;
      c += (off_t) 6;
    }
    else if (strncmp("HSPACE", ptr, (size_t) 6) == 0)
    {
      cat |= UTF8LEX_CAT_SEP_SPACE;
      c += (off_t) 6;
    }
    else if (strncmp("LETTER_OTHER", ptr, (size_t) 12) == 0)
    {
      cat |= UTF8LEX_CAT_LETTER_OTHER;
      c += (off_t) 12;
    }
    else if (strncmp("LETTER", ptr, (size_t) 6) == 0)
    {
      cat |= UTF8LEX_GROUP_LETTER;
      c += (off_t) 6;
    }
    else if (strncmp("LOWER", ptr, (size_t) 5) == 0)
    {
      cat |= UTF8LEX_CAT_LETTER_LOWER;
      c += (off_t) 5;
    }
    else if (strncmp("MARK_E", ptr, (size_t) 6) == 0)
    {
      cat |= UTF8LEX_CAT_MARK_ENCLOSING;
      c += (off_t) 6;
    }
    else if (strncmp("MARK_NS", ptr, (size_t) 7) == 0)
    {
      cat |= UTF8LEX_CAT_MARK_NON_SPACING;
      c += (off_t) 7;
    }
    else if (strncmp("MARK_SC", ptr, (size_t) 7) == 0)
    {
      cat |= UTF8LEX_CAT_MARK_SPACING_COMBINING;
      c += (off_t) 7;
    }
    else if (strncmp("MARK", ptr, (size_t) 4) == 0)
    {
      cat |= UTF8LEX_GROUP_MARK;
      c += (off_t) 4;
    }
    else if (strncmp("MATH", ptr, (size_t) 4) == 0)
    {
      cat |= UTF8LEX_CAT_SYM_MATH;
      c += (off_t) 4;
    }
    else if (strncmp("MODIFIER", ptr, (size_t) 8) == 0)
    {
      cat |= UTF8LEX_CAT_LETTER_MODIFIER;
      c += (off_t) 8;
    }
    else if (strncmp("NA", ptr, (size_t) 2) == 0)
    {
      cat |= UTF8LEX_CAT_OTHER_NA;
      c += (off_t) 2;
    }
    else if (strncmp("NEWLINE", ptr, (size_t) 7) == 0)
    {
      cat |= UTF8LEX_EXT_SEP_LINE;
      c += (off_t) 7;
    }
    else if (strncmp("NUM_LETTER", ptr, (size_t) 10) == 0)
    {
      cat |= UTF8LEX_CAT_NUM_LETTER;
      c += (off_t) 10;
    }
    else if (strncmp("NUM_OTHER", ptr, (size_t) 9) == 0)
    {
      cat |= UTF8LEX_CAT_NUM_OTHER;
      c += (off_t) 9;
    }
    else if (strncmp("NUM", ptr, (size_t) 3) == 0)
    {
      cat |= UTF8LEX_GROUP_NUM;
      c += (off_t) 3;
    }
    else if (strncmp("OTHER", ptr, (size_t) 5) == 0)
    {
      cat |= UTF8LEX_GROUP_OTHER;
      c += (off_t) 5;
    }
    else if (strncmp("PARAGRAPH", ptr, (size_t) 9) == 0)
    {
      cat |= UTF8LEX_CAT_SEP_PARAGRAPH;
      c += (off_t) 9;
    }
    else if (strncmp("PRIVATE", ptr, (size_t) 7) == 0)
    {
      cat |= UTF8LEX_CAT_OTHER_PRIVATE;
      c += (off_t) 7;
    }
    else if (strncmp("PUNCT_CLOSE", ptr, (size_t) 11) == 0)
    {
      cat |= UTF8LEX_CAT_PUNCT_CLOSE;
      c += (off_t) 11;
    }
    else if (strncmp("PUNCT_OPEN", ptr, (size_t) 10) == 0)
    {
      cat |= UTF8LEX_CAT_PUNCT_OPEN;
      c += (off_t) 10;
    }
    else if (strncmp("PUNCT_OTHER", ptr, (size_t) 11) == 0)
    {
      cat |= UTF8LEX_CAT_PUNCT_OTHER;
      c += (off_t) 11;
    }
    else if (strncmp("PUNCT", ptr, (size_t) 5) == 0)
    {
      cat |= UTF8LEX_GROUP_PUNCT;
      c += (off_t) 5;
    }
    else if (strncmp("QUOTE_CLOSE", ptr, (size_t) 11) == 0)
    {
      cat |= UTF8LEX_CAT_PUNCT_QUOTE_CLOSE;
      c += (off_t) 11;
    }
    else if (strncmp("QUOTE_OPEN", ptr, (size_t) 10) == 0)
    {
      cat |= UTF8LEX_CAT_PUNCT_QUOTE_OPEN;
      c += (off_t) 10;
    }
    else if (strncmp("SURROGATE", ptr, (size_t) 9) == 0)
    {
      cat |= UTF8LEX_CAT_OTHER_SURROGATE;
      c += (off_t) 9;
    }
    else if (strncmp("SYM_MODIFIER", ptr, (size_t) 12) == 0)
    {
      cat |= UTF8LEX_CAT_SYM_MODIFIER;
      c += (off_t) 12;
    }
    else if (strncmp("SYM_OTHER", ptr, (size_t) 9) == 0)
    {
      cat |= UTF8LEX_CAT_SYM_OTHER;
      c += (off_t) 9;
    }
    else if (strncmp("SYM", ptr, (size_t) 3) == 0)
    {
      cat |= UTF8LEX_GROUP_SYM;
      c += (off_t) 3;
    }
    else if (strncmp("TITLE", ptr, (size_t) 5) == 0)
    {
      cat |= UTF8LEX_CAT_LETTER_TITLE;
      c += (off_t) 5;
    }
    else if (strncmp("UPPER", ptr, (size_t) 5) == 0)
    {
      cat |= UTF8LEX_CAT_LETTER_UPPER;
      c += (off_t) 5;
    }
    else if (strncmp("VSPACE", ptr, (size_t) 6) == 0)
    {
      cat |= UTF8LEX_CAT_SEP_LINE;
      c += (off_t) 6;
    }
    else if (strncmp("WHITESPACE", ptr, (size_t) 10) == 0)
    {
      cat |= UTF8LEX_GROUP_WHITESPACE;
      c += (off_t) 10;
    }
    else
    {
      fprintf(stderr,
              "*** utf8lex_parse_cat(): unknown category starting at %d: \"%s\"\n",
              (int) c,
              ptr);
      return UTF8LEX_ERROR_CAT;
    }

    bool is_or_found = false;
    for (off_t o = c; o < length_bytes; o ++)
    {
      if (str[o] == '|')
      {
        c = o + 1;
        is_or_found = true;
        break;
      }
      else if (str[o] != ' ')
      {
        ptr = (unsigned char *) (str + o);
        fprintf(stderr,
                "*** utf8lex_parse_cat(): invalid text starting at %d: \"%s\"\n",
                (int) o,
                ptr);
        return UTF8LEX_ERROR_CAT;
      }
    }

    if (! is_or_found)
    {
      // End of the string, we're done.
      is_category_expected = false;
      break;
    }

    // Loop, continue looking for more categories.
  }

  if (is_category_expected)
  {
    if (cat == UTF8LEX_CAT_NONE)
    {
      fprintf(stderr,
              "*** utf8lex_parse_cat(): empty categories: \"%s\"\n",
              str);
    }
    else
    {
      fprintf(stderr,
              "*** utf8lex_parse_cat(): dangling '|': \"%s\"\n",
              str);
    }

    return UTF8LEX_ERROR_CAT;
  }

  *cat_pointer = cat;

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                           utf8lex_string_t
// ---------------------------------------------------------------------
utf8lex_error_t utf8lex_string_init(
        utf8lex_string_t *self,
        size_t max_length_bytes,  // How many bytes have been allocated.
        size_t length_bytes,  // How many bytes have been written.
        unsigned char *bytes
        )
{
  if (self == NULL
      || bytes == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (length_bytes < (size_t) 0
           || max_length_bytes < length_bytes)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  self->max_length_bytes = max_length_bytes;
  self->length_bytes = length_bytes;
  self->bytes = bytes;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_string_clear(
        utf8lex_string_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->max_length_bytes = (size_t) 0;
  self->length_bytes = (size_t) 0;
  self->bytes = NULL;

  return UTF8LEX_OK;
}


// Print state (location) to the specified string:
utf8lex_error_t utf8lex_state_string(
        utf8lex_string_t *str,
        utf8lex_state_t *state
        )
{
  if (str == NULL
      || str->bytes == NULL
      || state == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  size_t num_bytes_written = snprintf(
      str->bytes,
      str->max_length_bytes,
      "(bytes@%d[%d], chars@%d[%d], graphemes@%d[%d], lines@%d[%d])",
      state->loc[UTF8LEX_UNIT_BYTE].start,
      state->loc[UTF8LEX_UNIT_BYTE].length,
      state->loc[UTF8LEX_UNIT_CHAR].start,
      state->loc[UTF8LEX_UNIT_CHAR].length,
      state->loc[UTF8LEX_UNIT_GRAPHEME].start,
      state->loc[UTF8LEX_UNIT_GRAPHEME].length,
      state->loc[UTF8LEX_UNIT_LINE].start,
      state->loc[UTF8LEX_UNIT_LINE].length);

  if (num_bytes_written >= str->max_length_bytes)
  {
    // The error string was truncated.
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  return UTF8LEX_OK;
}

// Print error message to the specified string:
utf8lex_error_t utf8lex_error_string(
        utf8lex_string_t *str,
        utf8lex_error_t error
        )
{
  if (str == NULL
      || str->bytes == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (error < UTF8LEX_OK
           || error > UTF8LEX_ERROR_MAX)
  {
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
  case UTF8LEX_ERROR_CHAIN_INSERT:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_CHAIN_INSERT");
    break;
  case UTF8LEX_ERROR_CAT:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_CAT");
    break;
  case UTF8LEX_ERROR_PATTERN_TYPE:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_PATTERN_TYPE");
    break;
  case UTF8LEX_ERROR_EMPTY_LITERAL:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_EMPTY_LITERAL");
    break;
  case UTF8LEX_ERROR_REGEX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_REGEX");
    break;
  case UTF8LEX_ERROR_UNIT:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_UNIT");
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
  case UTF8LEX_ERROR_BAD_MIN:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_MIN");
    break;
  case UTF8LEX_ERROR_BAD_MAX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_BAD_MAX");
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

  case UTF8LEX_ERROR_MAX:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "UTF8LEX_ERROR_MAX");
    break;

  default:
    num_bytes_written = snprintf(str->bytes, str->max_length_bytes,
                                 "unknown error %d", (int) error);
    return UTF8LEX_ERROR_BAD_ERROR;
  }

  if (num_bytes_written >= str->max_length_bytes)
  {
    // The error string was truncated.
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                       utf8lex_cat_pattern_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_cat_pattern_init(
        utf8lex_cat_pattern_t *self,
        utf8lex_cat_t cat,  // The category, such as UTF8LEX_GROUP_LETTER.
        int min,  // Minimum consecutive occurrences of the cat (1 or more).
        int max  // Maximum consecutive occurrences (-1 = no limit).
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (cat <= UTF8LEX_CAT_NONE
           || cat >= UTF8LEX_CAT_MAX)
  {
    return UTF8LEX_ERROR_CAT;
  }
  else if (min <= 0)
  {
    return UTF8LEX_ERROR_BAD_MIN;
  }
  else if (max != -1
           && max < min)
  {
    return UTF8LEX_ERROR_BAD_MAX;
  }

  self->pattern_type = UTF8LEX_PATTERN_TYPE_CAT;
  self->cat = cat;
  utf8lex_format_cat(self->cat, self->str);
  self->min = min;
  self->max = max;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_cat_pattern_clear(
        utf8lex_abstract_pattern_t *self  // Must be utf8lex_cat_pattern_t *
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_cat_pattern_t *cat_pattern =
    (utf8lex_cat_pattern_t *) self;

  cat_pattern->pattern_type = NULL;
  cat_pattern->cat = UTF8LEX_CAT_NONE;
  cat_pattern->str[0] = 0;
  cat_pattern->min = 0;
  cat_pattern->max = 0;

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                      utf8lex_literal_pattern_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_literal_pattern_init(
        utf8lex_literal_pattern_t *self,
        unsigned char *str
        )
{
  if (self == NULL
      || str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  size_t num_bytes = strlen(str);
  if (num_bytes == (size_t) 0)
  {
    return UTF8LEX_ERROR_EMPTY_LITERAL;
  }

  self->pattern_type = UTF8LEX_PATTERN_TYPE_LITERAL;
  self->str = str;
  self->length[UTF8LEX_UNIT_BYTE] = (int) num_bytes;

  // We know how many bytes the ltieral is.
  // Now we'll now use utf8proc to count how many characters,
  // graphemes, lines, etc. are in the matching region.

  utf8lex_string_t utf8lex_str;
  utf8lex_string_init(&utf8lex_str,  // self
                      num_bytes,     // max_length_bytes
                      num_bytes,     // length_bytes
                      self->str);    // bytes
  utf8lex_buffer_t buffer;
  utf8lex_buffer_init(&buffer,       // self
                      NULL,          // prev
                      &utf8lex_str,  // str
                      true);         // is_eof
  utf8lex_state_t state;
  utf8lex_state_init(&state,         // self
                     &buffer);       // buffer
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state.loc[unit].start = 0;
    state.loc[unit].length = 0;
  }

  off_t offset = (off_t) 0;
  size_t length[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    length[unit] = (size_t) 0;
  }

  // Keep reading graphemes until we've reached the end of the literal string:
  for (int ug = 0;
       length[UTF8LEX_UNIT_BYTE] < self->length[UTF8LEX_UNIT_BYTE];
       ug ++)
  {
    // Read in one UTF-8 grapheme cluster per loop iteration:
    off_t grapheme_offset = offset;
    size_t grapheme_length[UTF8LEX_UNIT_MAX];  // Uninitialized is fine.
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        &state,  // state, including absolute locations.
        &grapheme_offset,  // start byte, relative to start of buffer string.
        grapheme_length,  // size_t[] # bytes, chars, etc read.
        &codepoint,  // codepoint
        &cat  //cat
        );

    if (error != UTF8LEX_OK)
    {
      // The literal is a string that utf8proc either needs MORE bytes for,
      // or rejected outright.  We can't accept it.
      return error;
    }

    // We found another grapheme inside the literal string.
    // Keep looking for more graphemes inside the literal string.
    offset = grapheme_offset;
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      length[unit] += grapheme_length[unit];
    }
  }

  // Check to make sure strlen and utf8proc agree on # bytes.  (They should.)
  if (length[UTF8LEX_UNIT_BYTE] != self->length[UTF8LEX_UNIT_BYTE])
  {
    fprintf(stderr,
            "*** strlen and utf8proc disagree: strlen = %d vs utf8proc = %d\n",
            self->length[UTF8LEX_UNIT_BYTE],
            length[UTF8LEX_UNIT_BYTE]);
  }

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->length[unit] = (int) length[unit];
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_literal_pattern_clear(
        utf8lex_abstract_pattern_t *self  // Must be utf8lex_literal_pattern_t *
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_literal_pattern_t *literal_pattern =
    (utf8lex_literal_pattern_t *) self;

  literal_pattern->pattern_type = NULL;
  literal_pattern->str = NULL;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    literal_pattern->length[unit] = -1;
  }

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                       utf8lex_regex_pattern_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_regex_pattern_init(
        utf8lex_regex_pattern_t *self,
        unsigned char *pattern
        )
{
  if (self == NULL
      || pattern == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Compile the regex_pattern string into a pcre2code
  // regular expression:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#SEC20
  int pcre2_error;  // We don't actually use this for now.
  PCRE2_SIZE pcre2_offset;  // We don't actually use this for now.
  self->regex = pcre2_compile(
                              pattern,  // the regular expression pattern
                              PCRE2_ZERO_TERMINATED,  // '\0' at end.
                              0,  // default options.
                              &pcre2_error,  // for error code.
                              &pcre2_offset,  // for error offset.
                              NULL);  // no compile context.
  if (pcre2_error < 0)
  {
    PCRE2_UCHAR pcre2_error_string[256];
    pcre2_get_error_message(pcre2_error,
                            pcre2_error_string,
                            (PCRE2_SIZE) 256);
    fprintf(stderr,
            "*** pcre2 error: %s\n", pcre2_error_string);
    utf8lex_regex_pattern_clear((utf8lex_abstract_pattern_t *) self);

    return UTF8LEX_ERROR_BAD_REGEX;
  }

  self->pattern_type = UTF8LEX_PATTERN_TYPE_REGEX;
  self->pattern = pattern;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_regex_pattern_clear(
        utf8lex_abstract_pattern_t *self  // Must be utf8lex_regex_pattern_t *
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_regex_pattern_t *regex_pattern =
    (utf8lex_regex_pattern_t *) self;

  if (regex_pattern->regex != NULL)
  {
    pcre2_code_free(regex_pattern->regex);
  }

  regex_pattern->pattern_type = NULL;
  regex_pattern->pattern = NULL;
  regex_pattern->regex = NULL;

  return UTF8LEX_OK;
}


// ---------------------------------------------------------------------
//                        utf8lex_token_type_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_token_type_init(
        utf8lex_token_type_t *self,
        utf8lex_token_type_t *prev,
        unsigned char *name,
        utf8lex_abstract_pattern_t *pattern,
        unsigned char *code
        )
{
  if (self == NULL
      || name == NULL
      || pattern == NULL
      || pattern->pattern_type == NULL
      || pattern->pattern_type->lex == NULL
      || code == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (prev != NULL
           && prev->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  self->next = NULL;
  self->prev = prev;
  // id is set below, in the if/else statements.
  self->name = name;
  self->pattern = pattern;
  self->code = code;
  if (self->prev != NULL)
  {
    self->id = self->prev->id + 1;
    self->prev->next = self;
  }
  else
  {
    self->id = (uint32_t) 0;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_token_type_clear(
        utf8lex_token_type_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self->next;
  }

  if (self->next != NULL)
  {
    self->next->prev = self->prev;
  }

  if (self->pattern != NULL
      && self->pattern->pattern_type != NULL
      && self->pattern->pattern_type->clear != NULL)
  {
    self->pattern->pattern_type->clear(self->pattern);
  }

  self->next = NULL;
  self->prev = NULL;
  self->id = (uint32_t) 0;
  self->name = NULL;
  self->pattern = NULL;
  self->code = NULL;

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                          utf8lex_location_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_location_init(
        utf8lex_location_t *self,
        int start,
        int length
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (start < 0)
  {
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length < 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  self->start = start;
  self->length = length;

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_location_clear(
        utf8lex_location_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->start = -1;
  self->length = -1;

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                           utf8lex_token_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_token_init(
        utf8lex_token_t *self,
        utf8lex_token_type_t *token_type,
        utf8lex_state_t *state  // For buffer and absolute location.
        )
{
  if (self == NULL
      || token_type == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Check valid buffer-relative starts, lengths:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (state->buffer->loc[unit].start < 0)
    {
      return UTF8LEX_ERROR_BAD_START;
    }
    else if (state->buffer->loc[unit].length < 0)
    {
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
  }

  // Check valid bytes range within buffer:
  int start_byte = state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  int length_bytes = state->buffer->loc[UTF8LEX_UNIT_BYTE].length;
  if ((size_t) start_byte >= state->buffer->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length_bytes < 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  else if ((start_byte + length_bytes) > state->buffer->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  // Check valid absolute starts, lengths:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    if (state->loc[unit].start < 0)
    {
      return UTF8LEX_ERROR_BAD_START;
    }
    else if (state->loc[unit].length < 0)
    {
      return UTF8LEX_ERROR_BAD_LENGTH;
    }
  }

  self->token_type = token_type;
  self->start_byte = start_byte;  // Bytes offset into str where token starts.
  self->length_bytes = length_bytes;  // # bytes in token.
  self->str = state->buffer->str;  // The buffer's string.
  // Absolute locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    // Absolute location from the absolute state:
    self->loc[unit].start = state->loc[unit].start;
    // Absolute length from the length of the token within the buffer:
    self->loc[unit].length = state->buffer->loc[unit].length;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_token_clear(
        utf8lex_token_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->token_type = NULL;
  self->start_byte = -1;
  self->length_bytes = -1;
  self->str = NULL;

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  return UTF8LEX_OK;
}


// Returns UTF8LEX_MORE if the destination string truncates the token:
extern utf8lex_error_t utf8lex_token_copy_string(
        utf8lex_token_t *self,
        unsigned char *str,
        size_t max_bytes)
{
  if (self == NULL
      || str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  int start_byte = self->start_byte;
  int length_bytes = self->length_bytes;
  if (start_byte < 0)
  {
    return UTF8LEX_ERROR_BAD_START;
  }
  else if (length_bytes < 0)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }
  else if ((size_t) (start_byte + length_bytes) > self->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_LENGTH;
  }

  size_t num_bytes = length_bytes;
  if ((num_bytes + 1) > max_bytes)
  {
    num_bytes = max_bytes - 1;
  }

  void *source = &self->str->bytes[start_byte];
  memcpy(str, source, num_bytes);
  str[num_bytes] = 0;

  if (num_bytes != length_bytes)
  {
    return UTF8LEX_MORE;
  }

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                           utf8lex_buffer_t
// ---------------------------------------------------------------------

const uint32_t UTF8LEX_BUFFER_STRINGS_MAX = 16384;

utf8lex_error_t utf8lex_buffer_init(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *prev,
        utf8lex_string_t *str,
        bool is_eof
        )
{
  if (self == NULL
      || str == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->next = NULL;
  self->prev = prev;
  self->str = str;
  self->is_eof = is_eof;

  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = 0;
    self->loc[unit].length = 0;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_buffer_clear(
        utf8lex_buffer_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (self->prev != NULL)
  {
    self->prev->next = self->next;
  }

  if (self->next != NULL)
  {
    self->next->prev = self->prev;
  }

  self->prev = NULL;
  self->next = NULL;
  self->str = NULL;


  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  return UTF8LEX_OK;
}


utf8lex_error_t utf8lex_buffer_add(
        utf8lex_buffer_t *self,
        utf8lex_buffer_t *tail
        )
{
  if (self == NULL
      || tail == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (tail->prev != NULL
           || tail->next != NULL)
  {
    return UTF8LEX_ERROR_CHAIN_INSERT;
  }

  // Find the end of the buffer, in case this (self) buffer is not at the end:
  utf8lex_buffer_t *buffer = self;
  int infinite_loop = (int) UTF8LEX_BUFFER_STRINGS_MAX;
  for (int b = 0; b < infinite_loop; b ++)
  {
    if (buffer->next == NULL)
    {
      buffer->next = tail;
      tail->prev = buffer;
      break;
    }

    buffer = buffer->next;
  }

  if (tail->prev != buffer)
  {
    return UTF8LEX_ERROR_INFINITE_LOOP;
  }

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                            ut8lex_state_t
// ---------------------------------------------------------------------

utf8lex_error_t utf8lex_state_init(
        utf8lex_state_t *self,
        utf8lex_buffer_t *buffer
        )
{
  if (self == NULL
      || buffer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->buffer = buffer;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  return UTF8LEX_OK;
}

utf8lex_error_t utf8lex_state_clear(
        utf8lex_state_t *self
        )
{
  if (self == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  self->buffer = NULL;
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    self->loc[unit].start = -1;
    self->loc[unit].length = -1;
  }

  return UTF8LEX_OK;
}

// ---------------------------------------------------------------------
//                            utf8lex_lex()
// ---------------------------------------------------------------------

static utf8lex_error_t utf8lex_cat_from_utf8proc_category(
        utf8proc_category_t utf8proc_cat,
        utf8lex_cat_t *cat_pointer
        )
{
  if (cat_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  utf8lex_error_t error = UTF8LEX_NO_MATCH;
  switch (utf8proc_cat)
  {
  case UTF8PROC_CATEGORY_CN:
    *cat_pointer = UTF8LEX_CAT_OTHER_NA;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LU:
    *cat_pointer = UTF8LEX_CAT_LETTER_UPPER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LL:
    *cat_pointer = UTF8LEX_CAT_LETTER_LOWER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LT:
    *cat_pointer = UTF8LEX_CAT_LETTER_TITLE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LM:
    *cat_pointer = UTF8LEX_CAT_LETTER_MODIFIER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_LO:
    *cat_pointer = UTF8LEX_CAT_LETTER_OTHER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_MN:
    *cat_pointer = UTF8LEX_CAT_MARK_NON_SPACING;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_MC:
    *cat_pointer = UTF8LEX_CAT_MARK_SPACING_COMBINING;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ME:
    *cat_pointer = UTF8LEX_CAT_MARK_ENCLOSING;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ND:
    *cat_pointer = UTF8LEX_CAT_NUM_DECIMAL;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_NL:
    *cat_pointer = UTF8LEX_CAT_NUM_LETTER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_NO:
    *cat_pointer = UTF8LEX_CAT_NUM_OTHER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PC:
    *cat_pointer = UTF8LEX_CAT_PUNCT_CONNECTOR;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PD:
    *cat_pointer = UTF8LEX_CAT_PUNCT_DASH;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PS:
    *cat_pointer = UTF8LEX_CAT_PUNCT_OPEN;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PE:
    *cat_pointer = UTF8LEX_CAT_PUNCT_CLOSE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PI:
    *cat_pointer = UTF8LEX_CAT_PUNCT_QUOTE_OPEN;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PF:
    *cat_pointer = UTF8LEX_CAT_PUNCT_QUOTE_CLOSE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_PO:
    *cat_pointer = UTF8LEX_CAT_PUNCT_OTHER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_SM:
    *cat_pointer = UTF8LEX_CAT_SYM_MATH;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_SC:
    *cat_pointer = UTF8LEX_CAT_SYM_CURRENCY;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_SK:
    *cat_pointer = UTF8LEX_CAT_SYM_MODIFIER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_SO:
    *cat_pointer = UTF8LEX_CAT_SYM_OTHER;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ZS:
    *cat_pointer = UTF8LEX_CAT_SEP_SPACE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ZL:
    *cat_pointer = UTF8LEX_CAT_SEP_LINE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_ZP:
    *cat_pointer = UTF8LEX_CAT_SEP_PARAGRAPH;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_CC:
    *cat_pointer = UTF8LEX_CAT_OTHER_CONTROL;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_CF:
    *cat_pointer = UTF8LEX_CAT_OTHER_FORMAT;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_CS:
    *cat_pointer = UTF8LEX_CAT_OTHER_SURROGATE;
    error = UTF8LEX_OK;
    break;
  case UTF8PROC_CATEGORY_CO:
    *cat_pointer = UTF8LEX_CAT_OTHER_PRIVATE;
    error = UTF8LEX_OK;
    break;

  default:
    error = UTF8LEX_ERROR_CAT;  // No idea what category this is!
    *cat_pointer = UTF8LEX_CAT_NONE;
    break;
  }

  return error;
}

// Or in the EXT category/ies (if any), according to utf8lex
// rules (for example, for line separators - utf8proc does not
// have a special category for CRLF etc
static utf8lex_error_t utf8lex_cat_ext(
        int32_t codepoint,
        utf8lex_cat_t *cat_pointer
        )
{
  if (cat_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  //
  // Separator rules specified by Unicode that are NOT obeyed
  // by utf8proc (except when using its malloc()-based functions,
  // and specifying options such as UTF8PROC_NLF2LS).
  //
  // From:
  //     Unicode 15.1.0
  //     https://www.unicode.org/reports/tr14/tr14-51.html#BK
  //     https://www.unicode.org/reports/tr14/tr14-51.html#CR
  //     https://www.unicode.org/reports/tr14/tr14-51.html#LF
  //     https://www.unicode.org/reports/tr14/tr14-51.html#NL
  //   Also for more details on history (such as VT / vertical tab), see:
  //     https://www.unicode.org/standard/reports/tr13/tr13-5.html#Definitions
  //
  // 000A    LINE FEED (LF)
  // 000B    LINE TABULATION (VT)
  // 000C    FORM FEED (FF)
  // 000D    CARRIAGE RETURN (CR) (unless followed by 000A LF)
  // 0085    NEXT LINE (NEL)
  // 2028    LINE SEPARATOR
  // 2029    PARAGRAPH SEPARATOR
  //
  switch (codepoint)
  {
  case 0x000A:  // LINE FEED (LF)
  case 0x000B:  // LINE TABULATION (VT)
  case 0x000C:  // FORM FEED (FF)
  case 0x000D:  // CARRIAGE RETURN (CR) (unless followed by 000A LF)
  case 0x0085:  // NEXT LINE (NEL)
  case 0x2028:  // LINE SEPARATOR
  case 0x2029:  // PARAGRAPH SEPARATOR
    *cat_pointer |= UTF8LEX_EXT_SEP_LINE;
    break;
  // default: do nothing.
  }

  return UTF8LEX_OK;
}

// Determines the category/ies of the specified Unicode 32 bit codepoint.
// Pass in a reference to the utf8lex_cat_t; on success, the specified
// utf8lex_cat_t pointer will be overwritten.
utf8lex_error_t utf8lex_cat_codepoint(
        int32_t codepoint,
        utf8lex_cat_t *cat_pointer
        )
{
  if (cat_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  // Figure out the utf8lex_cat_t equivalent of the utf8proc_category_t:
  utf8proc_category_t utf8proc_cat =
    utf8proc_category((utf8proc_int32_t) codepoint);
  utf8lex_cat_t utf8lex_cat = UTF8LEX_CAT_NONE;
  utf8lex_error_t error =
    utf8lex_cat_from_utf8proc_category(utf8proc_cat, &utf8lex_cat);
  if (error != UTF8LEX_OK)
  {
    // Couldn't figure out the first UTF-8 character's category.
    // Maybe a utf8lex bug?
    return error;
  }

  // Now or in the EXT category/ies (if any), according to utf8lex
  // rules (for example, for line separators - utf8proc does not
  // have a special category for CRLF etc
  error = utf8lex_cat_ext(codepoint,
                          &utf8lex_cat);
  if (error != UTF8LEX_OK)
  {
    // Couldn't figure out the first UTF-8 character's category.
    // Maybe a utf8lex bug?
    return error;
  }

  *cat_pointer = utf8lex_cat;

  return UTF8LEX_OK;
}

// Reads to the end of a grapheme, sets the first codepoint of the
// grapheme, and the number of bytes read.
// Note that CR, LF (U+000D, U+000A), often equivalent to '\r\n')
// is treated as a special grapheme cluster (however LF, CR is NOT for now),
// since the 2 characters, combined in sequence, usually represent
// one single line separator.
// The state is used only for the string buffer to read from,
// not for its location info.
// The offset, lengths, codepoint and cat are all set upon
// successfully reading one complete grapheme cluster.
utf8lex_error_t utf8lex_read_grapheme(
        utf8lex_state_t *state,
        off_t *offset_pointer,  // Mutable.
        size_t lengths_pointer[UTF8LEX_UNIT_MAX],  // Mutable.
        int32_t *codepoint_pointer,  // Mutable.
        utf8lex_cat_t *cat_pointer  // Mutable.
        )
{
  if (state == NULL
      || offset_pointer == NULL
      || lengths_pointer == NULL
      || codepoint_pointer == NULL
      || cat_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (*offset_pointer < (off_t) 0
           || *offset_pointer >= state->buffer->str->length_bytes)
  {
    return UTF8LEX_ERROR_BAD_START;
  }

  utf8lex_error_t error = UTF8LEX_ERROR_BAD_ERROR;
  off_t curr_offset = *offset_pointer;
  utf8proc_int32_t first_codepoint = (utf8proc_int32_t) -1;
  utf8lex_cat_t first_cat = UTF8LEX_CAT_NONE;
  utf8proc_int32_t prev_codepoint = (utf8proc_int32_t) -1;
  utf8proc_int32_t utf8proc_state = (utf8proc_int32_t) 0;;
  size_t total_bytes_read = (size_t) 0;
  size_t total_chars_read = (size_t) 0;
  size_t total_lines_read = (size_t) 0;
  for (int u8c = 0; ; u8c ++)
  {
    unsigned char *str_pointer = (unsigned char *)
      (state->buffer->str->bytes + curr_offset);
    size_t max_bytes = (size_t)
      (state->buffer->str->length_bytes - curr_offset);

    if (max_bytes <= (size_t) 0)
    {
      if (state->buffer->next != NULL)
      {
        // Continue reading from the next buffer in the chain.
        state->buffer = state->buffer->next;
        curr_offset = (off_t) 0;
        u8c --;
        continue;
      }
      else if (state->buffer->is_eof == true)
      {
        // No more bytes can be read in, we're at EOF.
        if (u8c == 0)
        {
          // We haven't read any bytes so far.  How did we get here...?
          error = UTF8LEX_NO_MATCH;  // Should we also return ..._EOF from here?
          break;
        }
        else
        {
          // Finished reading a grapheme with at least 1 valid codepoint.  Done.
          error = UTF8LEX_OK;
          break;
        }
      }
      else
      {
        // Ask for more bytes to be read in, so that we can find
        // a full grapheme, even if it straddles buffer boundaries.
        error = UTF8LEX_MORE;
      }
      break;
    }

    utf8proc_int32_t utf8proc_codepoint;
    utf8proc_ssize_t utf8proc_num_bytes_read = utf8proc_iterate(
        (utf8proc_uint8_t *) str_pointer,
        (utf8proc_ssize_t) max_bytes,
        &utf8proc_codepoint);
    int num_lines_read = 0;

    if (utf8proc_num_bytes_read == UTF8PROC_ERROR_INVALIDUTF8
        && (state->buffer->str->length_bytes - (size_t) curr_offset)
           < UTF8LEX_MAX_BYTES_PER_CHAR)
    {
      if (state->buffer->is_eof == true)
      {
        // No more bytes can be read in, we're at EOF.
        // Bad UTF-8 character at the end of the buffer.
        error = UTF8LEX_ERROR_BAD_UTF8;
      }
      else
      {
        // Need to read more bytes for the full grapheme.
        error = UTF8LEX_MORE;
      }
      break;
    }
    else if (utf8proc_num_bytes_read <= (utf8proc_ssize_t) 0)
    {
      // Possible errors:
      //     - UTF8PROC_ERROR_NOMEM
      //       Memory could not be allocated.
      //     - UTF8PROC_ERROR_OVERFLOW
      //       The given string is too long to be processed.
      //     - UTF8PROC_ERROR_INVALIDUTF8
      //       The given string is not a legal UTF-8 string.
      //     - UTF8PROC_ERROR_NOTASSIGNED
      //       The UTF8PROC_REJECTNA flag was set and an unassigned codepoint
      //       was found.
      //     - UTF8PROC_ERROR_INVALIDOPTS
      //       Invalid options have been used.
      if (u8c == 0)
      {
        error = UTF8LEX_ERROR_BAD_UTF8;
        break;
      }
      else
      {
        // Finished reading at least 1 codepoint.  Done.
        // We'll return to this bad character the next time we lex.
        error = UTF8LEX_OK;
        break;
      }
    }

    if (u8c == 0)  // First UTF-8 character we've read in the grapheme.
    {
      first_codepoint = utf8proc_codepoint;
    }
    else if (prev_codepoint == 0x000D  // Special case: CR, LF
             && utf8proc_codepoint == 0x000A)
    {
      // Not a grapheme break; these 2 characters in sequence
      // represent a special grapheme, 1 separator.
      num_lines_read = -1;
    }
    else  // Not the first UTF-8 character we've read in the grapheme.
    {
      // Check for grapheme break.  Keep reading until we find one.
      utf8proc_bool is_grapheme_break = utf8proc_grapheme_break_stateful(
          prev_codepoint,
          utf8proc_codepoint,
          &utf8proc_state);
      if (is_grapheme_break == (utf8proc_bool) true)
      {
        error = UTF8LEX_OK;
        break;
      }
    }

    // Check the category of character, so that we can determine
    // whether to increment # lines:
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    error = utf8lex_cat_codepoint((int32_t) utf8proc_codepoint,
                                  &cat);
    if (error != UTF8LEX_OK)
    {
      // Maybe a utf8lex bug?
      return error;
    }
    if (u8c == 0)
    {
      first_cat = cat;
    }
    if ((cat & UTF8LEX_CAT_SEP_LINE)
        || (cat & UTF8LEX_CAT_SEP_PARAGRAPH)
        || (cat & UTF8LEX_EXT_SEP_LINE))
    {
      num_lines_read ++;
    }

    curr_offset += (off_t) utf8proc_num_bytes_read;
    total_bytes_read += (size_t) utf8proc_num_bytes_read;
    total_chars_read += (size_t) 1;
    total_lines_read += (size_t) num_lines_read;

    // Keep reading more bytes until we find a grapheme boundary
    // (or run out of bytes).
    prev_codepoint = utf8proc_codepoint;
  }

  if (error != UTF8LEX_OK)
  {
    // Could be bad UTF-8, or need more bytes, etc.
    return error;
  }

  // Success.
  *offset_pointer += (off_t) total_bytes_read;
  lengths_pointer[UTF8LEX_UNIT_BYTE] = (size_t) total_bytes_read;
  lengths_pointer[UTF8LEX_UNIT_CHAR] = (size_t) total_chars_read;
  lengths_pointer[UTF8LEX_UNIT_GRAPHEME] = (size_t) 1;
  lengths_pointer[UTF8LEX_UNIT_LINE] = (size_t) total_lines_read;
  *codepoint_pointer = first_codepoint;
  // We only set the category/ies according to the first codepoint
  // of the grapheme cluster.  The remainder of the characters
  // in the grapheme cluster will be diacritics and so on,
  // so we ignore them for categorizationg purposes.
  *cat_pointer = first_cat;

  return UTF8LEX_OK;
}

static utf8lex_error_t utf8lex_lex_cat(
        utf8lex_token_type_t *token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  if (token_type == NULL
      || token_type->pattern == NULL
      || token_type->pattern->pattern_type == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (token_type->pattern->pattern_type != UTF8LEX_PATTERN_TYPE_CAT)
  {
    return UTF8LEX_ERROR_PATTERN_TYPE;
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  size_t length[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    length[unit] = (size_t) 0;
  }

  utf8lex_cat_pattern_t *cat_pattern =
    (utf8lex_cat_pattern_t *) token_type->pattern;
  for (int ug = 0;
       cat_pattern->max == -1 || ug < cat_pattern->max;
       ug ++)
  {
    // Read in one UTF-8 grapheme cluster:
    off_t grapheme_offset = offset;
    size_t grapheme_length[UTF8LEX_UNIT_MAX];  // Uninitialized is fine.
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        state,  // state, including absolute locations.
        &grapheme_offset,  // start byte, relative to start of buffer string.
        grapheme_length,  // size_t[] # bytes, chars, etc read.
        &codepoint,  // codepoint
        &cat  //cat
        );

    if (error == UTF8LEX_MORE)
    {
      return error;
    }
    else if (error != UTF8LEX_OK)
    {
      if (ug < cat_pattern->min)
      {
        return error;
      }
      else
      {
        // Finished reading at least (min) graphemes.  Done.
        // We'll return to this bad UTF-8 grapheme the next time we lex.
        break;
      }
    }

    if (cat_pattern->cat & cat)
    {
      // A/the category we're looking for.
      error = UTF8LEX_OK;
    }
    else if (ug < cat_pattern->min)
    {
      // Not the category we're looking for, and we haven't found
      // at least (min) graphemes matching this category, so fail
      // with no match.
      return UTF8LEX_NO_MATCH;
    }
    else
    {
      // Not a/the category we're looking for, but we already
      // finished reading at least (min) graphemes, so we're done.
      break;
    }

    // We found another grapheme of the expected cat.
    // Keep looking for more matches for this token,
    // until we hit the max.
    offset = grapheme_offset;
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      length[unit] += grapheme_length[unit];
    }
  }

  // Found what we're looking for.
  // Update buffer locations amd the absolute locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state->buffer->loc[unit].length = (int) length[unit];
    state->loc[unit].length = (int) length[unit];
  }

  utf8lex_error_t error = utf8lex_token_init(
      token_pointer,
      token_type,
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_lex_literal(
        utf8lex_token_type_t *token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  if (token_type == NULL
      || token_type->pattern == NULL
      || token_type->pattern->pattern_type == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (token_type->pattern->pattern_type != UTF8LEX_PATTERN_TYPE_LITERAL)
  {
    return UTF8LEX_ERROR_PATTERN_TYPE;
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  size_t remaining_bytes = state->buffer->str->length_bytes - (size_t) offset;

  utf8lex_literal_pattern_t *literal =
    (utf8lex_literal_pattern_t *) token_type->pattern;
  size_t token_length_bytes = literal->length[UTF8LEX_UNIT_BYTE];

  for (off_t c = (off_t) 0;
       (size_t) c < remaining_bytes && (size_t) c < token_length_bytes;
       c ++)
  {
    if (state->buffer->str->bytes[offset + c] != literal->str[c])
    {
      return UTF8LEX_NO_MATCH;
    }
  }

  if (remaining_bytes < token_length_bytes)
  {
    // Not enough bytes to read the string.
    // (It was matching for maybe a few bytes, anyway.)
    if (state->buffer->is_eof)
    {
      // No more bytes can be read in, we're at EOF.
      return UTF8LEX_NO_MATCH;
    }
    else
    {
      // Need to read more bytes for the full grapheme.
      return UTF8LEX_MORE;
    }
  }

  // Matched the literal exactly.

  // Update buffer locations and absolute locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state->buffer->loc[unit].length = literal->length[unit];
    state->loc[unit].length = literal->length[unit];
  }

  utf8lex_error_t error = utf8lex_token_init(
      token_pointer,
      token_type,
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


static utf8lex_error_t utf8lex_lex_regex(
        utf8lex_token_type_t *token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  if (token_type == NULL
      || token_type->pattern == NULL
      || token_type->pattern->pattern_type == NULL
      || state == NULL
      || state->buffer == NULL
      || state->buffer->str == NULL
      || token_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }
  else if (token_type->pattern->pattern_type != UTF8LEX_PATTERN_TYPE_REGEX)
  {
    return UTF8LEX_ERROR_PATTERN_TYPE;
  }

  off_t offset = (off_t) state->buffer->loc[UTF8LEX_UNIT_BYTE].start;
  size_t remaining_bytes = state->buffer->str->length_bytes - (size_t) offset;

  // For now we use the traditional (Perl-compatible, "NFA") algorithm:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#SEC28
  //
  // For differences between the traditional and "DFA" algorithms, see:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2matching.html
  pcre2_match_data *match = pcre2_match_data_create(
      1,  // ovecsize.  We only care about the whole match, not sub-groups.
      NULL);  // gcontext.
  if (match == NULL)
  {
    return UTF8LEX_ERROR_REGEX;
  }

  utf8lex_regex_pattern_t *regex_pattern =
    (utf8lex_regex_pattern_t *) token_type->pattern;

  // Match options:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#matchoptions
  //     PCRE2_ANCHORED
  //     The PCRE2_ANCHORED option limits pcre2_match() to matching
  //     at the first matching position. If a pattern was compiled
  //     with PCRE2_ANCHORED, or turned out to be anchored by virtue
  //     of its contents, it cannot be made unachored at matching time.
  //     Note that setting the option at match time disables JIT matching.
  //     PCRE2_NOTBOL
  //     This option specifies that first character of the subject string
  //     is not the beginning of a line, so the circumflex metacharacter
  //     should not match before it. Setting this without having
  //     set PCRE2_MULTILINE at compile time causes circumflex never to match.
  //     This option affects only the behaviour of the circumflex
  //     metacharacter. It does not affect \A.
  //     PCRE2_NOTEOL
  //     This option specifies that the end of the subject string
  //     is not the end of a line, so the dollar metacharacter
  //     should not match it nor (except in multiline mode) a newline
  //     immediately before it. Setting this without having
  //     set PCRE2_MULTILINE at compile time causes dollar never to match.
  //     This option affects only the behaviour of the dollar metacharacter.
  //     It does not affect \Z or \z.
  //     PCRE2_NOTEMPTY
  //     An empty string is not considered to be a valid match if this option
  //     is set. If there are alternatives in the pattern, they are tried.
  //     If all the alternatives match the empty string, the entire match
  //     fails. For example, if the pattern
  //         a?b?
  //     is applied to a string not beginning with "a" or "b", it matches
  //     an empty string at the start of the subject. With PCRE2_NOTEMPTY set,
  //     this match is not valid, so pcre2_match() searches further
  //     into the string for occurrences of "a" or "b". 
  int pcre2_error = pcre2_match(
      regex_pattern->regex,  // The pcre2_code (compiled regex).
      (PCRE2_SPTR) state->buffer->str->bytes,  // subject
      (PCRE2_SIZE) state->buffer->str->length_bytes,  // length
      (PCRE2_SIZE) offset,  // startoffset
      (uint32_t) PCRE2_ANCHORED,  // options (see above)
      match,  // match_data
      (pcre2_match_context *) NULL);  // NULL means use defaults.

  if (pcre2_error == PCRE2_ERROR_NOMATCH)
  {
    pcre2_match_data_free(match);
    return UTF8LEX_NO_MATCH;
  }
  else if (pcre2_error == PCRE2_ERROR_PARTIAL)
  {
    pcre2_match_data_free(match);
    if (state->buffer->is_eof)
    {
      // No more bytes can be read in, we're at EOF.
      // Bad UTF-8 character at the end of the buffer.
      return UTF8LEX_ERROR_BAD_UTF8;
    }
    else
    {
      // Need to read more bytes for the full grapheme.
      return UTF8LEX_MORE;
    }
  }
  // Negative number indicates error:
  // https://pcre2project.github.io/pcre2/doc/html/pcre2api.html#SEC32
  else if (pcre2_error < 0)
  {
    pcre2_match_data_free(match);
    return UTF8LEX_ERROR_REGEX;
  }

  uint32_t num_ovectors = pcre2_get_ovector_count(match);
  if (num_ovectors == (uint32_t) 0)
  {
    pcre2_match_data_free(match);
    return UTF8LEX_ERROR_REGEX;
  }

  // Extract the whole match from the ovectors:
  // pcre2_match_data *match_data);
  PCRE2_SIZE pcre2_match_length_bytes;
  pcre2_substring_length_bynumber(
      match, // match_data
      (uint32_t) 0,  // number
      &pcre2_match_length_bytes);
  size_t match_length_bytes = (size_t) pcre2_match_length_bytes;

  pcre2_match_data_free(match);

  if (match_length_bytes == (size_t) 0)
  {
    return UTF8LEX_NO_MATCH;
  }

  PCRE2_UCHAR match_substring[256];
  PCRE2_SIZE match_substring_length = 256;
  pcre2_substring_copy_bynumber(match, (uint32_t) 0, match_substring, &match_substring_length);

  // Matched.
  //
  // We know how many bytes matched the regular expression.
  // Now we'll now use utf8proc to count how many characters,
  // graphemes, lines, etc. are in the matching region.
  size_t length[UTF8LEX_UNIT_MAX];
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    length[unit] = (size_t) 0;
  }

  // Keep reading graphemes until we've reached the end of the regex match:
  for (int ug = 0;
       length[UTF8LEX_UNIT_BYTE] < match_length_bytes;
       ug ++)
  {
    // Read in one UTF-8 grapheme cluster per loop iteration:
    off_t grapheme_offset = offset;
    size_t grapheme_length[UTF8LEX_UNIT_MAX];  // Uninitialized is fine.
    int32_t codepoint = (int32_t) -1;
    utf8lex_cat_t cat = UTF8LEX_CAT_NONE;
    utf8lex_error_t error = utf8lex_read_grapheme(
        state,  // state, including absolute locations.
        &grapheme_offset,  // start byte, relative to start of buffer string.
        grapheme_length,  // size_t[] # bytes, chars, etc read.
        &codepoint,  // codepoint
        &cat  //cat
        );

    if (error != UTF8LEX_OK)
    {
      // pcre2 matched something that utf8proc either needs MORE bytes for,
      // or rejected outright.
      return error;
    }

    // We found another grapheme inside the regex match.
    // Keep looking for more graphemes inside the regex match.
    offset = grapheme_offset;
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      length[unit] += grapheme_length[unit];
    }
  }

  // Check to make sure pcre2 and utf8proc agree on # bytes.  (They should.)
  if (length[UTF8LEX_UNIT_BYTE] != match_length_bytes)
  {
    fprintf(stderr,
            "*** pcre2 and utf8proc disagree: pcre2 match_length_bytes = %d vs utf8proc = %d\n",
            match_length_bytes,
            length[UTF8LEX_UNIT_BYTE]);
  }

  // Update buffer locations amd the absolute locations:
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    state->buffer->loc[unit].length = (int) length[unit];
    state->loc[unit].length = (int) length[unit];
  }

  utf8lex_error_t error = utf8lex_token_init(
      token_pointer,
      token_type,
      state);  // For buffer and absolute location.
  if (error != UTF8LEX_OK)
  {
    return error;
  }

  return UTF8LEX_OK;
}


// A token pattern that matches a sequence of N characters
// of a specific utf8lex_cat_t cat, such as UTF8LEX_GROUP_WHITESPACE:
static utf8lex_pattern_type_t UTF8LEX_PATTERN_TYPE_CAT_INTERNAL =
  {
    .name = "CATEGORY",
    .lex = utf8lex_lex_cat,
    .clear = utf8lex_cat_pattern_clear
  };
utf8lex_pattern_type_t *UTF8LEX_PATTERN_TYPE_CAT =
  &UTF8LEX_PATTERN_TYPE_CAT_INTERNAL;

// A token pattern that matches a literal string,
// such as "int" or "==" or "proc" and so on:
static utf8lex_pattern_type_t UTF8LEX_PATTERN_TYPE_LITERAL_INTERNAL =
  {
    .name = "LITERAL",
    .lex = utf8lex_lex_literal,
    .clear = utf8lex_literal_pattern_clear
  };
utf8lex_pattern_type_t *UTF8LEX_PATTERN_TYPE_LITERAL =
  &UTF8LEX_PATTERN_TYPE_LITERAL_INTERNAL;

// A token pattern that matches a regular expression,
// such as "^[0-9]+" or "[\\p{N}]+" or "[_\\p{L}][_\\p{L}\\p{N}]*" or "[\\s]+"
// and so on:
static utf8lex_pattern_type_t UTF8LEX_PATTERN_TYPE_REGEX_INTERNAL =
  {
    .name = "REGEX",
    .lex = utf8lex_lex_regex,
    .clear = utf8lex_regex_pattern_clear
  };
utf8lex_pattern_type_t *UTF8LEX_PATTERN_TYPE_REGEX =
  &UTF8LEX_PATTERN_TYPE_REGEX_INTERNAL;


utf8lex_error_t utf8lex_lex(
        utf8lex_token_type_t *first_token_type,
        utf8lex_state_t *state,
        utf8lex_token_t *token_pointer
        )
{
  if (first_token_type == NULL
      || state == NULL
      || token_pointer == NULL)
  {
    return UTF8LEX_ERROR_NULL_POINTER;
  }

  if (state->loc[UTF8LEX_UNIT_BYTE].start < 0)
  {
    for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
         unit < UTF8LEX_UNIT_MAX;
         unit ++)
    {
      state->loc[unit].start = 0;
      state->loc[unit].length = 0;
    }
  }
  // EOF check:
  else if (state->buffer->loc[UTF8LEX_UNIT_BYTE].start
           >= state->buffer->str->length_bytes)
  {
    // We've lexed to the end of the buffer.
    if (state->buffer->next == NULL)
    {
      if (state->buffer->is_eof == true)
      {
        // Done lexing.
        return UTF8LEX_EOF;
      }
      else
      {
        // Please, sir, may I have some more?
        return UTF8LEX_MORE;
      }
    }

    // Move on to the next buffer in the chain.
    state->buffer = state->buffer->next;
  }

  utf8lex_token_type_t *matched = NULL;
  for (utf8lex_token_type_t *token_type = first_token_type;
       token_type != NULL;
       token_type = token_type->next)
  {
    utf8lex_error_t error;
    if (token_type->pattern == NULL
        || token_type->pattern->pattern_type == NULL
        || token_type->pattern->pattern_type->lex == NULL)
    {
      error = UTF8LEX_ERROR_NULL_POINTER;
      break;
    }

    // Call the pattern_type's lexer.  On successful tokenization,
    // it will set the absolute offset and lengths of the token
    // (and optionally update the lengths stored in the buffer
    // and absolute state).
    error = token_type->pattern->pattern_type->lex(
        token_type,
        state,
        token_pointer);

    if (error == UTF8LEX_NO_MATCH)
    {
      // Did not match this one token type.  Carry on with the loop.
      continue;
    }
    else if (error == UTF8LEX_MORE)
    {
      // Need to read more bytes before trying again.
      return error;
    }
    else if (error == UTF8LEX_OK)
    {
      // Matched the token type.  Break out of the loop.
      matched = token_type;
      break;
    }
    else
    {
      // Some other error.  Return the error to the caller.
      return error;
    }
  }

  // If we get this far, we've either 1) matched a token type,
  // or 2) not matched any token type.
  if (matched == NULL)
  {
    return UTF8LEX_NO_MATCH;
  }

  // We have a match.
  for (utf8lex_unit_t unit = UTF8LEX_UNIT_NONE + (utf8lex_unit_t) 1;
       unit < UTF8LEX_UNIT_MAX;
       unit ++)
  {
    int length_units = token_pointer->loc[unit].length;

    // Update buffer locations past end of this token:
    state->buffer->loc[unit].start += length_units;
    state->buffer->loc[unit].length = 0;
    // Update absolute locations past end of this token:
    state->loc[unit].start += length_units;
    state->loc[unit].length = 0;
  }

  return UTF8LEX_OK;
}
