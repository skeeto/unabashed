# Text that eval parses to its very end without a final newline (an odd
# run of trailing backslashes, a backslash-newline, a comment ending in
# a backslash) leaves bash's parser after an end-of-input token. The
# next command read from any input then starts outside command
# position: no reserved words or aliases, and a simple command it
# begins has line -1. Errors at a line below 1 drop "line N: ".
eval 'echo "a\"'
eval 'echo "a'
eval '
echo "a'
eval 'echo $(echo "a\")'
eval "echo 'a"
eval 'echo ${a'
eval 'echo a\'
eval 'echo "a\"'
eval 'echo "a\"'
eval 'echo a\'
echo $LINENO; echo $LINENO
eval 'echo a\'
eval '
echo "a\"'
t() { printf '%s\n' "$1" >/tmp/t.sh; bash /tmp/t.sh; echo "status $?"; bash -c "$1"; echo "status $?"; }
b='\'
t "eval 'echo a$b'
if :; then echo if; fi"
t "eval 'echo a$b'
{ echo brace; }"
t "eval 'echo a$b'
case x in x) echo case;; esac"
t "eval 'echo a$b'
x=(1 2)"
t "eval 'echo a$b'
! false; echo \$?"
t "eval 'echo a$b'
[[ a ]]"
t "eval 'echo a$b'
((1))"
t "eval 'echo a$b'
time echo t"
t "eval 'echo a$b'
nosuch"
t "eval 'echo a$b'
x=1 y=2 echo \$LINENO \$x; v='a  b'"
t "v='a  b'; eval 'echo a$b'
x=\$v; echo \"\$x\" \$LINENO"
t "eval 'echo a$b'
2>/dev/null echo r \$LINENO; echo r \$LINENO"
t "eval 'echo a$b'
(echo paren \$LINENO)"
t "eval 'echo a$b'
x=\$(echo cs \$LINENO); echo \"\$x\" \`echo bq \$LINENO\`"
t "eval 'echo a$b'
f() { echo fn \$LINENO; }; f"
t "eval 'echo a$b'

echo blank \$LINENO"
t "eval 'echo a$b'
# comment
echo comment \$LINENO"
t "eval 'echo a #$b'
echo comment2 \$LINENO"
t "eval 'echo a$b
'
echo contnl \$LINENO"
t "eval 'echo a$b$b$b'
echo three \$LINENO"
t "eval 'echo a$b$b'
echo even \$LINENO"
t "eval 'echo a$b'
eval 'echo \$LINENO
echo \$LINENO'"
t "eval 'echo a$b'
eval 'if'"
t "eval 'echo a$b'
eval 'echo (('"
t "eval 'echo a$b'; eval ''
echo empty \$LINENO"
t "shopt -s expand_aliases; alias e=echo; eval 'echo a$b'
e alias \$LINENO"
t "c='echo a$b'; eval 'eval \"\$c\"
echo inner \$LINENO
if :; then echo if; fi'"
t "c='echo a$b'; eval 'eval \"\$c\"'
echo outer \$LINENO"
t "g() { eval 'echo a$b'; }
g; echo semi \$LINENO
echo func \$LINENO"
t "for i in 1 2; do eval 'echo a$b'; done
echo loop \$LINENO"
t "{ eval 'echo a$b'; echo inbrace \$LINENO; }
echo after \$LINENO"
t "command eval 'echo a$b'
echo command \$LINENO"
t "(eval 'echo a$b')
echo sub \$LINENO"
t "x=\$(eval 'echo a$b'); echo \"\$x\"
echo cs2 \$LINENO"
t "eval 'echo a$b' | cat
echo pipe \$LINENO"
t "printf 'echo src$b$b' >/tmp/s
. /tmp/s
if :; then echo src; fi"
t "c='echo a$b'; printf 'eval \"\$c\"\n' >/tmp/s
. /tmp/s
echo src2 \$LINENO"
t "trap 'echo t$b' USR1
kill -USR1 \$\$
echo trap \$LINENO"
t "c='echo t$b'; trap 'eval \"\$c\"' USR1
eval 'echo a$b'; kill -USR1 \$\$
echo trap2 \$LINENO"
t "trap 'echo d$b' DEBUG
echo debug \$LINENO"
t "x=\${ eval 'echo a$b';}; echo \"\$x\"
echo funsub \$LINENO"
t "x=\${ eval 'echo a$b'; } y=\${ echo b; }; echo \"\$x\$y\"
echo funsub2 \$LINENO"
t "c='echo a$b'; printf '%s\n' 1 >/tmp/m; mapfile -C 'eval \"\$c\"; :' -c 1 arr </tmp/m
if :; then echo mapfile; fi"
