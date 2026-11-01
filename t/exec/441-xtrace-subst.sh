# xtrace prefixes inside command substitution and subshells
set -x
x=$(echo inner)
(echo sub)
y=$(echo $(echo deep))
eval 'echo evald'
z=${ echo funsub; }
set +x
echo done 2>&1
