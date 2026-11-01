# backslash-newline in here-document bodies and delimiters
v=V
while IFS= read -r l; do echo "[$l]"; done <<EOF2
joined \
here $v
not \joined
double \
EOF2
while IFS= read -r l; do echo "{$l}"; done <<'EOF2'
kept \
here $v
EOF2
while IFS= read -r l; do echo "<$l>"; done <<EOF2
dollar at end $
dollar-space $ x
dollar-brace-less ${v}x
EOF2
