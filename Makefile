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
# Parameters for gcc compiling .c into .o files, .o files into exes, etc:
#
CC_ARGS = -Werror
LINK_ARGS =

LC_CTYPE=en_US.UTF-8

.PHONY: all
all: build test

.PHONY: container
container:
	docker build . \
	    --file build.Dockerfile \
	    --tag utf8lex:latest
	docker run \
	    --rm \
	    --volume `pwd`:/utf8lex:rw \
	    utf8lex:latest \
	    bash -c 'make clean && make all'

.PHONY: build
build:
	cd src \
	    && make build

clean:
	cd src \
	    && make clean
	cd tests/unit \
	    && make clean
	cd tests/integration \
	    && make clean

.PHONY: test
test: build unit_tests integration_tests

.PHONY: unit_tests
unit_tests: build
	cd tests/unit \
	    && make build \
	    && make run

.PHONY: integration_tests
integration_tests: build
	cd tests/integration \
	    && make build \
	    && make run
