#
# utf8lex
# Copyright © 2023-2025 Johann Tienhaara
# All rights reserved
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# @version Last updated 2025-01-26
# Debian Bookworm (Debian 12)
# Image from:
#
#     https://hub.docker.com/_/debian
#
FROM debian:12.11-slim

#
# Docker's builtin TARGETARCH build arg:
#
#     https://docs.docker.com/engine/reference/builder/?_gl=1*13mno0d*_ga*NjYxNDI5MzM5LjE2OTQxMDIzNzI.*_ga_XJWPQMJYHQ*MTY5NDQ1MzA1OS4yLjEuMTY5NDQ1MzI4Ny4yOC4wLjA.#automatic-platform-args-in-the-global-scope
#
ARG TARGETARCH

#
# Valgrind platform, for checking for memory leaks etc on most platforms.
# Platforms supported by valgrind:
#   https://valgrind.org/info/platforms.html
#
# TARGETARCH          VALGRIND_PLATFORM
# ------------------- -------------------
# i386                X86/Linux (maintenance)
# ---                 X86/Android
# ---                 X86/Darwin
# ---                 X86/FreeBSD
# ---                 X86/illumos
# ---                 X86/Solaris
# amd64               AMD64/Linux
# ---                 AMD64/FreeBSD
# ---                 AMD64/illumos
# ---                 AMD64/FreeBSD
# ---                 AMD64/Solaris
# arm32v5             ---
# ---                 ---
# arm32v7             ARM/Linux
# ---                 ARM/Android
# arm64v8             ARM64/Linux
# ---                 ARM64/Android
# ---                 ARM64/FreeBSD
# ---                 MIPS32/Linux
# ---                 MIPS32/Android
# ---                 MIPS64/Linux
# mips64le            ---
# ---                 PPC32/Linux
# ---                 PPC64/Linux
# ppc64le             PPC64LE/Linux
# riscv64             ---
# s390x               S390X/Linux
#
# On platforms where valgrind is unsupported (VALGRIND_PLATFORM="UNSUPPORTED"),
# we run tests without memory checks etc.
#
ARG VALGRIND_PLATFORM

USER root

ENV DEBIAN_ARCHITECTURE=$TARGETARCH
ENV VALGRIND_PLATFORM=$VALGRIND_PLATFORM
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC/UTC

#
# Packages for utf8lex:
#
#     ca-certificates
#         Latest certificate authorities.
#     gcc
#         C compiler.
#     gdb
#         C debugger.
#     libpcre2-dev
#         UTF-8 character regular expressions.
#     libutf8proc-dev
#         UTF-8 character reading and categorizing.
#     libutf8proc2
#         UTF-8 character reading and categorizing.
#     locales [required] [dynamic]
#         Required both for building C code, and for runtime reading
#         and writing of UTF-8-encoded characters.
#     make
#         Traditional make.  Required for building things from Makefiles.
#     strace
#         For troubleshooting.
#     valgrind
#         For troubleshooting and finding memory leaks.
#
RUN apt-get update --yes \
    && apt-get install --no-install-recommends --yes \
       ca-certificates \
       gcc \
       gdb \
       libpcre2-dev \
       libutf8proc-dev \
       libutf8proc2 \
       locales \
       make \
       strace \
    && if test "$VALGRIND_PLATFORM" != "UNSUPPORTED"; \
       then \
           apt-get install --no-install-recommends --yes \
               valgrind \
               ; \
       fi \
    && apt-get clean

ENV LC_CTYPE=C.utf8

#
# User utf8lex
#
# Install a non-root user, to mitigate intrusions and mistakes
# at runtime.
#
# 32 digit random number for utf8lex user's password.
#
RUN mkdir /home/utf8lex \
    && useradd \
           --home-dir /home/utf8lex \
           --password `od --read-bytes 32 --format u --address-radix none /dev/urandom | tr --delete ' \n'` \
           utf8lex \
    && chown -R utf8lex:utf8lex /home/utf8lex

#
# utf8lex working directory
#
RUN mkdir -p /utf8lex \
    && chown -R utf8lex:utf8lex /utf8lex \
    && chmod ug+rwx,o-rwx /utf8lex

USER utf8lex
WORKDIR /utf8lex

#
# No default entrypoint.
#
CMD [""]
ENTRYPOINT [""]
