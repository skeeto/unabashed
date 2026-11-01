# here-document bodies: quotes are literal, parameter operators work
set -- p1 p2
unset u
v=val
while IFS= read -r l; do echo "[$l]"; done <<EOF2
at: $@ star: $* count: $#
default: ${u:-dflt} alt: ${v:+alt} len: ${#v}
dq: "$v" sq: '$v' mixed: "a'b"
brace: ${v%l} ${v#v}
dollar-sq: $'x' locale: $"y"
lone \ backslash and \" and \'
tilde ~ stays
EOF2
