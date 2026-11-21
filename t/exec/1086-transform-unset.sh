# An unknown transformation operator, or none (${x@}), on an unset
# value expands to nothing without an error, as bash checks the
# operator only for a value: unset variables, missing elements and
# positional parameters, indirectly too; set values, lists with
# elements and set -u's unbound variables still fail
for op in Z z B 0 % _ '' ZZ QQ; do
  for setup in 'unset x' 'x=' 'x=v' 'x=([1]=a)' 'declare -A x=([k]=v)' \
               'declare -a x' 'set -u; unset x' 'y=x; unset x'; do
    for ref in x 'x[0]' 'x[@]' '!y' 3 '@' 'n[1]'; do
      printf '%s|%s|%s: ' "$op" "$setup" "$ref"
      ( eval "$setup; set -- a b; echo \"<\${$ref@$op}>\"" ) 2>&1
    done
  done
done
