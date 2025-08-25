# utf8lex

|  [![Build](https://img.shields.io/github/actions/workflow/status/jtienhaara/utf8lex/build.yaml)](https://github.com/jtienhaara/utf8lex/blob/main/.github/workflows/build.yaml)  |  [![Release](https://img.shields.io/github/v/release/jtienhaara/utf8lex)](https://github.com/jtienhaara/utf8lex/releases)  |
|  [![License](https://img.shields.io/github/license/jtienhaara/utf8lex)](https://github.com/jtienhaara/utf8lex/blob/main/LICENSE)
|  ![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)  |  [![Valgrind](https://img.shields.io/badge/Valgrind-006094?style=flat-square)](https://github.com/jtienhaara/utf8lex/blob/main/src/Makefile)  |
|  ![Debian](https://img.shields.io/badge/Debian-D70A53?style=for-the-badge&logo=debian&logoColor=white)  |  386  |  amd64  |  arm/v7  |  arm64  |  mips64le  |  ppc64le  |  s390x  |

C lexing of [UTF-8-encoded characters](http://en.wikipedia.org/wiki/UTF-8).

`utf8lex` is mostly intended to be used as a library.  Static .a and dynamic .so
libraries are built for various Debian-Linux architectures:

- linux/386
- linux/amd64
- linux/arm/v7
- linux/arm64
- linux/mips64le
- linux/ppc64le
- linux/s390x

A kludgy commandline replacement for the `lex` command is included,
the `utf8lex` command.  But it is not ready for serious lexing tasks.
Many `lex` features have not yet been implemented.


## Features

- Builtin categories (`utf8lex_cat_t`) for various UTF-8 graphemes,
  including groups (`UTF8LEX_GROUP_LETTER`, `UTF8LEX_GROUP_NOT_LETTER`,
  `UTF8LEX_GROUP_HSPACE` for horizontal whitespace,
  `UTF8LEX_GROUP_VSPACE` for vertical whitespace such as newlines,
  `UTF8LEX_EXT_SEP_LINE` for most newline needs such as '\r' and '\n',
  and so on).
- Definitions (`utf8lex_definition_t`) to define the structures
  of lexical elements.
  - `utf8lex_cat_definition_t` can be used to analyze individual graphemes
    by the builtin categories.
  - `utf8lex_literal_definition_t` can be used to match exact tokens,
    such as "class" or "<=".
  - `utf8lex_regex_definition_t` can be used to match tokens
    by regular expressions, using
    [pcre2 regular expression syntax](https://pcre2project.github.io/pcre2/doc/pcre2syntax/).
    For example, an integer might be matched against the regex
    `[\+\-]?[1-9][0-9]*(e[\+]?[1-9][0-9]*)?`, or a floating point number
    might be matched against the regex
    `[\+\-]?[1-9][0-9]*(\.[1-9][0-9]*)?(e[\+\-]?[1-9][0-9]*)?`,
    or a variable name or other id might be matched against the regex
    `[_\p{L}][_\p{L}\p{N}]*`.
  - `utf8lex_multi_definition_t` can be used to combine sequences or
    "OR'ed" definitions.  For example, a newline definition might be made of
    multiple "OR'ed" builtin categories: `VSPACE | PARAGRAPH | NEWLINE`.
    Or an expression with integers might be made of multiple more complex
    definitions: `INT WHITESPACE* OP WHITESPACE* INT` (where INT, OP
    and WHITESPACE definitions are provided before lexing).
- Rules ('utf8lex_rule_t`), each of which has a name and some source code
  in a string (such as C code for traditional `lex` analysis).
- All lexically analyzed tokens (`utf8lex_token_t`) come with multi-level
  source code tracking, all 0-indexed:
  - `UTF8LEX_UNIT_BYTE` within the source file or text that was analyzed;
  - `UTF8LEX_UNIT_CHAR`, the character position (UTF-8 chars are 1-6 bytes);
  - `UTF8LEX_UNIT_GRAPHEME`, the grapheme position (considering diacritics);
  - `UTF8LEX_UNIT_LINE`, calculated automatically (by `utf8lex_read.c`)
    whenever a character in any of the following categories is read
    during lexical analysis:
    - `UTF8LEX_CAT_SEP_LINE`
    - `UTF8LEX_CAT_SEP_PARAGRAPH`
    - `UTF8LEX_EXT_SEP_LINE`
- No mallocs or frees in `utf8lex` itself (stack-based allocation).
  - However, note that `pcre2` does malloc and free for regular expressions.


## Examples

The [examples directory](examples/) contains `.l` example files, such as:

- [programming_tokens.l](examples/programming_tokens.l)
- [programming_kludgy_grammar.l](examples/programming_kludgy_grammar.l)

These lexicons can be used with `utf8lex` kludgy commandline tool
to generate a lexer that will analyze the not-really-very-useful
example of a custom language source file:
[program_001.language](examples/program_001.language).

To build a `.l` file into a `.c` file with locally built utf8lex program
and libraries:

```
cd examples
../build/utf8lex programming_tokens.l
```

To then compile the `.c` file, along with the example lexer, into a program:

```
gcc -I../src -c example_lexer.c -o example_lexer.o
gcc -I../src -c programming_tokens.c -o programming_tokens.o
gcc -Wl,--no-as-needed -lutf8proc -lpcre2-8 \
    -L../build -Wl,-rpath,../build -lutf8lex \
    example_lexer.o programming_tokens.o \
    -o programming_tokens
```

The newly built lexer can then be used to analyze `program_001.language`
and output tokens to stdout:

```
./programming_tokens program_001.language
```

With output along the lines of:

```
TOKEN: rule_16 "// This is an example programming language file."
TOKEN: rule_1 "\n"
TOKEN: rule_16 "// Lex it by building the example_lexer program in this directory (make build),"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "// then run:"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     example_lexer program_001.language"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "// The example_lexer will output the tokens read by the lexical analyzer,"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "// followed by EOF:"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     TOKEN: rule_16 \"// This is an example programming language file.\""
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     TOKEN: rule_1 \"\\n\""
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     TOKEN: rule_16 \"// Lex it by building the example_lexer program in this directory (make build),\""
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     TOKEN: rule_1 \"\\n\""
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     TOKEN: rule_16 \"// then run:\""
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     TOKEN: rule_1 \"\\n\""
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     TOKEN: rule_16 \"//\""
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     TOKEN: rule_1 \"\\n\""
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     TOKEN: rule_16 \"//     example_lexer program_001.language\""
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     ..."
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//     EOF"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "// This example programming language file is released to the public domain."
TOKEN: rule_1 "\n"
TOKEN: rule_16 "// (It's useless.  It's just a contrived example.)"
TOKEN: rule_1 "\n"
TOKEN: rule_16 "//"
TOKEN: rule_1 "\n"
TOKEN: rule_68 "x"
TOKEN: rule_1 " "
TOKEN: rule_27 "="
TOKEN: rule_1 " "
TOKEN: rule_66 "6"
TOKEN: rule_1 " "
TOKEN: rule_60 "*"
TOKEN: rule_1 " "
TOKEN: rule_66 "9"
TOKEN: rule_1 "\n"
TOKEN: rule_68 "x"
TOKEN: rule_27 "="
TOKEN: rule_66 "6"
TOKEN: rule_60 "*"
TOKEN: rule_66 "9"
TOKEN: rule_1 "\n"
TOKEN: rule_68 "x"
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_27 "="
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_66 "6"
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_60 "*"
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_66 "9"
TOKEN: rule_1 "\n"
TOKEN: rule_68 "int"
TOKEN: rule_1 " "
TOKEN: rule_68 "x"
TOKEN: rule_1 " "
TOKEN: rule_27 "="
TOKEN: rule_1 " "
TOKEN: rule_66 "6"
TOKEN: rule_1 " "
TOKEN: rule_60 "*"
TOKEN: rule_1 " "
TOKEN: rule_66 "9"
TOKEN: rule_1 "\n"
TOKEN: rule_68 "int"
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_1 " "
TOKEN: rule_68 "x"
TOKEN: rule_27 "="
TOKEN: rule_66 "6"
TOKEN: rule_60 "*"
TOKEN: rule_66 "9"
TOKEN: rule_1 "\n"
TOKEN: rule_68 "y"
TOKEN: rule_1 " "
TOKEN: rule_27 "="
TOKEN: rule_1 " "
TOKEN: rule_6 "\"This is a double quoted string with \\\" in the middle.\""
TOKEN: rule_1 "\n"
TOKEN: rule_68 "z"
TOKEN: rule_1 " "
TOKEN: rule_27 "="
TOKEN: rule_1 " "
TOKEN: rule_7 "'This is a single quoted string with \\\" and \\' and \\\\'"
TOKEN: rule_1 "\n"
TOKEN: rule_68 "empty_string"
TOKEN: rule_1 " "
TOKEN: rule_27 "="
TOKEN: rule_1 " "
TOKEN: rule_6 "\"\""
TOKEN: rule_1 "\n"
TOKEN: rule_68 "anywhere_between_1_and_10_inclusive"
TOKEN: rule_1 " "
TOKEN: rule_27 "="
TOKEN: rule_1 " "
TOKEN: rule_4 "1..10"
TOKEN: rule_1 "\n"
EOF
```


## Downloading

To download a release, visit the
[utf8lex releases](https://github.com/jtienhaara/utf8lex/releases) page.
Dynamic and static libraries and commandline executables are available.


## Building

To build `utf8lex` using a Debian container, you'll need to install
`make` and `docker`, then run `make debian-container`.  This will
build the dynamic and static libraries and the commandline `utf8lex`
in the ./build directory, and build and run all tests and examples.

To build `utf8lex` from scratch, you'll need to install:

- `gcc`
- `pcre2` (`libpcre2-dev` on Debian)
- `utf8proc` (`libutf8proc-dev` and `libutf8proc2` on Debian)
- `make`
- `awk`
- `sed`
- `valgrind` (or `export VALGRIND_PLATFORM=UNSUPPORTED` to build without valgrind)

Make sure your character encoding type is set to UTF-8.  For example, on Debian:

`LC_CTYPE=C.utf8`

Now you can either build all source, libraries, executable and build and run
all tests and examples:

```
make
```

Or you can build and/or run individual components:

```
make templates
make build
make unit_tests
make integration_tests
make test
make examples
```


## Data structures

`utf8lex`'s data structures are defined in [src/utf8lex.h](src/utf8lex.h).
Documentation is pretty close to non-existent at this stage,
so using the data structures is currently a matter of checking out
[unit tests](tests/unit) or [integration tests](tests/integration)
for guidance.


## Dependencies

`utf8lex` depends on two wonderful open source UTF-8-friendly libraries:

- [utf8proc](https://github.com/JuliaStrings/utf8proc) for character handling
- [pcre2](https://github.com/PCRE2Project/pcre2) for regular expressions

When building with `libutf8lex.so`, or using `utf8lex` from the commandlne,
you will need to have these two dependencies installed.  For example, on Debian:

```
sudo apt-get update \
    && sudo apt-get install libpcre2-dev libutf8proc-dev libutf8proc2
```

Installing other tools (such as gcc and valgrind) might be useful,
depending on your needs.  See [debian.Dockerfile](debian.Dockerfile),
for example, for the tools and libraries used to build `utf8lex`.


## Licenses

| utf8lex:                 |
|--------------------------|------------|
| SPDX-License-Identifier: | Apache-2.0 |

| Dependencies: |
|---------------|
| utf8proc      | SPDX-License-Identifier: | MIT                               |
|               | SPDX-License-Identifier: | Unicode-DFS-2016	               |
| pcre2         | SPDX-License-Identifier: | BSD-3-Clause WITH PCRE2-exception |

[utf8proce LICENSE.md](https://github.com/JuliaStrings/utf8proc/blob/master/LICENSE.md)

[pcre2 LICENSE.md](https://github.com/PCRE2Project/pcre2/blob/master/LICENCE.md)


## TODO

Eventually there will be a [GitHub Issues list](https://github.com/jtienhaara/utf8lex/issues) to track todo items, once a stable release has been created.


## Contact

`utf8lex` is made by @jtienhaara.  For now, until issues have been set up,
bugs, feature requests and other queries will go through him.
