# In bash, a key-value list for an associative array (its first word not
# [key]=value, nor starting with "[") expands pair by pair; an empty key
# is reported, its value not expanded, and the rest is assigned with
# status 0. A list starting with "[" takes subscripts throughout. From
# declare, every word expands first: without -A as an indexed array's,
# split and brace expanded, an unquoted one that expands to nothing
# dropped.
declare -A h
h=('' v); echo "a $? ${#h[@]}"
h=(a 1 '' $(echo notexpanded >&2) b 3); echo "b $? ${!h[@]}"
h+=('' $(echo notexpanded >&2) c 4); echo "c $? ${!h[@]}"
h+=([]=$(echo notexpanded >&2) d 5); echo "d $? ${!h[@]}"
h=(k); echo "e $? ${!h[@]}"
h=([k]); echo notreached
h=([k] v); echo notreached
h=(* v); echo "f $? ${!h[@]}"
e= x='p q'
declare -A h2=(a $e k v); echo "g $? ${!h2[@]}"
declare -A h3=('' $(echo expanded >&2) k v); echo "h $? ${!h3[@]}"
declare -A h4=(a "$e" k v); echo "i $? ${!h4[@]} <${h4[a]}>"
declare h4=($x {1,2} $e j); echo "j $? ${!h4[@]}"
declare -A h4=($x {1,2} $e j); echo "k $? ${!h4[@]}"
declare h4=(a 1 [k]=v); echo "l $? ${!h4[@]}"
set -x
declare h4=(a $e k v)
declare h4=([$x]=$x ['']=1)
set +x
echo end
