# set +B turns brace expansion off; brace expansion results in
# assignments and other non-argument contexts.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show {a,b}
set +B
show {a,b} {1..3}
echo $-
set -B
show {a,b}
v={a,b}
show "$v"
v=x{1..2}
show "$v"
for i in {1..3}; do printf '%s ' "$i"; done; echo
case b in {a,b}) echo matched;; *) echo literal;; esac
case '{a,b}' in {a,b}) echo literal;; *) echo no;; esac
show a{b,c}=d
show {a,b}=x
export e{1,2}=v
show "${e1-unset}" "${e2-unset}"
