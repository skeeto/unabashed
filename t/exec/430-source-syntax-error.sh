# A syntax error in a sourced file
mk() { while IFS= read -r l; do printf '%s\n' "$l"; done; }
mk > /tmp/bad.sh <<'END'
echo first-line
if then
echo unreachable
END
. /tmp/bad.sh
echo "status $?"
mk > /tmp/bad2.sh <<'END'
echo ok
echo "unterminated
END
source /tmp/bad2.sh
echo "status $?"
mk > /tmp/bad3.sh <<'END'
echo a
)
echo b
END
. /tmp/bad3.sh
echo "status $?"
echo alive
