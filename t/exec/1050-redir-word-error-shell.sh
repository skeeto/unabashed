# An expansion error in a redirection's word only fails the redirection
# where bash applies it in a child it forked first: a program's (also
# through command, which skips functions), a subshell's, a null
# command's of fd 0 or with a {var}. For a builtin, a function, a
# compound command, exec or other assignments alone, the error ends
# the shell, or abandons the command, as in any other word.
for w in '${x?msg}' '$u' '$((1+))' '${#x:1}'; do
  for c in 'echo hi >W' ': >W' 'f(){ echo f; }; f >W' '{ echo g; } >W' \
           'while false; do :; done >W' 'exec >W' 'y=1 >W' 'builtin echo >W' \
           'command echo >W' 'command exec cat </dev/null >W' \
           'f(){ echo hi >W; echo in $?; }; f' \
           'cat </dev/null >W' 'command cat </dev/null >W' 'nosuch >W' \
           'cat(){ :; }; command cat </dev/null >W' 'y=1 <W' '{v}>W' \
           '( echo in ) >W' 'f() ( echo in ) >W; f' \
           '(echo hi >W; echo in $?)' 'v=$(echo hi >W; echo in $?); echo "[$v]"'; do
    s="set -u; ${c//W/$w}; echo st \$?"
    echo "== $s"
    bash -c "$s" 2>&1; echo "exit $?"
    bash -o posix -c "$s" 2>&1; echo "posix exit $?"
  done
done
# a script file goes on with its next command, as after any other word
printf '%s\n' 'echo hi >${#x:1}; echo no' 'echo next $?' \
       'cat </dev/null >${#x:1}; echo st $?' >script
bash script 2>&1; echo "exit $?"
