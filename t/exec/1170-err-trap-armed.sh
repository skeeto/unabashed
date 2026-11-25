# ERR runs after a command only if its action was set before that
# command began: one that a function, eval, source or a lastpipe stage
# sets runs for what follows inside, not again for itself
h() { trap 'echo t' ERR; false; }
h; echo "s=$?"
trap - ERR
(eval "trap 'echo e' ERR; false"; echo "s=$?")
echo "trap 'echo src' ERR; false" >s.sh
(. ./s.sh; echo "s=$?"; false)
(shopt -s lastpipe; true | { trap 'echo lp' ERR; false; }; echo "s=$?")
(f() { trap 'echo t' ERR; (( 0 )); }; f; echo "s=$?")
(f() { trap 'echo t' ERR; false; }; f >&8; echo "s=$?")
(trap "trap 'echo dbg-set' ERR" DEBUG; false; false)
(false ${ trap 'echo t' ERR; }; false)
# source of a file it cannot read ends the shell under set -e before
# ERR could run, as any other file error
(trap 'echo err' ERR; . ./nx; echo "s=$?")
(trap 'echo err' ERR; set -e; . ./nx; echo no); echo "st=$?"
