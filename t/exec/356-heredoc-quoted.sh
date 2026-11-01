# quoted delimiters suppress all expansion
name=world
while IFS= read -r l; do echo "[$l]"; done <<'EOF2'
hello $name $(echo no) `no` $((1+2))
back\slash \
next
EOF2
while IFS= read -r l; do echo "{$l}"; done <<"EOF3"
dq $name
EOF3
while IFS= read -r l; do echo "<$l>"; done <<E\OF
part $name
EOF
while IFS= read -r l; do echo "($l)"; done <<'A B'
spaced $name
A B
while IFS= read -r l; do echo "|$l|"; done <<E"O"F
mixed $name
EOF
