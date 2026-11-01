echo start
f() { echo "${1?need an arg}"; echo "in f"; }
(f; echo after-f); echo "st=$?"
(: "${u?}"; echo after-u); echo "st=$?"
(v=; : "${v:?empty or unset}"; echo after-v); echo "st=$?"
echo "${top?top level}"
echo notreached
