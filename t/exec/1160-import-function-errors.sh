# A function in the environment that does not parse as one definition
# alone is reported, as bash's parse of it named for the function, is
# not defined, makes $? 1, and is passed on to programs as it came. A
# new shell parses it before any option of its own: one written with
# extglob on fails where extglob is off.
shopt -s extglob
f() { [ *([0-9]) = x ]; echo in f; }
export -f f
bash -c 'echo "\$?=$?"; f; echo "st=$?"'
export -nf f
env 'BASH_FUNC_g%%=() { echo hi; }; echo evil' bash -c 'echo "\$?=$?"; g'
env 'BASH_FUNC_g%%=() { echo hi; ' bash -c 'g'
env 'BASH_FUNC_g%%=() { echo hi
  ( ; }' bash -c 'g'
env 'BASH_FUNC_g%%=() { echo $(( ; }' bash -c 'g'
env 'BASH_FUNC_g%%=() { if
}' 'BASH_FUNC_h%%=() { echo "h $LINENO"; }' bash -c 'h; g'
env 'BASH_FUNC_a/b%%=() { echo hi; }' bash -c 'echo "\$?=$?"'
echo passed on
env 'BASH_FUNC_g%%=() { if; }' bash -c 'bash -c "echo \$?"; printenv BASH_FUNC_g%%'
env 'BASH_FUNC_g%%=()' bash -c 'echo "quiet $?"'
