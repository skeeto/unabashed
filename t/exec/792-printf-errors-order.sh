# printf: diagnostics come after the complete lines written before them,
# as bash's standard output is line-buffered
exec 2>&1
printf 'a\n%db\n' x
printf 'a\n%z\n' x
printf 'a\n%s%n\n' x 1bad
printf 'a\n%s%n\n' x
printf 'a\n%(%Y)Z\n' x
printf 'a\n%.*s\n' 99999999999 x
printf 'a\n%5'
echo
printf 'a\n\x\n'
printf 'a\n%b\n' '\x'
printf 'a\n%s\n%d\n' x y
printf -v v 'a\n%d\n' x; printf '%s' "$v"
printf '%s\n' one two three; printf '%d\n' z
