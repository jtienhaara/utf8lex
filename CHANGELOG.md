# Changelog

Keep a Changelog: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
This project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Guiding Principles:
- Changelogs are for humans, not machines.
- There should be an entry for every single version.
- The same types of changes should be grouped.
- Versions and sections should be linkable.
- The latest version comes first.
- The release date of each version is displayed.
- Mention whether you follow Semantic Versioning.

Types of changes:
- Added for new features.
- Changed for changes in existing functionality.
- Deprecated for soon-to-be removed features.
- Removed for now removed features.
- Fixed for any bug fixes.
- Security in case of vulnerabilities.

## [Unreleased]

### Added
- First crack at utf8lex by @jtienhaara
- Data structures for lexical analysis of UTF-8 documents.
- Relies on two wonderful libraries:
  - [libutf8proc](https://github.com/JuliaStrings/utf8proc)
  - [libpcre2-8](https://github.com/PCRE2Project/pcre2)
- Kludgy re-implementation of "lex" command for UTF-8
  ([src/utf8lex.c](src/utf8lex.c) and src/utf8lex_generate_*.c).
- Some unit and integration tests (no coverage stats yet).
- Some examples (nothing serious yet
  - TODO use utf8lex to [lex a real .c file](https://www.lysator.liu.se/c/ANSI-C-grammar-l.html)).
- CI with Makefile (including valgrind for memory checking, where available)
  and GitHub Actions across various Debian platforms:
  - linux/386
  - linux/amd64
  - linux/arm/v7
  - linux/arm64
  - linux/mips64le
  - linux/ppc64le
  - linux/s390x
