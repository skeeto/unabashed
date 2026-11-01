# parameter expansion in here-documents
unset u; s=val; x=abcabc
set -- p1 'p 2'
cat_() { while IFS= read -r line; do printf '%s\n' "$line"; done; }
cat_ <<END
${u:-'quoted'} ${u:-"dq"} ${s:+"$s"}
${x#'a'} ${x%"c"} ${x/b/'B'} ${x//"b"/x}
$@ | $* | ${#} | ${@:2}
\${s} \$s ${s:-\}} ${u:-a\"b} ${u:-\\}
${u:-~} ${s@Q}
END
cat_ <<'END'
${s} $s
END
