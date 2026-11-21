# unset through a nameref to an array element expands its subscript,
# but under array_expand_once bash takes it as it is: an associative
# key literally, an index evaluated without expansion, whose error
# abandons the command and the rest of the -c text.
a=(x y z); declare -n r='a[$(echo ran >&2; echo 1)]'
unset r; echo "a $? ${a[*]}"
shopt -s array_expand_once
i=1
a=(x y z); declare -n r2='a[i+1]'
unset r2; echo "b $? ${a[*]}"
declare -A h=([k]=1 ['$(echo ran >&2)']=2 ['$i']=3 [1]=4)
declare -n r3='h[$i]'
unset r3; echo "c $? ${!h[*]}"
declare -n r4='h[$(echo ran >&2)]'
unset -v r4; echo "d $? ${!h[*]}"
declare -n r5="h['k']"
unset r5; echo "e $? ${!h[*]}"
a=(x y z); declare -n r6='a[$(echo ran >&2; echo 2)]'
unset r6; echo notreached
