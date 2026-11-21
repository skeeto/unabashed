# Given attributes that change an assignment, _ is bound after each
# command as any variable is assigned, as bash binds it: through a
# nameref (its target unexported), to element 0 of an array, converted
# by -u or -l, and not over a readonly one, which keeps its value and
# reports each time. A plain _, exported or unset, is the last word.
bash -c 'declare -n _=foo; echo a b; echo "[$foo]"; declare -p foo; echo "[$_]"'
bash -c 'export foo=1; declare -n _=foo; echo a b; declare -p foo; printenv foo || echo unexported'
bash -c 'echo x; declare -n _; echo a b; declare -p x'
bash -c 'arr=(x y z); declare -n _="arr[1]"; echo a b; declare -p arr'
bash -c 'declare -n _=foo; foo=bar; echo "[$foo]"; for i in 1 2; do :; done; echo "[$foo]"'
bash -c 'declare -n _=foo; f() { local foo; echo a; echo "in [$foo]"; }; f; echo "[$foo]"'
bash -c 'f() { local -n _=foo; echo a b; echo "in $foo"; }; f; echo "[$foo]"; declare -p _'
bash -c 'declare -n _=foo; echo a | cat; echo "[$foo]"; echo "$(echo cs; echo "{$foo}")"; echo "[$foo]"'
bash -c 'declare -n _=foo; unset _; echo a b; declare -p _ foo'
bash -c 'declare -n _=foo; declare +n _; echo a b; declare -p _'
bash -c 'readonly foo=1; declare -n _=foo; echo a b; echo "[$foo]"' 2>&1
echo "-- readonly"
bash -c 'echo x; readonly _; echo a b; echo "[$_]"; (echo "sub [$_]")' 2>&1
bash -c 'readonly _=y; echo a b; echo "[$_]"' 2>&1
echo "-- arrays"
bash -c '_=(1 2); echo a b; declare -p _'
bash -c 'echo x; declare -A _; echo a b; declare -p _'
echo "-- case"
bash -c 'echo x; declare -u _; echo abc; echo "[$_]"'
bash -c 'echo x; f() { local -l _; echo ABC; echo "in [$_]"; }; f; echo "[$_]"'
echo "-- plain"
bash -c 'echo x; export _; echo a; declare -p _'
bash -c 'unset _; echo a b; echo "[$_]"'
bash -c 'echo x; declare -u _; declare +u _; echo abc; echo "[$_]"'
