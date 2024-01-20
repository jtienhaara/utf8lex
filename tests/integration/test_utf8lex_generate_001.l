%{
#define EXCLAMATION "Woohoo!"
%}

    #define QUESTION "What sort of exclamation should we cry?"

ID [_\p{L}][_\p{L}\p{N}]*
LINE_BREAK VSPACE | PARAGRAPH | NEWLINE
%%

ID { printf("Hello, ID world\n"); }
LINE_BREAK {}

%%
const int FOO = 0x0042;
