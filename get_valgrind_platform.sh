#!/bin/sh

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
# Machine types returned by uname -m inside a Debian (including Ubuntu) system:
#   https://wiki.debian.org/ArchitectureSpecificsMemo#Architecture_baselines
# Platforms supported by Docker Debian official image:
#   https://hub.docker.com/_/debian
#
# Platform name matrix:
#
# uname -m            Docker/Debian       Valgrind
# ------------------- ------------------- -------------------
# i386                i386                X86/Linux (maintenance)
# i486                i386                X86/Linux (maintenance)
# i586                i386                X86/Linux (maintenance)
# i686                i386                X86/Linux (maintenance)
# x86_64              amd64               AMD64/Linux
# arm*                arm32v7             ARM/Linux
# aarch64             arm64v8             ARM64/Linux
# mips64              mips64le            UNSUPPORTED
# ppcle               ppc64le             PPC64LE/Linux
# ppc64le             ppc64le             PPC64LE/Linux
# riscv64             riscv64             UNSUPPORTED
# s390x               s390x               S390X/Linux
#

MACHINE_TYPE=`uname -m`
VALGRIND_PLATFORM=`echo "$MACHINE_TYPE" \
    | awk '
           $0 == "i386" || $0 == "i486" || $0 == "i586" || $0 == "i686" { print "X86/Linux"; }
           $0 == "x86_64" { print "AMD64/Linux"; }
           $0 ~ "^arm.*$" { print "ARM/Linux"; }
           $0 == "aarch64" { print "ARM64/Linux"; }
           $0 == "mips64" { print "UNSUPPORTED"; }
           $0 == "ppcle" || $0 == "ppc64le" { print "PPC64LE/Linux"; }
           $0 == "riscv64" { print "UNSUPPORTED"; }
           $0 == "s390x" { print "S390X/Linux"; }
          '`
EXIT_CODE=$?
if test $EXIT_CODE -ne 0
then
    echo "ERROR getting Valgrind platform for (uname -m) machine type: $MACHINE_TYPE" >&2
    exit $EXIT_CODE
elif test -z "$VALGRIND_PLATFORM"
then
    echo "ERROR Unknown Valgrind platform for (uname -m) machine type: $MACHINE_TYPE" >&2
    exit 1
fi

echo "$VALGRIND_PLATFORM"
exit 0
