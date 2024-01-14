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
#include <inttypes.h>  // For int32_t.
#include <stdbool.h>  // For bool, true, false.
#include <string.h>  // For strlen(), strncmp().

#include <utf8proc.h>

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

const utf8lex_cat_t UTF8LEX_GROUP_ALL =
  UTF8LEX_CAT_OTHER_NA
  | UTF8LEX_CAT_LETTER_UPPER
  | UTF8LEX_CAT_LETTER_LOWER
  | UTF8LEX_CAT_LETTER_TITLE
  | UTF8LEX_CAT_LETTER_MODIFIER
  | UTF8LEX_CAT_LETTER_OTHER
  | UTF8LEX_CAT_MARK_NON_SPACING
  | UTF8LEX_CAT_MARK_SPACING_COMBINING
  | UTF8LEX_CAT_MARK_ENCLOSING
  | UTF8LEX_CAT_NUM_DECIMAL
  | UTF8LEX_CAT_NUM_LETTER
  | UTF8LEX_CAT_NUM_OTHER
  | UTF8LEX_CAT_PUNCT_CONNECTOR
  | UTF8LEX_CAT_PUNCT_DASH
  | UTF8LEX_CAT_PUNCT_OPEN
  | UTF8LEX_CAT_PUNCT_CLOSE
  | UTF8LEX_CAT_PUNCT_QUOTE_OPEN
  | UTF8LEX_CAT_PUNCT_QUOTE_CLOSE
  | UTF8LEX_CAT_PUNCT_OTHER
  | UTF8LEX_CAT_SYM_MATH
  | UTF8LEX_CAT_SYM_CURRENCY
  | UTF8LEX_CAT_SYM_MODIFIER
  | UTF8LEX_CAT_SYM_OTHER
  | UTF8LEX_CAT_SEP_SPACE
  | UTF8LEX_CAT_SEP_LINE
  | UTF8LEX_CAT_SEP_PARAGRAPH
  | UTF8LEX_CAT_OTHER_CONTROL
  | UTF8LEX_CAT_OTHER_FORMAT
  | UTF8LEX_CAT_OTHER_SURROGATE
  | UTF8LEX_CAT_OTHER_PRIVATE
  | UTF8LEX_EXT_SEP_LINE;
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
const utf8lex_cat_t UTF8LEX_GROUP_NOT_OTHER =
  UTF8LEX_GROUP_ALL & (~UTF8LEX_GROUP_OTHER);
const utf8lex_cat_t UTF8LEX_GROUP_LETTER =
  UTF8LEX_CAT_LETTER_UPPER
  | UTF8LEX_CAT_LETTER_LOWER
  | UTF8LEX_CAT_LETTER_TITLE
  | UTF8LEX_CAT_LETTER_MODIFIER
  | UTF8LEX_CAT_LETTER_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_NOT_LETTER =
  UTF8LEX_GROUP_ALL & (~UTF8LEX_GROUP_LETTER);
const utf8lex_cat_t UTF8LEX_GROUP_MARK =
  UTF8LEX_CAT_MARK_NON_SPACING
  | UTF8LEX_CAT_MARK_SPACING_COMBINING
  | UTF8LEX_CAT_MARK_ENCLOSING;
const utf8lex_cat_t UTF8LEX_GROUP_NOT_MARK =
  UTF8LEX_GROUP_ALL & (~UTF8LEX_GROUP_MARK);
const utf8lex_cat_t UTF8LEX_GROUP_NUM =
  UTF8LEX_CAT_NUM_DECIMAL
  | UTF8LEX_CAT_NUM_LETTER
  | UTF8LEX_CAT_NUM_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_NOT_NUM =
  UTF8LEX_GROUP_ALL & (~UTF8LEX_GROUP_NUM);
const utf8lex_cat_t UTF8LEX_GROUP_PUNCT =
  UTF8LEX_CAT_PUNCT_CONNECTOR
  | UTF8LEX_CAT_PUNCT_DASH
  | UTF8LEX_CAT_PUNCT_OPEN
  | UTF8LEX_CAT_PUNCT_CLOSE
  | UTF8LEX_CAT_PUNCT_QUOTE_OPEN
  | UTF8LEX_CAT_PUNCT_QUOTE_CLOSE
  | UTF8LEX_CAT_PUNCT_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_NOT_PUNCT =
  UTF8LEX_GROUP_ALL & (~UTF8LEX_GROUP_PUNCT);
const utf8lex_cat_t UTF8LEX_GROUP_SYM =
  UTF8LEX_CAT_SYM_MATH
  | UTF8LEX_CAT_SYM_CURRENCY
  | UTF8LEX_CAT_SYM_MODIFIER
  | UTF8LEX_CAT_SYM_OTHER;
const utf8lex_cat_t UTF8LEX_GROUP_NOT_SYM =
  UTF8LEX_GROUP_ALL & (~UTF8LEX_GROUP_SYM);
const utf8lex_cat_t UTF8LEX_GROUP_HSPACE =
  UTF8LEX_CAT_SEP_SPACE;
const utf8lex_cat_t UTF8LEX_GROUP_NOT_HSPACE =
  UTF8LEX_GROUP_ALL & (~UTF8LEX_GROUP_HSPACE);
const utf8lex_cat_t UTF8LEX_GROUP_VSPACE =
  UTF8LEX_CAT_SEP_LINE
  | UTF8LEX_CAT_SEP_PARAGRAPH
  | UTF8LEX_EXT_SEP_LINE;
const utf8lex_cat_t UTF8LEX_GROUP_NOT_VSPACE =
  UTF8LEX_GROUP_ALL & (~UTF8LEX_GROUP_VSPACE)
  // Unfortunately we have to remove LF, CR, etc this way:
  & (~UTF8LEX_CAT_OTHER_CONTROL);
const utf8lex_cat_t UTF8LEX_GROUP_WHITESPACE =
  UTF8LEX_GROUP_HSPACE
  | UTF8LEX_GROUP_VSPACE;
const utf8lex_cat_t UTF8LEX_GROUP_NOT_WHITESPACE =
  UTF8LEX_GROUP_ALL & (~UTF8LEX_GROUP_WHITESPACE)
  // Unfortunately we have to remove LF, CR, etc this way:
  & (~UTF8LEX_CAT_OTHER_CONTROL);


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

  // Curently can format up to 404 bytes.
  // (All 40 possible tokens 284 bytes + 40 * 3 chars (" | ") / 120 bytes.)

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
  if ((remaining_cat & UTF8LEX_GROUP_HSPACE) == UTF8LEX_GROUP_HSPACE)
  {
    // 10-13 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sHSPACE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_GROUP_HSPACE);
    remaining_num_bytes -= num_bytes_written;
    total_bytes_written += num_bytes_written;
    maybe_or = or;
  }
  if ((remaining_cat & UTF8LEX_GROUP_VSPACE) == UTF8LEX_GROUP_VSPACE)
  {
    // 10-13 bytes.
    size_t num_bytes_written = snprintf(str_pointer + total_bytes_written,
                                        remaining_num_bytes,
                                        "%sVSPACE",
                                        maybe_or);
    remaining_cat = remaining_cat & (~ UTF8LEX_GROUP_VSPACE);
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
                                        "%sSPACE",
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
                                        "%sLINE",
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
      cat |= UTF8LEX_GROUP_HSPACE;
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
    else if (strncmp("LINE", ptr, (size_t) 4) == 0)
    {
      cat |= UTF8LEX_CAT_SEP_LINE;
      c += (off_t) 4;
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
    else if (strncmp("SPACE", ptr, (size_t) 5) == 0)
    {
      cat |= UTF8LEX_CAT_SEP_SPACE;
      c += (off_t) 6;
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
      cat |= UTF8LEX_GROUP_VSPACE;
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
