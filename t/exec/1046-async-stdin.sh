# Without job control an asynchronous command reads /dev/null unless
# its standard input is redirected, as bash decides it: by the input
# redirections of the compound command running it in this shell (any
# input one, on any descriptor), by an input pipe, or by its own. A
# pipeline stage's or job's own redirections do not count, a ( list )
# goes by its own alone, and an asynchronous pipeline keeps the input.
# (bash leaves its flag set after a compound command, so each line ends
# with one that clears it.)
exec 2>&1 <<<outer
t() { printf '%s: ' "$1"; }
z() { echo; }
t 1; { cat & wait; } <<<hi | wc -c; { z; } 2>&1
t 2; { cat & wait; } <<<hi & wait; z; { :; } 2>&1
t 3; { cat & wait; } <<<hi | cat | cat; z; { :; } 2>&1
t 4; { ( cat & wait ); } <<<hi; z; { :; } 2>&1
t 5; { ( cat & wait ) | cat; } <<<hi; z; { :; } 2>&1
t 6; { { cat & wait; } 2>/dev/null; } <<<hi; z; { :; } 2>&1
t 7; { cat & wait; } 3<<<hi; { :; } 2>&1
t 8; exec <<<outer; cat </dev/stdin & wait; { :; } 2>&1
t 9; exec <<<outer; cat 3<<<x & wait; { :; } 2>&1
t 10; exec <<<outer; cat | cat & wait; { :; } 2>&1
t 11; { cat & wait; } <<<hi; { :; } 2>&1
t 12; ( cat & wait ) <<<hi; { :; } 2>&1
t 13; echo x | { cat & wait; } 2>/dev/null; { :; } 2>&1
t 14; echo x | ( cat & wait ) 2>/dev/null; { :; } 2>&1
t 15; f() { cat & wait; }; f <<<hi; z; { :; } 2>&1
t 16; { cat & wait; } {fd}<<<hi; z; { :; } 2>&1
t 17; shopt -s lastpipe; echo x | { cat & wait; }; z; shopt -u lastpipe
t 18; { { cat & wait; } <<<hi | cat; } <<<x; { :; } 2>&1
