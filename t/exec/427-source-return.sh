# return in a sourced file
mk() { while IFS= read -r l; do printf '%s\n' "$l"; done; }
mk > /tmp/r.sh <<'END'
echo before-return
return 4
echo after-return
END
. /tmp/r.sh
echo "status $?"
mk > /tmp/r0.sh <<'END'
false
return
END
. /tmp/r0.sh
echo "bare return: $?"
mk > /tmp/rf.sh <<'END'
inner() { return 9; }
inner
echo "inner returned $?"
END
. /tmp/rf.sh
echo "file status $?"
f() { . /tmp/r.sh; echo "in f after source: $?"; return 1; }
f
echo "f: $?"
mk > /tmp/rloop.sh <<'END'
for i in 1 2 3; do
  if [ $i = 2 ]; then return 6; fi
  echo "loop $i"
done
END
. /tmp/rloop.sh
echo "loop status $?"
return 2>&1
echo "top return: $?"
