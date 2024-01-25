#
# utf8lex
# Copyright 2023 Johann Tienhaara
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
# @version Last updated 2023-12-24
# Debian Bookworm (Debian 12)
# Image from:
#
#     https://hub.docker.com/_/debian
#
FROM debian:12.2-slim

USER root

ENV DEBIAN_FRONTEND=noninteractive

#
# Packages for utf8lex:
#
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
#     make [required] [dynamic] [security]
#         Traditional make.  Required for building things from Makefiles.
#
RUN apt-get update --yes \
    && apt-get install --no-install-recommends --yes \
       gcc \
       gdb \
       libpcre2-dev \
       libutf8proc-dev \
       libutf8proc2 \
       locales \
       make \
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
