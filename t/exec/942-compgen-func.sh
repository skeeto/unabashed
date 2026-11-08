# compgen -F, -C and -V
s() {
    echo "args: $# <$1> <$2> <$3>"
    echo "line <$COMP_LINE> point <$COMP_POINT> cword <$COMP_CWORD> words ${#COMP_WORDS[@]}"
    COMPREPLY=(r1 r2)
    return 3
}
false; compgen -F s wd; echo "st $?"
declare -p COMP_LINE COMP_POINT COMP_WORDS COMP_CWORD COMPREPLY 2>&1 | wc -l
one() { COMPREPLY=one; }; compgen -F one x
sparse() { COMPREPLY=([5]=five [2]=two); }; compgen -F sparse x
assoc() { declare -gA COMPREPLY=([k]=v); }; compgen -F assoc x; echo "st $?"
unset COMPREPLY
retry() { COMPREPLY=(no); return 124; }; compgen -F retry -W w x; echo "st $?"
nf() { COMPREPLY=(no); return 127; }; compgen -F nf -W x x; echo "st $?"
loc() { local COMPREPLY=(lost); }; compgen -F loc x; echo "st $?"
compgen -F nofunc -W x x; echo "st $?"
compgen -F s -W w -X 'r1' -P p x 2>/dev/null
compgen -C echo w
compgen -C 'printf "%s\n" "a\\" b "" "  l3  "; :' w | cat -A
compgen -C 'echo # comment' w; echo "st $?"
compgen -C 'printf "x\\\ny\nz"' w | cat -A
arr=(old); compgen -V arr -W 'b1 b2 c' b; echo "st $? ${arr[*]}"
compgen -V arr -W q z; echo "st $? ${#arr[@]}"
declare -A aa; compgen -V aa -W q q; echo "st $?"
unalias -a; alias fee=one fi=two fo=three fum=four
compgen -a -X 'fo*' -V vv -P 'unalias -- ' f; printf '%s\n' "${vv[@]}"
