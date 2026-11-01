# Quoted empty strings survive as arguments; unquoted empty
# expansions vanish.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
e=
show ''
show ""
show '' ""
show $e
show "$e"
show $e''
show ''$e
show "$e"$e
show $e $e $e
show a '' b
show a $e b
show "${e}"
show ${e}
show ${e:-}
show "${e:-}"
show ${unset-}
show ${unset-''}
show ${unset-""}
show ${unset-"$e"}
show x${e}""
show \"\"
s=' '
show $s
show $s''
