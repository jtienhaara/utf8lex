#
# utf8lex
# Copyright © 2023-2025 Johann Tienhaara
# All rights reserved
#
# SPDX-License-Identifier: Apache-2.0
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

.PHONY: all
all: templates build test examples

.PHONY: build-debian-container
build-debian-container:
	@DEBIAN_ARCHITECTURE=`./get_debian_architecture.sh` \
	    && VALGRIND_PLATFORM=`./get_valgrind_platform.sh` \
	    && echo "DEBIAN_ARCHITECTURE='$$DEBIAN_ARCHITECTURE'" \
	    && echo "VALGRIND_PLATFORM='$$VALGRIND_PLATFORM'" \
	    && docker build . \
	        --file debian.Dockerfile \
	        --platform "$$DEBIAN_ARCHITECTURE" \
	        --build-arg "VALGRIND_PLATFORM=$$VALGRIND_PLATFORM" \
	        --tag utf8lex-debian:latest

.PHONY: debian-container
debian-container: build-debian-container
	docker run \
	    --rm \
	    --volume `pwd`:/utf8lex:rw \
	    utf8lex-debian:latest \
	    bash -c 'make clean && make all'

.PHONY: debian-container-re-version
debian-container-re-version: build-debian-container
	docker run \
	    --rm \
	    --env UTF8LEX_VERSION=$$UTF8LEX_VERSION \
	    --volume `pwd`:/utf8lex:rw \
	    utf8lex-debian:latest \
	    bash -c 'make re-version'

.PHONY: debian-container-debug
debian-container-debug: build-debian-container
	docker run \
	    --rm \
	    -i --tty \
	    --volume `pwd`:/utf8lex:rw \
	    utf8lex-debian:latest \
	    bash -c 'make clean && make templates && make build && make all && echo "***** No core dump, no debug *****" || make debug'

.PHONY: templates
templates:
	cd templates \
	    && make templates

.PHONY: build
build:
	cd src \
	    && make build

.PHONY: test
test: unit_tests integration_tests

.PHONY: unit_tests
unit_tests:
	cd tests/unit \
	    && make build \
	    && make run

.PHONY: integration_tests
integration_tests:
	cd tests/integration \
	    && make build \
	    && make run

.PHONY: examples
examples:
	cd examples \
	    && make build \
	    && make run

.PHONY: re-version
re-version:
	cd src \
	    && make re-version

.PHONY: debug
debug:
	cd tests/integration \
	    && make build \
	    && make debug

.PHONY: clean
clean:
	cd templates \
	    && make clean
	cd src \
	    && make clean
	cd tests/unit \
	    && make clean
	cd tests/integration \
	    && make clean
	cd examples \
	    && make clean
