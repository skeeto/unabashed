# A trap action keeps PIPESTATUS, as it keeps $?
trap 'false | false | false' USR1
true | (exit 3)
kill -USR1 $$
echo "$? ${PIPESTATUS[*]}"
trap 'PIPESTATUS=(7 8)' USR2
kill -USR2 $$
echo "${PIPESTATUS[*]}"
f() { (exit 4) | true; }
trap 'f' USR1
kill -USR1 $$; echo "${PIPESTATUS[*]}"
