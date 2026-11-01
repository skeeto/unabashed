# Nested sourcing and sourcing from functions
mk() { while IFS= read -r l; do printf '%s\n' "$l"; done; }
mk > /tmp/a.sh <<'END'
echo "a start $1"
. /tmp/b.sh from-a
echo "a end $1"
END
mk > /tmp/b.sh <<'END'
echo "b with $1"
return 3
END
. /tmp/a.sh top
echo "status $?"
f() { local v=fv; . /tmp/c.sh; echo "f sees $v $w"; }
mk > /tmp/c.sh <<'END'
echo "c sees $v"
v=changed
local w=localw
END
f
echo "global v=${v-unset} w=${w-unset}"
mk > /tmp/self.sh <<'END'
n=$((n+1))
if [ $n -lt 4 ]; then . /tmp/self.sh; fi
echo "depth $n"
END
n=0
. /tmp/self.sh
