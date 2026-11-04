# vos
# The command line built for a native (MSVCRT) child round-trips
argv '' 'a b' 'a"b' 'a\b' 'a\' 'a\\' 'a\"b' '"' '\' 'tab	x' ' lead' \
     'trail ' 'x y\' '"quoted"' 'a\\"b' '%PATH%' '^&|<>' "it's" 'é'
argv32 '' 'a b' 'a"b' 'a\' 'a\"b' 'x y\'
