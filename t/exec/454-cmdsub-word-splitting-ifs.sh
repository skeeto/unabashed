# Splitting of command substitution results with custom IFS
IFS=,
set -- $(echo 'a,b,,c')
echo "$#"
for x; do echo "<$x>"; done
IFS=
set -- $(echo 'a b c')
echo "$#"
unset IFS
set -- $(printf ' x \n y ')
echo "$# [$1] [$2]"
x=$(printf 'p q')
set -- $x"$(echo ' r ')"
echo "$#"; for a; do echo "<$a>"; done
