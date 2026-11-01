# unquoted here-documents expand parameters, commands and arithmetic
name=world
while IFS= read -r l; do echo "[$l]"; done <<EOF2
hello $name
${name}s and $(echo cmd) and `echo bq`
sum $((1+2))
escaped \$name \`x\` \ back
quotes "kept" 'too' $name
line \
joined
trailing backslash \x
EOF2
echo status $?
