# command/builtin wrappers around eval and source
mk() { while IFS= read -r l; do printf '%s\n' "$l"; done; }
command eval 'echo via-command'
builtin eval 'echo via-builtin'
mk > /tmp/w.sh <<'END'
echo "sourced $*"
END
builtin source /tmp/w.sh b1
command . /tmp/w.sh c1
command eval 'if' 2>&1
echo "st=$?"
echo alive
