# <<- strips leading tabs from body lines and the delimiter
v=val
while IFS= read -r l; do echo "[$l]"; done <<-EOF
	tab one
		two tabs $v
    four spaces
 	space then tab
	EOF
if true; then
	while IFS= read -r l; do echo "<$l>"; done <<-'END'
		quoted $v
	END
fi
while IFS= read -r l; do echo "{$l}"; done <<EOF
	not stripped
	EOF
EOF
