# eval containing heredocs and multi-line text
eval 'while read l; do echo "h: $l"; done <<END
line one
line two
END'
eval "
a=1
b=2
echo \$((a+b))
"
eval 'f() {
  echo multi-line-func
}'
f
