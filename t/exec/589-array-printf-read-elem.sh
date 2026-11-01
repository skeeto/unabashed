printf -v "a[2]" %s hi; read "a[0]" b <<< "x y"; declare -p a b
declare -A h; printf -v "h[k k]" %d 42; declare -p h
