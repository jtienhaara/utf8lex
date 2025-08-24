%{
/*
 * Integration test file for utf8lex.
 *
 * Copyright © 2023-2025 Johann Tienhaara
 * All rights reserved
 *
 * SPDX-License-Identifier: Apache-2.0
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
%}

%{
#define EXCLAMATION "Woohoo!"
%}

    #define QUESTION "What sort of exclamation should we cry?"

ID [_\p{L}][_\p{L}\p{N}]*
LINE_BREAK VSPACE | PARAGRAPH | NEWLINE
BACKIE "\\"
%%

ID { printf("Hello, ID world\n"); }
LINE_BREAK {}
[\+\-]?[1-9][0-9]*(\.[1-9][0-9]*)?(e[\+\-][1-9][0-9]*)? {
    printf("That was quite the number.\n");
}
BACKIE {;}

%%
const int FOO = 0x0042;
