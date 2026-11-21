# What & reads in bash's child for a ( list ), a pipeline stage or a
# job: a ( list ) starts with only its own redirections of standard
# input, or a pipe in, deciding it (not the shell's state, nor an
# enclosing subshell's); another compound command run as a stage or
# job keeps the shell's, a pipe in setting it, and its redirections
# there change nothing; a coproc's reads its pipe unless a ( list )
echo input > in.txt
echo file > f.txt
t() { printf '%s [%s]\n' "$1" "$(tr '\n' ' ' <o.txt)"; }
exec < in.txt; ( cat & wait ) < f.txt >o.txt; t 1
exec < in.txt; echo piped | ( cat & wait ) >o.txt; t 2
exec < in.txt; echo piped | ( cat & wait ) | cat >o.txt; t 3
exec < in.txt; ( ( cat & wait ) ) < f.txt >o.txt; t 4
exec < in.txt; echo piped | ( ( cat & wait ) ) >o.txt; t 5
exec < in.txt; { cat & wait; } <<<hi | cat >o.txt; t 6
exec < in.txt; { cat & wait; } <<<hi >o.txt & wait; t 7
exec < in.txt; ( echo piped | { cat & wait; } >o.txt ); t 8
exec < in.txt; coproc { cat & wait; }
echo cp >&${COPROC[1]}; exec {COPROC[1]}>&-; cat <&${COPROC[0]} >o.txt; wait; t 9
echo "== after { :; } < /dev/null"
{ :; } < /dev/null
exec < in.txt; cat >o.txt & wait; t s1
exec < in.txt; ( cat & wait ) >o.txt; t s2
exec < in.txt; ( ( cat & wait ) ) >o.txt; t s3
exec < in.txt; ( exec <f.txt; cat & wait ) >o.txt; t s4
exec < in.txt; ( cat & wait ) | cat >o.txt; t s5
exec < in.txt; { cat & wait; } | cat >o.txt; t s6
exec < in.txt; { cat & wait; } >o.txt & wait; t s7
exec < in.txt; { cat & wait; } <<<hi | cat >o.txt; t s8
exec < in.txt; f() ( cat & wait ); f >o.txt; t s9
exec < in.txt; f <f.txt >o.txt; t s10
exec < in.txt; ( cat & wait ) <<<here >o.txt; t s11
exec < in.txt; coproc ( cat & wait )
echo cp >&${COPROC[1]}; exec {COPROC[1]}>&-; cat <&${COPROC[0]} >o.txt; wait; t s12
