# The lexer's per-character checks: after a ${...} operator, a later
# #, %, /, ^ or , is part of the word, and a backslash before ", `, $ or
# \ in a compound assignment inside "$(...)" escapes it.
p() { printf '<%s>' "$@"; echo; }
unset u
p "${u:-a/$'x'}" "${u:-a#$'x'}" "${u:-a%$'x'}" "${u:-a,$'x'}" "${u:-a^$'x'}"
p "${u+a/$'x'}" "${u=a#$'\t'}"
echo "$(a=(\"x\" y); echo "${a[0]}|${a[1]}")"
echo "$(a=(\`x\` \$y); echo "${a[0]}|${a[1]}")"
echo "$(a=(\\ z); echo "${a[0]}|${a[1]}")"
