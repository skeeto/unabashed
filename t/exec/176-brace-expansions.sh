# Brace expansion happens before parameter expansion and does not
# apply to braces from expansions or ${...}.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
x=1 y=3 a=X b=Y
show {$a,$b}
show {$x..$y}
z='{a,b}'
show $z
show "$z"
show ${x}{p,q}
show ${a:-{p,q}}
show "${a}"{p,q}
show {"$a","$b"}
show {$a,"$b c"}
show $a{1,2}$b
show {x,$(echo c)}
show {`echo 1`,2}
show {$((1+1)),3}
show ${a}{1..3}
set -- one two
show {"$@",x}
