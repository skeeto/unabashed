set -u
[[ -v nope ]] || echo "not set"
[ -z "${nope-}" ] && echo empty
declare v
(echo "declared-noval=$v"; echo after-a); echo "st=$?"
f() { local l; echo "local=$l"; echo after-b; }
(f); echo "st=$?"
(echo "${nope?custom message}"; echo after-c); echo "st=$?"
(echo "${nope:?}"; echo after-d); echo "st=$?"
e=
(echo "${e:?is empty}"; echo after-e); echo "st=$?"
(n=nope; echo "${!n}"; echo after-f); echo "st=$?"
(echo "${nope@Q}"; echo after-g); echo "st=$?"
set +u
echo "plain=[$nope]"
set -o nounset
(echo "$nope"); echo "st=$?"
