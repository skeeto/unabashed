# Comments, newlines and heredocs inside command substitutions
x=$(
  echo first   # a comment with ) paren
  echo second
)
echo "$x"
y=$(cat_free=1; while read l; do echo "<$l>"; done <<END
heredoc line
  indented
END
)
echo "$y"
z=$(while read l; do echo "$l"; done <<'END'
$not ) expanded
END
)
echo "$z"
w="$(echo 'a # not comment')"
echo "$w"
v=$(echo a#b)
echo "$v"
