# Command substitution inside heredoc bodies
v=var
while IFS= read -r l; do echo "[$l]"; done <<END
plain $(echo cmdsub) and \`echo bq\` and `echo real-bq`
$(printf 'multi\nline')
nested $(echo "$(echo $v)")
escaped \$(echo no)
END
while IFS= read -r l; do echo "[$l]"; done <<'END'
quoted $(echo no)
END
