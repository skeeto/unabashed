# Brace expansion happens before tilde expansion.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show {~,~/x}
show ~{a,b}
show {a,~}
show x{~,y}
show {1,2}~
cd /tmp; cd /home/oracle
show {~+,~-}
