# for lists built from brace, command substitution and arithmetic
for x in {1..3}; do printf '%s ' $x; done; echo
for x in a{b,c}d; do printf '%s ' $x; done; echo
for x in $(echo p q; echo r); do printf '<%s>' $x; done; echo
for x in "$(echo p q)"; do printf '<%s>' "$x"; done; echo
for x in $((1+1)) $((2*3)); do printf '%s ' $x; done; echo
for x in ~ '~' ~/sub; do printf '%s ' "$x"; done; echo
for x in a\ b 'c'd "e"'f'; do printf '[%s]' "$x"; done; echo
for x in $(exit 3); do echo never; done; echo "cmdsub status: $?"
