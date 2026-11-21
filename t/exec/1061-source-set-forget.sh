# A sourced file with arguments keeps the parameters its own set made
# only if no function call or source followed that set (bash's
# pop_dollar_vars and set_dollar_vars_unchanged clear the record).
# BASH_ARGV's bottom is set up by the first source outside a function,
# and not by a read inside one.
ra() { echo "${BASH_ARGV[*]} / ${BASH_ARGC[*]}"; }
set -- x y; f0() { ra; }; f0 q
p() { :; }
r() { printf '<%s>' "$@"; echo; }
echo ': nothing' >s0.sh
echo 'set -- in1 in2; p x' >s1.sh
echo 'p x; set -- in1 in2' >s2.sh
echo 'set -- in1 in2; source ./s0.sh' >s3.sh
echo 'set -- in1 in2; source ./s0.sh a1' >s4.sh
echo 'set -- in1 in2; x=${ p; }' >s5.sh
echo 'set -- in1 in2; x=${ :; }' >s6.sh
echo 'source ./s7b.sh' >s7.sh
echo 'set -- in1 in2' >s7b.sh
echo 'set -- in1 in2 in3; p; shift' >s8.sh
echo 'set -- in1 in2 in3; shift; p' >s9.sh
echo 'p; set -- in1 in2 in3; shift' >s10.sh
echo 'set -- in1 in2; (p); p | :; echo $(p)' >s11.sh
echo 'set -- in1 in2; p; return' >s12.sh
echo 'set -- in1 in2; trap p USR1; kill -USR1 $$; trap - USR1' >s13.sh
for f in s1 s2 s3 s4 s5 s6 s7 s8 s9 s10 s11 s12 s13; do
    set -- a b c; source ./$f.sh t1 t2; printf '%s args: ' $f; r "$@"
    set -- a b c; source ./$f.sh; printf '%s none: ' $f; r "$@"
    f1() { source ./$1.sh t1 t2; printf '%s func: ' $1; r "$@"; }
    set -- a b c; f1 $f; printf '%s after: ' $f; r "$@"
done
shopt -s extdebug
r() { printf '<%s>' "$@"; echo " ${BASH_ARGV[*]} / ${BASH_ARGC[*]}"; }
set -- a b c; source ./s1.sh t1 t2; r "$@"
set -- a b c; source ./s2.sh t1 t2; r "$@"
set -- a b c; source ./s3.sh t1 t2; r "$@"
