# LINENO and error-message line numbers inside sourced files
mk() { while IFS= read -r l; do printf '%s\n' "$l"; done; }
mk > /tmp/ln.sh <<'END'
echo "line $LINENO"

echo "line $LINENO"
nosuchcmd_abc
f() { echo "f line $LINENO"; }
END
. /tmp/ln.sh
echo "back $LINENO"
f
eval 'nosuchcmd_def'
mk > /tmp/ln2.sh <<'END'
echo ${undefined_v:?message here}
echo not
END
. /tmp/ln2.sh
echo "st=$?"
