# An operator's word that assigns the parameter, in arithmetic or in a
# ${ ...; }, leaves the value being operated on as it was; an array's
# elements for ${a[@]:off:len} are those once the offset and length are
# known, and a negative offset counts from the end as it was
x=hello_world_this_is_long; echo "${x:(x=5)-5}"; echo "$x"
x=hello_world_this_is_long; echo "${x:(x=5)-5:3}"
x=hello_world_this_is_long; echo "${x#${ x=zz; }hello}"
x=abcdefghijklmnopqrstuvwxyz; echo "${x/${ x=QQ; }abc/X}"
x=abcdefghijklmnopqrstuvwxyz; echo "${x^^${ x=QQ; }[a-c]}"
x=abcdefghijklmnopqrstuvwxyz; echo "${x:-${ x=QQ; }}"
a=(aaaaaaaaaaaaaaaaaaaaa bbbbbbbbbbbbbbbbbbbbbbb ccc)
echo "${a[@]:(a[1]=7)*0}"
a=(aaaaaaaaaaaaaaaaaaaaa bbbbbbbbbbbbbbbbbbbbbbb ccc)
echo "${a[@]:${ a=(x y); }0}"
a=(one_long_parameter two_long_parameter three)
echo "${a[@]: -1:${ a=(p q r s); }1}"
a=('${ a=(q r s); }first_value_long' second_value_long)
echo "${a[@]@P}"; echo "${a[@]}"
set -- one two three; echo "${@:1:${ set -- a b c; }2}"
set -- one two three; echo "${@: -1:${ set -- a b c d; }1}"
x=hello_world_this_is_long; ( echo "${x:(x=5)-5}" )
