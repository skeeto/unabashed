# ${name?word} and ${name:?word} expand the word for the message as an
# unquoted string, as bash's expand_string does, even within double
# quotes: quotes are removed, the result splits on IFS and is joined
# by single spaces, and nothing is globbed or brace-expanded.
v='a  b'
bash -c 'echo "${u?a\ b}"' 2>&1
bash -c 'echo "${y:?\*}"' 2>&1
bash -c 'echo "${y:?"a\*"}"' 2>&1
bash -c 'echo "${y:?'"'a b'"'}"' 2>&1
bash -c 'echo ${u?*}' 2>&1
bash -c 'echo "${y?\a}"' 2>&1
bash -c 'echo ${x:?$(printf "a\nb")}' 2>&1
bash -c 'echo ${x:?"$(printf "a\nb")"}' 2>&1
v=$v bash -c 'echo ${x:?$v}' 2>&1
v=$v bash -c 'echo ${x:?"$v"}' 2>&1
v=$v bash -c 'IFS=; echo ${x:?$v}' 2>&1
bash -c 'echo ${x:?a{1,2}b}' 2>&1
echo "end $?"
