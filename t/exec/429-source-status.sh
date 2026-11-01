# Exit status of source: last command, empty file, exit inside
mk() { while IFS= read -r l; do printf '%s\n' "$l"; done; }
mk > /tmp/st.sh <<'END'
echo running
false
END
. /tmp/st.sh
echo "last false: $?"
: > /tmp/empty.sh
false
. /tmp/empty.sh
echo "empty file: $?"
mk > /tmp/ex.sh <<'END'
echo exiting
exit 12
END
(. /tmp/ex.sh; echo no)
echo "exit in subshell source: $?"
x=$(. /tmp/ex.sh)
echo "x=$x st=$?"
. /tmp/ex.sh
echo never
