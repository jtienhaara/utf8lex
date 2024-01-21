%{
#define EXCLAMATION "Woohoo!"
%}

    #define QUESTION "What sort of exclamation should we cry?"

ID [_\p{L}][_\p{L}\p{N}]*
LINE_BREAK VSPACE | PARAGRAPH | NEWLINE
%%

ID { printf("Hello, ID world\n"); }
LINE_BREAK {}
[\+\-]?[1-9][0-9]*(\.[1-9][0-9]*)(e[\+\-][1-9][0-9]*)? {
    printf("That was quite the number.\n");
    }

%%
const int FOO = 0x0042;
