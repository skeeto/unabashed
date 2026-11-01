# Quoting interactions: splitting and globbing of unquoted results
s=$(printf 'one two  three')
for w in $(printf 'a b\nc'); do echo "<$w>"; done
for w in "$(printf 'a b\nc')"; do echo "<$w>"; done
IFS=:
for w in $(echo x:y::z); do echo "{$w}"; done
IFS=' 	
'
set -- $(echo '*nomatch*')
echo "$# $1"
set -- "$(echo 'a   b')"
echo "$# $1"
echo "$(echo '$HOME')"
echo $(echo "'quoted'")
x='$(echo no)'
echo "$x"
echo $x
