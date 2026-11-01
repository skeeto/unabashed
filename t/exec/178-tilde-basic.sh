# Tilde prefixes at the start of a word.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show ~ ~/ ~/x/y
show x~ "~" '~' \~ ~"/x" "~/x"
cd /tmp
show ~+ ~+/a
cd /home/oracle
show ~- ~-/b
show ~nosuchuser ~nosuchuser/x
HOME=/h/x
show ~ ~/y
HOME=
show ~ ~/z
HOME='/a b'
show ~ ~/c
unset HOME
show ~ ~/d
HOME=/home/oracle
unset OLDPWD
show ~-
PWD=/fake
show ~+
show ~+x ~-x ~0 ~+0
