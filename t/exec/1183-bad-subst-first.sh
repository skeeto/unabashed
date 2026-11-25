# A ${...} whose name has no operator after it is a bad substitution
# before its subscript, value or indirection is read, as bash checks
# the name first: no arithmetic error, unbound variable or command
# substitution comes first. ${x:} is one too, before set -u looks.
# The message names the text bash expanded: a double-quoted part as it
# was extracted (\" unescaped), a declaration's whole assignment word;
# a double-quoted ${name[ whose subscript does not close is "no
# closing }" in the whole word. Under set -e these errors, which bash
# reports with report_error, end the shell (arithmetic errors do not).
# Each line is a script; ;; stands for a newline.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "${c//;;/$'\n'}"
    echo "status $?"
done <<'EOF'
declare -A map; echo ${map[a b]&}
x=1; echo ${x!}
set -u; echo ${x!}
set -u; echo ${r&}
set -u; echo ${u[0]&}
set -u; a=(1); echo ${a[$u]&}
echo ${a[1/0]&} ${a[1/0]!} ${a[1/0]x}
echo ${#a[1/0]&}
echo ${a[$(echo ran)]&}
echo ${a[1/0]:-x}
set -u; echo ${u:}
set -u; echo ${u@}
set -u; echo ${10x}
unset arr; echo ${arr[-1]:}
echo ${!x*} ${!1*}
i=1; echo "$((i))${x.y^^}\""
x=1; echo "a${x!}\"b"
x=1; echo "${x!}`echo \"q\"`"
x=1; echo "a${x!}\\b"
x=1; echo ${x.y}\"a
x=1; declare x=${x!}
f() { local -i x=${x!}; }; f
x=${x!}$y
echo "${x[}"
echo "a${x[:1}b" "${y[}"
echo "${!x[}$y"
echo "${x[1}]"
echo "${#x[}"
echo ${x[}
set -e;; echo ${r&};; echo after
set -e;; [ ${r&} ] || echo or;; echo after
set -e;; if echo "${x[}"; then :; fi;; echo after
set -e;; a=(); echo ${a[-1]};; echo after
set -e;; echo ${!u};; echo after
set -e;; trap 'echo exit $?' EXIT; echo ${r&};; echo after
set -e;; echo $((1/0));; echo after
set -e;; s=abc; echo ${s:1:-5};; echo after
set -e;; (echo ${r&});; echo after
set -e;; eval 'echo ${r&}' || echo or;; echo after
EOF
