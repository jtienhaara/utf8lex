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

ORIGINAL_DIR = $(PWD)

BUILD_DIR ?= ../build

#
# Do NOT include utf8lex.c (with main()) in SOURCE_FILES:
#
TEMPLATE_FILES ?= \
	c/mmap/head.c \
	c/mmap/tail.c

GENERATED_SOURCE_FILES = \
	$(patsubst %.c,$(BUILD_DIR)/templates/utf8lex_template_%.c,$(notdir $(TEMPLATE_FILES)))

.PHONY: all
all: templates

TEMPLATE_C_HEAD = /* \
	| * utf8lex \
	| * Copyright © 2023-2025 Johann Tienhaara \
	| * All rights reserved \
	| * \
	| * SPDX-License-Identifier: Apache-2.0 \
	| * \
	| * Licensed under the Apache License, Version 2.0 (the \"License\"); \
	| * you may not use this file except in compliance with the License. \
	| * You may obtain a copy of the License at \
	| * \
	| *     http://www.apache.org/licenses/LICENSE-2.0 \
	| * \
	| * Unless required by applicable law or agreed to in writing, software \
	| * distributed under the License is distributed on an \"AS IS\" BASIS, \
	| * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. \
	| * See the License for the specific language governing permissions and \
	| * limitations under the License. \
	| */ \
	| \
	|\#include <stdio.h> \
	|

# Rule for $(GENERATED_SOURCE_FILES):
$(BUILD_DIR)/templates/utf8lex_template_%.c: c/mmap/%.c
	mkdir -p $(BUILD_DIR)/templates
	echo "$(TEMPLATE_C_HEAD)" \
	    | awk '{ \
	               line = $$0; \
	               gsub(/[ ]*\|/, "\n", line); \
	               printf "%s\n", line; \
	           }' \
	    > $@
	for TEMPLATE_CONST in `echo "$<" \
	    | sed 's|\.c$$||' \
	    | sed 's|[^a-zA-Z_0-9]|_|g' \
	    | awk '{ print toupper($$0); }'` ; \
	do \
	    echo "const unsigned char *$$TEMPLATE_CONST =" \
	        >> $@ ; \
	    break; \
	done
	cat $< \
	    | sed 's|\\|\\\\|g' \
	    | sed 's|"|\\\"|g' \
	    | awk 'BEGIN { \
	               printf "    \""; \
	           } \
	           { \
	               line = $$0; \
	               printf "%s\\n", line; \
	           } \
	           END { \
	               print "\";"; \
	           }' \
	    >> $@

.PHONY: templates
templates: $(GENERATED_SOURCE_FILES)
	@echo "templates = $(TEMPLATE_FILES) generateds = $(GENERATED_SOURCE_FILES)"

.PHONY: clean
clean:
	rm -f $(GENERATED_SOURCE_FILES)
