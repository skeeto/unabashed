== stdout 3295
-- declare -A map; echo ${map[a b]&}
bash: line 1: ${map[a b]&}: bad substitution
status 1
-- x=1; echo ${x!}
bash: line 1: ${x!}: bad substitution
status 1
-- set -u; echo ${x!}
bash: line 1: ${x!}: bad substitution
status 1
-- set -u; echo ${r&}
bash: line 1: ${r&}: bad substitution
status 1
-- set -u; echo ${u[0]&}
bash: line 1: ${u[0]&}: bad substitution
status 1
-- set -u; a=(1); echo ${a[$u]&}
bash: line 1: ${a[$u]&}: bad substitution
status 1
-- echo ${a[1/0]&} ${a[1/0]!} ${a[1/0]x}
bash: line 1: ${a[1/0]&}: bad substitution
status 1
-- echo ${#a[1/0]&}
bash: line 1: ${#a[1/0]&}: bad substitution
status 1
-- echo ${a[$(echo ran)]&}
bash: line 1: ${a[$(echo ran)]&}: bad substitution
status 1
-- echo ${a[1/0]:-x}
bash: line 1: 1/0: division by 0 (error token is "0")
status 1
-- set -u; echo ${u:}
bash: line 1: ${u:}: bad substitution
status 1
-- set -u; echo ${u@}
bash: line 1: u: unbound variable
status 127
-- set -u; echo ${10x}
bash: line 1: ${10x}: bad substitution
status 1
-- unset arr; echo ${arr[-1]:}
bash: line 1: arr: bad array subscript
bash: line 1: ${arr[-1]:}: bad substitution
status 1
-- echo ${!x*} ${!1*}
bash: line 1: ${!1*}: bad substitution
status 1
-- i=1; echo "$((i))${x.y^^}\""
bash: line 1: $((i))${x.y^^}": bad substitution
status 1
-- x=1; echo "a${x!}\"b"
bash: line 1: a${x!}"b: bad substitution
status 1
-- x=1; echo "${x!}`echo \"q\"`"
bash: line 1: ${x!}`echo "q"`: bad substitution
status 1
-- x=1; echo "a${x!}\\b"
bash: line 1: a${x!}\\b: bad substitution
status 1
-- x=1; echo ${x.y}\"a
bash: line 1: ${x.y}\"a: bad substitution
status 1
-- x=1; declare x=${x!}
bash: line 1: x=${x!}: bad substitution
status 1
-- f() { local -i x=${x!}; }; f
bash: line 1: x=${x!}: bad substitution
status 1
-- x=${x!}$y
bash: line 1: ${x!}$y: bad substitution
status 1
-- echo "${x[}"
bash: line 1: bad substitution: no closing `}' in "${x[}"
status 1
-- echo "a${x[:1}b" "${y[}"
bash: line 1: bad substitution: no closing `}' in "a${x[:1}b"
status 1
-- echo "${!x[}$y"
bash: line 1: bad substitution: no closing `}' in "${!x[}$y"
status 1
-- echo "${x[1}]"
bash: line 1: bad substitution: no closing `}' in "${x[1}]"
status 1
-- echo "${#x[}"
bash: line 1: ${#x[}: bad substitution
status 1
-- echo ${x[}
bash: line 1: ${x[}: bad substitution
status 1
-- set -e;; echo ${r&};; echo after
bash: line 2: ${r&}: bad substitution
status 1
-- set -e;; [ ${r&} ] || echo or;; echo after
bash: line 2: ${r&}: bad substitution
status 1
-- set -e;; if echo "${x[}"; then :; fi;; echo after
bash: line 2: bad substitution: no closing `}' in "${x[}"
status 1
-- set -e;; a=(); echo ${a[-1]};; echo after
bash: line 2: a: bad array subscript
status 1
-- set -e;; echo ${!u};; echo after
bash: line 2: u: invalid indirect expansion
status 1
-- set -e;; trap 'echo exit $?' EXIT; echo ${r&};; echo after
bash: line 2: ${r&}: bad substitution
exit 1
status 1
-- set -e;; echo $((1/0));; echo after
bash: line 2: 1/0: division by 0 (error token is "0")
after
status 0
-- set -e;; s=abc; echo ${s:1:-5};; echo after
bash: line 2: -5: substring expression < 0
after
status 0
-- set -e;; (echo ${r&});; echo after
bash: line 2: ${r&}: bad substitution
status 1
-- set -e;; eval 'echo ${r&}' || echo or;; echo after
bash: line 2: ${r&}: bad substitution
or
after
status 0

== stderr 0

== merged 3295
-- declare -A map; echo ${map[a b]&}
bash: line 1: ${map[a b]&}: bad substitution
status 1
-- x=1; echo ${x!}
bash: line 1: ${x!}: bad substitution
status 1
-- set -u; echo ${x!}
bash: line 1: ${x!}: bad substitution
status 1
-- set -u; echo ${r&}
bash: line 1: ${r&}: bad substitution
status 1
-- set -u; echo ${u[0]&}
bash: line 1: ${u[0]&}: bad substitution
status 1
-- set -u; a=(1); echo ${a[$u]&}
bash: line 1: ${a[$u]&}: bad substitution
status 1
-- echo ${a[1/0]&} ${a[1/0]!} ${a[1/0]x}
bash: line 1: ${a[1/0]&}: bad substitution
status 1
-- echo ${#a[1/0]&}
bash: line 1: ${#a[1/0]&}: bad substitution
status 1
-- echo ${a[$(echo ran)]&}
bash: line 1: ${a[$(echo ran)]&}: bad substitution
status 1
-- echo ${a[1/0]:-x}
bash: line 1: 1/0: division by 0 (error token is "0")
status 1
-- set -u; echo ${u:}
bash: line 1: ${u:}: bad substitution
status 1
-- set -u; echo ${u@}
bash: line 1: u: unbound variable
status 127
-- set -u; echo ${10x}
bash: line 1: ${10x}: bad substitution
status 1
-- unset arr; echo ${arr[-1]:}
bash: line 1: arr: bad array subscript
bash: line 1: ${arr[-1]:}: bad substitution
status 1
-- echo ${!x*} ${!1*}
bash: line 1: ${!1*}: bad substitution
status 1
-- i=1; echo "$((i))${x.y^^}\""
bash: line 1: $((i))${x.y^^}": bad substitution
status 1
-- x=1; echo "a${x!}\"b"
bash: line 1: a${x!}"b: bad substitution
status 1
-- x=1; echo "${x!}`echo \"q\"`"
bash: line 1: ${x!}`echo "q"`: bad substitution
status 1
-- x=1; echo "a${x!}\\b"
bash: line 1: a${x!}\\b: bad substitution
status 1
-- x=1; echo ${x.y}\"a
bash: line 1: ${x.y}\"a: bad substitution
status 1
-- x=1; declare x=${x!}
bash: line 1: x=${x!}: bad substitution
status 1
-- f() { local -i x=${x!}; }; f
bash: line 1: x=${x!}: bad substitution
status 1
-- x=${x!}$y
bash: line 1: ${x!}$y: bad substitution
status 1
-- echo "${x[}"
bash: line 1: bad substitution: no closing `}' in "${x[}"
status 1
-- echo "a${x[:1}b" "${y[}"
bash: line 1: bad substitution: no closing `}' in "a${x[:1}b"
status 1
-- echo "${!x[}$y"
bash: line 1: bad substitution: no closing `}' in "${!x[}$y"
status 1
-- echo "${x[1}]"
bash: line 1: bad substitution: no closing `}' in "${x[1}]"
status 1
-- echo "${#x[}"
bash: line 1: ${#x[}: bad substitution
status 1
-- echo ${x[}
bash: line 1: ${x[}: bad substitution
status 1
-- set -e;; echo ${r&};; echo after
bash: line 2: ${r&}: bad substitution
status 1
-- set -e;; [ ${r&} ] || echo or;; echo after
bash: line 2: ${r&}: bad substitution
status 1
-- set -e;; if echo "${x[}"; then :; fi;; echo after
bash: line 2: bad substitution: no closing `}' in "${x[}"
status 1
-- set -e;; a=(); echo ${a[-1]};; echo after
bash: line 2: a: bad array subscript
status 1
-- set -e;; echo ${!u};; echo after
bash: line 2: u: invalid indirect expansion
status 1
-- set -e;; trap 'echo exit $?' EXIT; echo ${r&};; echo after
bash: line 2: ${r&}: bad substitution
exit 1
status 1
-- set -e;; echo $((1/0));; echo after
bash: line 2: 1/0: division by 0 (error token is "0")
after
status 0
-- set -e;; s=abc; echo ${s:1:-5};; echo after
bash: line 2: -5: substring expression < 0
after
status 0
-- set -e;; (echo ${r&});; echo after
bash: line 2: ${r&}: bad substitution
status 1
-- set -e;; eval 'echo ${r&}' || echo or;; echo after
bash: line 2: ${r&}: bad substitution
or
after
status 0

== status 0
