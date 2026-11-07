# GLOBIGNORE splits at colons, but not in quotes, brackets, extglob
# groups, or what looks like an expansion or substitution: $(...),
# ${...}, $[...], backquotes, <(...) and >(...), never expanded
exec 2>&1
cd /tmp && mkdir g && cd g && touch a b 'c:d' 'e)' '$(x' 'x:y)'
GLOBIGNORE='$(x:y)'; echo *
GLOBIGNORE='$(x:a)'; echo *
GLOBIGNORE='c:d'; echo *
GLOBIGNORE='[:]:a'; echo *
GLOBIGNORE='@(x:y)'; shopt -s extglob; echo *
GLOBIGNORE='${x:-a}:b'; echo *
GLOBIGNORE='"c:d"'; echo *
GLOBIGNORE='`x:a`:b'; echo *
GLOBIGNORE='$[1:2]:a'; echo *
GLOBIGNORE='<(x:y):a'; echo *
GLOBIGNORE='$(echo ")" x:a):b'; echo *
GLOBIGNORE='$((1:2)):a'; echo *
